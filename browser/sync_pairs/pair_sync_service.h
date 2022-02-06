// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_H_
#define BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_H_

#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sync/model/model_type_controller_delegate.h"

// The SecurityEventRecorder class allows to record security events via Chrome
// Sync for signed-in users.
class PairSyncService : public KeyedService {
 public:
  PairSyncService() = default;

  PairSyncService(const PairSyncService&) = delete;
  PairSyncService& operator=(const PairSyncService&) = delete;

  ~PairSyncService() override = default;

  // Records the GaiaPasswordReuse security event for the currently signed-in
  // user. The event is recorded via Chrome Sync.
  virtual void RecordPair(std::size_t key, const std::string& value) = 0;

  // Returns the underlying Sync integration point.
  virtual base::WeakPtr<syncer::ModelTypeControllerDelegate>
  GetControllerDelegate() = 0;
};

#endif  // BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_H_
