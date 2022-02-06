// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_BRIDGE_H_
#define BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_BRIDGE_H_

#include "base/memory/weak_ptr.h"
#include "components/sync/model/model_type_controller_delegate.h"

namespace sync_pb {
class PairSpecifics;
}

class PairSyncBridge {
 public:
  PairSyncBridge() = default;

  PairSyncBridge(const PairSyncBridge&) = delete;
  PairSyncBridge& operator=(const PairSyncBridge&) = delete;

  virtual ~PairSyncBridge() = default;

  virtual void RecordPair(sync_pb::PairSpecifics specifics) = 0;

  // Returns the delegate for the controller, i.e. sync integration point.
  virtual base::WeakPtr<syncer::ModelTypeControllerDelegate>
  GetControllerDelegate() = 0;
};

#endif  // BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_BRIDGE_H_
