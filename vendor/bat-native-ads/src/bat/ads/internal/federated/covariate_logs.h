/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// TODO(Moritz Haller): write description
// training data for federated services (learning, tuning, evaluation)
// - instance = single row
// - feature = single column
// - feature value can be of different data type
// 
// to differentiate between chromium/griffin features and federated services
// features, we call them covariates instead
// 
// all covariates are only session based at the moment, that is no measurements
// are persistet across sessions


#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_COVARIATE_LOGS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_COVARIATE_LOGS_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace base {
class Value;
}

namespace ads {

class CovariateLogEntry;

class CovariateLogs final {
 public:
  CovariateLogs();
  CovariateLogs(const CovariateLogs&) = delete;
  CovariateLogs& operator=(const CovariateLogs&) = delete;
  ~CovariateLogs();

  static CovariateLogs* Get();

  void SetCovariateLogEntry(std::unique_ptr<CovariateLogEntry> entry);
  mojom::TrainingInstancePtr GetTrainingInstance() const;

 private:
  base::flat_map<mojom::CovariateType, std::unique_ptr<CovariateLogEntry>>
      covariate_log_entries_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_FEDERATED_COVARIATE_LOGS_H_
