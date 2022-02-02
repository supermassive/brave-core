/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/log_entries/ad_notification_clicked_covariate_log_entry.h"

#include <cstdint>

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/federated/covariate_logs_util.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "bat/ads/pref_names.h"

namespace ads {

AdNotificationClickedCovariateLogEntry::AdNotificationClickedCovariateLogEntry() = default;

AdNotificationClickedCovariateLogEntry::~AdNotificationClickedCovariateLogEntry() = default;

void AdNotificationClickedCovariateLogEntry::SetClicked(const bool clicked) {
  clicked_ = clicked;
}

mojom::DataType AdNotificationClickedCovariateLogEntry::GetDataType() const {
  return mojom::DataType::kBool;
}

mojom::CovariateType AdNotificationClickedCovariateLogEntry::GetCovariateType() const {
  return mojom::CovariateType::kAdNotificationClicked;
}

std::string AdNotificationClickedCovariateLogEntry::GetValue() const {
  return ToString(clicked_);
}

}  // namespace ads
