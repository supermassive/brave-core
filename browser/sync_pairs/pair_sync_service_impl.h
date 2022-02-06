// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_IMPL_H_
#define BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_IMPL_H_

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/time/default_clock.h"
#include "base/timer/timer.h"
#include "brave/browser/sync_pairs/pair_sync_bridge.h"
#include "brave/browser/sync_pairs/pair_sync_service.h"

class PairSyncServiceImpl : public PairSyncService {
 public:
  PairSyncServiceImpl(std::unique_ptr<PairSyncBridge> pair_sync_bridge);

  PairSyncServiceImpl(const PairSyncServiceImpl&) = delete;
  PairSyncServiceImpl& operator=(const PairSyncServiceImpl&) = delete;

  ~PairSyncServiceImpl() override;

  void RecordPair(std::size_t key, const std::string& value) override;

  base::WeakPtr<syncer::ModelTypeControllerDelegate> GetControllerDelegate()
      override;

  void Shutdown() override;

 private:
  void RecordPair();

  std::unique_ptr<PairSyncBridge> pair_sync_bridge_;
  raw_ptr<base::Clock> clock_;
  base::RepeatingTimer timer_;
};

#endif  // BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_IMPL_H_
