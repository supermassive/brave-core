// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/sync_pairs/pair_sync_bridge_impl.h"

#include <set>
#include <utility>
#include <vector>

#include "base/big_endian.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/check_op.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "components/sync/model/entity_change.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/mutable_data_batch.h"
#include "components/sync/protocol/pair_specifics.pb.h"

namespace {

std::string GetStorageKeyFromSpecifics(
    const sync_pb::PairSpecifics& specifics) {
  // Force Big Endian, this means newly created keys are last in sort order,
  // which allows leveldb to append new writes, which it is best at.
  // TODO(markusheintz): Until we force |event_time_usec| to never conflict,
  // this has the potential for errors.
  std::string key(8, 0);
  base::WriteBigEndian(&key[0], specifics.key());
  return key;
}

std::unique_ptr<syncer::EntityData> ToEntityData(
    sync_pb::PairSpecifics specifics) {
  auto entity_data = std::make_unique<syncer::EntityData>();
  entity_data->name = base::NumberToString(specifics.key());
  entity_data->specifics.mutable_pair()->Swap(&specifics);
  return entity_data;
}

}  // namespace

PairSyncBridgeImpl::PairSyncBridgeImpl(
    syncer::OnceModelTypeStoreFactory store_factory,
    std::unique_ptr<syncer::ModelTypeChangeProcessor> change_processor)
    : syncer::ModelTypeSyncBridge(std::move(change_processor)) {
  LOG(INFO) << "PairSyncBridgeImpl::PairSyncBridgeImpl";
  std::move(store_factory)
      .Run(syncer::PAIRS, base::BindOnce(&PairSyncBridgeImpl::OnStoreCreated,
                                         weak_ptr_factory_.GetWeakPtr()));
}

PairSyncBridgeImpl::~PairSyncBridgeImpl() {}

void PairSyncBridgeImpl::RecordPair(sync_pb::PairSpecifics specifics) {
  if (!store_) {
    LOG(INFO) << "PairSyncBridgeImpl: !store_";
    return;
  }
  if (!change_processor()->IsTrackingMetadata()) {
    LOG(INFO) << "PairSyncBridgeImpl: not tracking metadata";
    return;
  }

  std::string storage_key = GetStorageKeyFromSpecifics(specifics);

  std::unique_ptr<syncer::ModelTypeStore::WriteBatch> write_batch =
      store_->CreateWriteBatch();
  write_batch->WriteData(storage_key, specifics.SerializeAsString());

  change_processor()->Put(storage_key, ToEntityData(std::move(specifics)),
                          write_batch->GetMetadataChangeList());

  store_->CommitWriteBatch(std::move(write_batch),
                           base::BindOnce(&PairSyncBridgeImpl::OnCommit,
                                          weak_ptr_factory_.GetWeakPtr()));
}

base::WeakPtr<syncer::ModelTypeControllerDelegate>
PairSyncBridgeImpl::GetControllerDelegate() {
  return change_processor()->GetControllerDelegate();
}

std::unique_ptr<syncer::MetadataChangeList>
PairSyncBridgeImpl::CreateMetadataChangeList() {
  return syncer::ModelTypeStore::WriteBatch::CreateMetadataChangeList();
}

absl::optional<syncer::ModelError> PairSyncBridgeImpl::MergeSyncData(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
    syncer::EntityChangeList entity_data) {
  DCHECK(entity_data.empty());
  DCHECK(change_processor()->IsTrackingMetadata());
  DCHECK(!change_processor()->TrackedAccountId().empty());
  return ApplySyncChanges(std::move(metadata_change_list),
                          std::move(entity_data));
}

absl::optional<syncer::ModelError> PairSyncBridgeImpl::ApplySyncChanges(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
    syncer::EntityChangeList entity_changes) {
  std::unique_ptr<syncer::ModelTypeStore::WriteBatch> write_batch =
      store_->CreateWriteBatch();
  for (const std::unique_ptr<syncer::EntityChange>& change : entity_changes) {
    DCHECK_EQ(syncer::EntityChange::ACTION_DELETE, change->type());
    write_batch->DeleteData(change->storage_key());
  }

  write_batch->TakeMetadataChangesFrom(std::move(metadata_change_list));
  store_->CommitWriteBatch(std::move(write_batch),
                           base::BindOnce(&PairSyncBridgeImpl::OnCommit,
                                          weak_ptr_factory_.GetWeakPtr()));
  return {};
}

void PairSyncBridgeImpl::GetData(StorageKeyList storage_keys,
                                 DataCallback callback) {
  store_->ReadData(storage_keys, base::BindOnce(&PairSyncBridgeImpl::OnReadData,
                                                weak_ptr_factory_.GetWeakPtr(),
                                                std::move(callback)));
}

void PairSyncBridgeImpl::GetAllDataForDebugging(DataCallback callback) {
  store_->ReadAllData(base::BindOnce(&PairSyncBridgeImpl::OnReadAllData,
                                     weak_ptr_factory_.GetWeakPtr(),
                                     std::move(callback)));
}

std::string PairSyncBridgeImpl::GetClientTag(
    const syncer::EntityData& entity_data) {
  return GetStorageKey(entity_data);
}

std::string PairSyncBridgeImpl::GetStorageKey(
    const syncer::EntityData& entity_data) {
  return GetStorageKeyFromSpecifics(entity_data.specifics.pair());
}

void PairSyncBridgeImpl::ApplyStopSyncChanges(
    std::unique_ptr<syncer::MetadataChangeList> delete_metadata_change_list) {
  if (delete_metadata_change_list) {
    store_->DeleteAllDataAndMetadata(base::BindOnce(
        &PairSyncBridgeImpl::OnCommit, weak_ptr_factory_.GetWeakPtr()));
  }
}

void PairSyncBridgeImpl::OnStoreCreated(
    const absl::optional<syncer::ModelError>& error,
    std::unique_ptr<syncer::ModelTypeStore> store) {
  LOG(INFO) << "PairSyncBridgeImpl::OnStoreCreated";
  if (error) {
    change_processor()->ReportError(*error);
    return;
  }

  store_ = std::move(store);
  store_->ReadAllMetadata(base::BindOnce(&PairSyncBridgeImpl::OnReadAllMetadata,
                                         weak_ptr_factory_.GetWeakPtr()));
}

void PairSyncBridgeImpl::OnReadData(
    DataCallback callback,
    const absl::optional<syncer::ModelError>& error,
    std::unique_ptr<syncer::ModelTypeStore::RecordList> data_records,
    std::unique_ptr<syncer::ModelTypeStore::IdList> missing_id_list) {
  OnReadAllData(std::move(callback), error, std::move(data_records));
}

void PairSyncBridgeImpl::OnReadAllData(
    DataCallback callback,
    const absl::optional<syncer::ModelError>& error,
    std::unique_ptr<syncer::ModelTypeStore::RecordList> data_records) {
  if (error) {
    change_processor()->ReportError(*error);
    return;
  }

  auto batch = std::make_unique<syncer::MutableDataBatch>();
  for (const syncer::ModelTypeStore::Record& r : *data_records) {
    sync_pb::PairSpecifics specifics;

    if (specifics.ParseFromString(r.value)) {
      DCHECK_EQ(r.id, GetStorageKeyFromSpecifics(specifics));
      batch->Put(r.id, ToEntityData(std::move(specifics)));
    } else {
      change_processor()->ReportError(
          {FROM_HERE, "Failed deserializing pairs."});
      return;
    }
  }
  std::move(callback).Run(std::move(batch));
}

void PairSyncBridgeImpl::OnReadAllMetadata(
    const absl::optional<syncer::ModelError>& error,
    std::unique_ptr<syncer::MetadataBatch> metadata_batch) {
  LOG(INFO) << "PairSyncBridgeImpl::OnReadAllMetadata";
  if (error) {
    change_processor()->ReportError(*error);
  } else {
    change_processor()->ModelReadyToSync(std::move(metadata_batch));
  }
}

void PairSyncBridgeImpl::OnCommit(
    const absl::optional<syncer::ModelError>& error) {
  if (error) {
    change_processor()->ReportError(*error);
  }
}
