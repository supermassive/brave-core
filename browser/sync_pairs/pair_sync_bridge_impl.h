// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_BRIDGE_IMPL_H_
#define BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_BRIDGE_IMPL_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/browser/sync_pairs/pair_sync_bridge.h"
#include "components/sync/model/model_type_change_processor.h"
#include "components/sync/model/model_type_store.h"
#include "components/sync/model/model_type_sync_bridge.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PairSyncBridgeImpl : public PairSyncBridge,
                           public syncer::ModelTypeSyncBridge {
 public:
  PairSyncBridgeImpl(
      syncer::OnceModelTypeStoreFactory store_factory,
      std::unique_ptr<syncer::ModelTypeChangeProcessor> change_processor);

  PairSyncBridgeImpl(const PairSyncBridgeImpl&) = delete;
  PairSyncBridgeImpl& operator=(const PairSyncBridgeImpl&) = delete;

  ~PairSyncBridgeImpl() override;

  void RecordPair(sync_pb::PairSpecifics specifics) override;

  base::WeakPtr<syncer::ModelTypeControllerDelegate> GetControllerDelegate()
      override;

  // ModelTypeSyncBridge implementation.
  std::unique_ptr<syncer::MetadataChangeList> CreateMetadataChangeList()
      override;
  absl::optional<syncer::ModelError> MergeSyncData(
      std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
      syncer::EntityChangeList entity_data) override;
  absl::optional<syncer::ModelError> ApplySyncChanges(
      std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
      syncer::EntityChangeList entity_changes) override;
  void GetData(StorageKeyList storage_keys, DataCallback callback) override;
  void GetAllDataForDebugging(DataCallback callback) override;
  std::string GetClientTag(const syncer::EntityData& entity_data) override;
  std::string GetStorageKey(const syncer::EntityData& entity_data) override;
  void ApplyStopSyncChanges(std::unique_ptr<syncer::MetadataChangeList>
                                delete_metadata_change_list) override;

 private:
  void OnStoreCreated(const absl::optional<syncer::ModelError>& error,
                      std::unique_ptr<syncer::ModelTypeStore> store);

  void OnReadData(
      DataCallback callback,
      const absl::optional<syncer::ModelError>& error,
      std::unique_ptr<syncer::ModelTypeStore::RecordList> data_records,
      std::unique_ptr<syncer::ModelTypeStore::IdList> missing_id_list);

  void OnReadAllData(
      DataCallback callback,
      const absl::optional<syncer::ModelError>& error,
      std::unique_ptr<syncer::ModelTypeStore::RecordList> data_records);

  void OnReadAllMetadata(const absl::optional<syncer::ModelError>& error,
                         std::unique_ptr<syncer::MetadataBatch> metadata_batch);

  void OnCommit(const absl::optional<syncer::ModelError>& error);

  std::unique_ptr<syncer::ModelTypeStore> store_;

  base::WeakPtrFactory<PairSyncBridgeImpl> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_BRIDGE_IMPL_H_
