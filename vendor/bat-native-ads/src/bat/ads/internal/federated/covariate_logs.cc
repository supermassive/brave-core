/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/federated/covariate_logs.h"

#include <string>
#include <utility>

#include "bat/ads/internal/logging.h"

#include "base/check.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/federated/covariate_log_entry.h"
#include "bat/ads/internal/federated/covariate_logs_util.h"
#include "bat/ads/internal/federated/log_entries/locale_country_at_time_of_serving_covariate_log_entry.h"
#include "bat/ads/internal/federated/log_entries/number_of_tabs_opened_in_past_30_minutes.h"

namespace ads {

namespace {

CovariateLogs* g_covariate_logs = nullptr;

}  // namespace

CovariateLogs::CovariateLogs() {
  DCHECK(!g_covariate_logs);
  g_covariate_logs = this;

  SetCovariateLogEntry(std::make_unique<NumberOfTabsOpenedInPast30Minutes>());
  SetCovariateLogEntry(std::make_unique<LocaleCountryAtTimeOfServingCovariateLogEntry>());
}

CovariateLogs::~CovariateLogs() {
  DCHECK(g_covariate_logs);
  g_covariate_logs = nullptr;
}

// static
CovariateLogs* CovariateLogs::Get() {
  DCHECK(g_covariate_logs);
  return g_covariate_logs;
}

void CovariateLogs::SetCovariateLogEntry(
    std::unique_ptr<CovariateLogEntry> entry) {
  DCHECK(entry);
  mojom::CovariateType key = entry->GetCovariateType();
  covariate_log_entries_[key] = std::move(entry);
}

mojom::TrainingInstancePtr CovariateLogs::GetTrainingInstance() const {
  mojom::TrainingInstancePtr log_entry = mojom::TrainingInstance::New();
  for (const auto& entry_pair : covariate_log_entries_) {
    CovariateLogEntry* entry = entry_pair.second.get();
    
    mojom::CovariatePtr covariate = mojom::Covariate::New();
    covariate->data_type = entry->GetDataType();
    covariate->covariate_type = entry->GetCovariateType();
    covariate->value = entry->GetValue();
    log_entry->covariates.push_back(std::move(covariate));
  }

  return log_entry;
}

}  // namespace ads
