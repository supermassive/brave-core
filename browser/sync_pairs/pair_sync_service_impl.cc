// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/sync_pairs/pair_sync_service_impl.h"
#include "base/logging.h"

#include <memory>
#include <utility>

#include "base/rand_util.h"
#include "brave/components/sync/protocol/pair_specifics.pb.h"

PairSyncServiceImpl::PairSyncServiceImpl(
    std::unique_ptr<PairSyncBridge> pair_sync_bridge)
    : pair_sync_bridge_(
          (DCHECK(pair_sync_bridge), std::move(pair_sync_bridge))),
      clock_(base::DefaultClock::GetInstance()) {
  timer_.Start(FROM_HERE, base::Seconds(30), this,
               &PairSyncServiceImpl::RecordPair);
}

PairSyncServiceImpl::~PairSyncServiceImpl() {}

void PairSyncServiceImpl::RecordPair(std::size_t key,
                                     const std::string& value) {
  sync_pb::PairSpecifics specifics;
  specifics.set_key(key);
  specifics.set_value(value);

  pair_sync_bridge_->RecordPair(std::move(specifics));
}

base::WeakPtr<syncer::ModelTypeControllerDelegate>
PairSyncServiceImpl::GetControllerDelegate() {
  if (pair_sync_bridge_) {
    return pair_sync_bridge_->GetControllerDelegate();
  }

  return base::WeakPtr<syncer::ModelTypeControllerDelegate>();
}

void PairSyncServiceImpl::Shutdown() {}

void PairSyncServiceImpl::RecordPair() {
  LOG(INFO) << "PairSyncServiceImpl::RecordPair";

  std::string value;
  for (std::size_t index = 0; index < 16; ++index) {
    value.append(1, static_cast<char>(base::RandInt('A', 'Z')));
  }

  RecordPair(clock_->Now().since_origin().InMicroseconds(), value);
}