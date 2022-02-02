/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/log_entries/number_of_tabs_opened_in_past_30_minutes.h"

#include <cstdint>

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/federated/covariate_logs_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/user_activity/user_activity.h"
#include "bat/ads/pref_names.h"

namespace ads {

namespace {

int64_t ComputeTabsOpened(const UserActivityEventList& events) {
  int64_t count = 0;
  for (const auto& event : events) {
    if (event.type  == UserActivityEventType::kOpenedNewTab) {
      count++;
    }
  }

  return count;
}

}  // namespace

NumberOfTabsOpenedInPast30Minutes::NumberOfTabsOpenedInPast30Minutes() = default;

NumberOfTabsOpenedInPast30Minutes::~NumberOfTabsOpenedInPast30Minutes() = default;

mojom::DataType NumberOfTabsOpenedInPast30Minutes::GetDataType() const {
  return mojom::DataType::kInt64;
}

mojom::CovariateType NumberOfTabsOpenedInPast30Minutes::GetCovariateType() const {
  return mojom::CovariateType::kNumberOfTabsOpenedInPast30Minutes;
}

std::string NumberOfTabsOpenedInPast30Minutes::GetValue() const {
  const UserActivityEventList events =
      UserActivity::Get()->GetHistoryForTimeWindow(base::Minutes(30));

  return ToString(ComputeTabsOpened(events));
}

}  // namespace ads
