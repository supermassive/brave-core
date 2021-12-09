/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/delay_generator.h"

#include <algorithm>

#include "base/threading/sequenced_task_runner_handle.h"
#include "bat/ledger/internal/core/bat_ledger_job.h"
#include "brave_base/random.h"

namespace ledger {

namespace {

class DelayJob : public BATLedgerJob<bool> {
 public:
  void Start(base::TimeDelta delay) {
    base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, base::BindOnce(&DelayJob::Callback, base::AsWeakPtr(this)),
        delay);
  }

 private:
  void Callback() { Complete(true); }
};

}  // namespace

const char DelayGenerator::kContextKey[] = "delay-generator";

Future<bool> DelayGenerator::Delay(base::Location location,
                                   base::TimeDelta delay) {
  context().LogInfo(location) << "Delay set for " << delay;
  return context().StartJob<DelayJob>(delay);
}

Future<bool> DelayGenerator::RandomDelay(base::Location location,
                                         base::TimeDelta delay) {
  uint64_t seconds = brave_base::random::Geometric(delay.InSecondsF());
  return Delay(location, base::Seconds(static_cast<int64_t>(seconds)));
}

Future<bool> DelayGenerator::RandomDelayWithBackoff(base::Location location,
                                                    base::TimeDelta delay,
                                                    base::TimeDelta max_delay,
                                                    int backoff_count) {
  delay *= 1 << std::min(backoff_count, 24);
  return RandomDelay(location, std::min(delay, max_delay));
}

}  // namespace ledger
