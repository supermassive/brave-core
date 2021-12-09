/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contribution/pending_contribution_manager.h"

#include <string>
#include <utility>
#include <vector>

#include "base/guid.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "bat/ledger/internal/core/delay_generator.h"
#include "bat/ledger/internal/core/sql_store.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace ledger {

namespace {

constexpr base::TimeDelta kExpiresAfter = base::Days(90);

// TODO(zenparsing): We need to clean up this table. "viewing_id" is not used.
// "added_date" should be "created_at". "publisher_id" should be
// "publisher_key". We should also add the original publisher status (but we
// don't want to use the old status values). We could probably get rid of the
// processed_publisher table if we only notify the observer when a publisher
// transitions from not verified to verified (at all).

using AddInfo = PendingContributionManager::AddInfo;
using PendingInfo = PendingContributionManager::PendingInfo;

class GetPendingJob : public BATLedgerJob<std::vector<PendingInfo>> {
 public:
  void Start() {
    static const char kSQL[] = R"sql(
      SELECT
        pc.pending_contribution_id, pc.type, pc.amount, pc.added_date,
        pc.publisher_id, pi.name, pi.url, spi.status
      FROM pending_contribution AS pc
      INNER JOIN publisher_info AS pi
        ON pi.publisher_id = pc.publisher_id
      LEFT JOIN server_publisher_info AS spi
        ON spi.publisher_key = pc.publisher_id
      ORDER BY
        pc.added_date
    )sql";

    context().Get<SQLStore>().Query(kSQL).Then(
        ContinueWith(&GetPendingJob::OnQueryCompleted));
  }

 private:
  bool ParseVerified(int64_t value) {
    switch (static_cast<mojom::PublisherStatus>(value)) {
      case mojom::PublisherStatus::NOT_VERIFIED:
      case mojom::PublisherStatus::CONNECTED:
        return false;
        break;
      default:
        return true;
    }
  }

  PendingContributionType ParseType(int64_t value) {
    switch (static_cast<mojom::RewardsType>(value)) {
      case mojom::RewardsType::RECURRING_TIP:
        return PendingContributionType::kRecurring;
      default:
        return PendingContributionType::kOneTime;
    }
  }

  void OnQueryCompleted(SQLReader reader) {
    std::vector<PendingInfo> pending_list;

    while (reader.Step()) {
      PendingInfo info;
      info.id = reader.ColumnInt64(0);
      info.type = ParseType(reader.ColumnInt64(1));
      info.amount = reader.ColumnDouble(2);
      info.created_at = base::Time::FromDoubleT(reader.ColumnDouble(3));
      info.expires_at = info.created_at + kExpiresAfter;
      info.publisher_key = reader.ColumnString(4);
      info.publisher_verified = ParseVerified(reader.ColumnInt64(7));
      info.publisher_name = reader.ColumnString(5);
      info.publisher_url = reader.ColumnString(6);
      pending_list.push_back(std::move(info));
    }

    Complete(std::move(pending_list));
  }
};

class AddJob : public BATLedgerJob<bool> {
 public:
  void Start(const std::vector<AddInfo>& list) {
    static const char kSQL[] = R"sql(
      INSERT OR REPLACE INTO pending_contribution
        (publisher_id, amount, added_date, type, viewing_id)
      VALUES (?, ?, ?, ?, '')
    )sql";

    double created_at = base::Time::Now().ToDoubleT();
    std::vector<mojom::DBCommandPtr> commands;

    for (auto& add_info : list) {
      if (add_info.publisher_key.empty() || add_info.amount <= 0)
        return Complete(false);

      int64_t type_id = static_cast<int64_t>(mojom::RewardsType::ONE_TIME_TIP);
      if (add_info.type == PendingContributionType::kRecurring)
        type_id = static_cast<int64_t>(mojom::RewardsType::RECURRING_TIP);

      commands.push_back(SQLStore::CreateCommand(
          kSQL, add_info.publisher_key, add_info.amount, created_at, type_id));
    }

    context()
        .Get<SQLStore>()
        .RunTransaction(std::move(commands))
        .Then(ContinueWith(&AddJob::OnInserted));
  }

 private:
  void OnInserted(SQLReader reader) {
    bool success = reader.Succeeded();
    // TODO(zenparsing): Log if error.
    context().GetLedgerClient()->PendingContributionSaved(
        success ? mojom::Result::LEDGER_OK : mojom::Result::LEDGER_ERROR);
    Complete(success);
  }
};

class DeleteJob : public BATLedgerJob<bool> {
 public:
  void Start(int64_t id) {
    static const char kSQL[] = R"sql(
      DELETE FROM pending_contribution WHERE pending_contribution_id = ?
    )sql";

    context().Get<SQLStore>().Run(kSQL, id).Then(
        ContinueWith(&DeleteJob::OnDeleted));
  }

 private:
  void OnDeleted(SQLReader reader) { Complete(reader.Succeeded()); }
};

class ClearJob : public BATLedgerJob<bool> {
 public:
  void Start() {
    static const char kSQL[] = R"sql(
      DELETE FROM pending_contribution
    )sql";

    context().Get<SQLStore>().Run(kSQL).Then(
        ContinueWith(&ClearJob::OnDeleted));
  }

 private:
  void OnDeleted(SQLReader reader) { Complete(reader.Succeeded()); }
};

class ProcessJob : public BATLedgerJob<bool> {
 public:
  void Start() {
    context().LogInfo(FROM_HERE) << "Starting pending contribution processing";

    static const char kSQL[] = R"sql(
      DELETE FROM pending_contribution
      WHERE added_date < ?
    )sql";

    base::Time cutoff = base::Time::Now() - kExpiresAfter;

    context()
        .Get<SQLStore>()
        .Run(kSQL, cutoff.ToDoubleT())
        .Then(ContinueWith(&ProcessJob::OnExipiredRecordsDeleted));
  }

 private:
  void OnExipiredRecordsDeleted(SQLReader reader) {
    if (!reader.Succeeded()) {
      context().LogError(FROM_HERE) << "Unable to delete expired pending "
                                       "contributions";
      return Complete(false);
    }

    context().StartJob<GetPendingJob>().Then(
        ContinueWith(&ProcessJob::OnGetPendingCompleted));
  }

  void OnGetPendingCompleted(std::vector<PendingInfo> pending_list) {
    // TODO(zenparsing): What happens if the pending query fails? Do we just
    // continue on from here, or should we return false? If we want to know if
    // the pending query succeeded, how would we get that info? An optional of
    // vector?

    pending_records_ = std::move(pending_list);
    OnProcessNextDelayElapsed(true);
  }

  void ProcessNext() {
    // TODO(zenparsing): This "is_testing" business is gross. How should we
    // handle this instead? Why is it gross? Because we have all of these random
    // is_testing things sprinkled around ad-hoc. Also, we need to avoid global
    // variables.
    base::TimeDelta delay =
        ledger::is_testing ? base::Seconds(2) : base::Seconds(45);

    context()
        .Get<DelayGenerator>()
        .RandomDelay(FROM_HERE, delay)
        .Then(ContinueWith(&ProcessJob::OnProcessNextDelayElapsed));
  }

  void OnProcessNextDelayElapsed(bool success) {
    if (pending_records_.empty()) {
      context().LogInfo(FROM_HERE) << "Pending contribution processing "
                                   << "complete";
      context().GetLedgerImpl()->contribution()->ProcessContributionQueue();
      return Complete(true);
    }

    current_record_ = std::move(pending_records_.back());
    pending_records_.pop_back();

    context().GetLedgerImpl()->publisher()->FetchServerPublisherInfo(
        current_record_.publisher_key,
        CreateLambdaCallback(&ProcessJob::OnServerPublisherInfoFetched));
  }

  void OnServerPublisherInfoFetched(mojom::ServerPublisherInfoPtr info) {
    if (!info) {
      context().LogInfo(FROM_HERE) << "Could not obtain publisher info for "
                                   << current_record_.publisher_key;
      return ProcessNext();
    }

    switch (info->status) {
      case mojom::PublisherStatus::NOT_VERIFIED:
      case mojom::PublisherStatus::CONNECTED: {
        return ProcessNext();
      }
      default: {
        break;
      }
    }

    context().LogInfo(FROM_HERE) << "Processing pending contribution for "
                                 << current_record_.publisher_key;

    // TODO(zenparsing): I suppose that we need to ensure that the user can
    // actually process the contribution. We'd need to have the user's balance
    // but we'd also need to know their external wallets and things like that.
    // Another wrinkle is that while the user's balance may be sufficient for
    // one pending tip it might not be sufficient for the next one. I believe
    // that the original design was to basically wait until processing finished
    // for each pending tip before moving to the next one (hence the delay?).

    std::vector<mojom::ContributionQueuePublisherPtr> queue_publishers;
    auto publisher = mojom::ContributionQueuePublisher::New();
    publisher->publisher_key = current_record_.publisher_key;
    publisher->amount_percent = 100.0;
    queue_publishers.push_back(std::move(publisher));

    // TODO(zenparsing): This naming is not good. These are queue entries, not
    // queues.

    auto queue = mojom::ContributionQueue::New();
    queue->id = base::GenerateGUID();
    queue->type = mojom::RewardsType::ONE_TIME_TIP;
    queue->amount = current_record_.amount;
    queue->partial = false;
    queue->publishers = std::move(queue_publishers);

    // TODO(zenparsing): We shouldn't even need this mojom business - a
    // contribution queue manager API should allow us to add with "normal" data.

    context().GetLedgerImpl()->database()->SaveContributionQueue(
        std::move(queue), CreateLambdaCallback(&ProcessJob::OnQueueSaved));
  }

  void OnQueueSaved(mojom::Result result) {
    if (result != mojom::Result::LEDGER_OK) {
      context().LogError(FROM_HERE) << "Unable to save contribution queue item "
                                    << "for pending contribution";
      return Complete(false);
    }

    context()
        .StartJob<DeleteJob>(current_record_.id)
        .Then(ContinueWith(&ProcessJob::OnPendingRecordDeleted));
  }

  void OnPendingRecordDeleted(bool success) {
    if (!success) {
      // TODO(zenparsing): Not sure if we should continue on or not. We can't
      // delete the queue entry we just added, can we? We need to delete the
      // added contibution queue item if we are unable to remove the pending
      // record. Actually, we should probably mark it as inactive first, and
      // then delete it later.
      return Complete(false);
    }

    context().GetLedgerClient()->OnContributeUnverifiedPublishers(
        mojom::Result::PENDING_PUBLISHER_REMOVED, current_record_.publisher_key,
        current_record_.publisher_name);

    const char kSQL[] = R"sql(
      INSERT OR IGNORE INTO processed_publisher (publisher_key) VALUES (?)
    )sql";

    context()
        .Get<SQLStore>()
        .Run(kSQL, current_record_.publisher_key)
        .Then(ContinueWith(&ProcessJob::OnProcessedPublisherInserted));
  }

  void OnProcessedPublisherInserted(SQLReader reader) {
    if (!reader.Succeeded()) {
      context().LogError(FROM_HERE) << "Failed to insert into processed "
                                       "publisher table";
    } else if (reader.Step()) {
      uint64_t inserted_rows = reader.ColumnInt64(0);
      if (inserted_rows > 0) {
        // TODO(zenparsing): This isn't quite right for the external wallet
        // mismatch case. The publisher could already be verified and *also*
        // still have a mismatch by the time that we get here.
        context().GetLedgerClient()->OnContributeUnverifiedPublishers(
            mojom::Result::VERIFIED_PUBLISHER, current_record_.publisher_key,
            current_record_.publisher_name);
      }
    }

    ProcessNext();
  }

  PendingInfo current_record_;
  std::vector<PendingInfo> pending_records_;
};

}  // namespace

PendingInfo::PendingInfo() = default;
PendingInfo::~PendingInfo() = default;

PendingInfo::PendingInfo(const PendingInfo& other) = default;
PendingInfo& PendingInfo::operator=(const PendingInfo& other) = default;

PendingInfo::PendingInfo(PendingInfo&& other) = default;
PendingInfo& PendingInfo::operator=(PendingInfo&& other) = default;

const char PendingContributionManager::kContextKey[] =
    "pending-contribution-processor";

PendingContributionManager::PendingContributionManager() = default;

PendingContributionManager::~PendingContributionManager() = default;

Future<std::vector<PendingInfo>>
PendingContributionManager::GetPendingContributions() {
  return context().StartJob<GetPendingJob>();
}

Future<double> PendingContributionManager::GetPendingContributionsTotal() {
  return GetPendingContributions().Map(
      base::BindOnce([](std::vector<PendingInfo> list) {
        double total = 0;
        for (auto& info : list) {
          total += info.amount;
        }
        return total;
      }));
}

Future<bool> PendingContributionManager::ProcessPendingContributions() {
  return process_cache_.GetFuture(
      [&]() { return context().StartJob<ProcessJob>(); });
}

Future<bool> PendingContributionManager::DeletePendingContribution(int64_t id) {
  return context().StartJob<DeleteJob>(id);
}

Future<bool> PendingContributionManager::ClearPendingContributions() {
  return context().StartJob<ClearJob>();
}

Future<bool> PendingContributionManager::AddPendingContribution(
    PendingContributionType type,
    const std::string& publisher_key,
    double amount) {
  return AddPendingContributions({AddInfo{
      .type = type, .publisher_key = publisher_key, .amount = amount}});
}

Future<bool> PendingContributionManager::AddPendingContributions(
    const std::vector<AddInfo>& list) {
  return context().StartJob<AddJob>(list);
}

}  // namespace ledger
