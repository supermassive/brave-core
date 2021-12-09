/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/contribution/contribution_tip.h"

#include "base/guid.h"
#include "bat/ledger/internal/contribution/pending_contribution_manager.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace contribution {

ContributionTip::ContributionTip(LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
}

ContributionTip::~ContributionTip() = default;

void ContributionTip::Process(
    const std::string& publisher_key,
    const double amount,
    ledger::ResultCallback callback) {
  if (publisher_key.empty()) {
    BLOG(0, "Failed to do tip due to missing publisher key");
    callback(type::Result::NOT_FOUND);
    return;
  }

  const auto server_callback = std::bind(&ContributionTip::ServerPublisher,
      this,
      _1,
      publisher_key,
      amount,
      callback);

  ledger_->publisher()->GetServerPublisherInfo(
      publisher_key,
      server_callback);
}

void ContributionTip::ServerPublisher(
    type::ServerPublisherInfoPtr server_info,
    const std::string& publisher_key,
    const double amount,
    ledger::ResultCallback callback) {
  auto status = type::PublisherStatus::NOT_VERIFIED;
  if (server_info) {
    status = server_info->status;
  }

  // Save to the pending list if not verified
  if (status == type::PublisherStatus::NOT_VERIFIED) {
    ledger_->context()
        .Get<PendingContributionManager>()
        .AddPendingContribution(PendingContributionType::kOneTime,
                                publisher_key, amount)
        .Then(callback_adapter_([callback](bool success) {
          callback(CallbackAdapter::ResultCode(success));
        }));
    return;
  }

  type::ContributionQueuePublisherList queue_list;
  auto publisher = type::ContributionQueuePublisher::New();
  publisher->publisher_key = publisher_key;
  publisher->amount_percent = 100.0;
  queue_list.push_back(std::move(publisher));

  auto queue = type::ContributionQueue::New();
  queue->id = base::GenerateGUID();
  queue->type = type::RewardsType::ONE_TIME_TIP;
  queue->amount = amount;
  queue->partial = false;
  queue->publishers = std::move(queue_list);

  auto save_callback = std::bind(&ContributionTip::QueueSaved,
      this,
      _1,
      callback);

  ledger_->database()->SaveContributionQueue(std::move(queue), save_callback);
}

void ContributionTip::QueueSaved(
    const type::Result result,
    ledger::ResultCallback callback) {
  if (result == type::Result::LEDGER_OK) {
    ledger_->contribution()->ProcessContributionQueue();
  } else {
    BLOG(0, "Queue was not saved");
  }

  callback(type::Result::LEDGER_OK);
}

}  // namespace contribution
}  // namespace ledger
