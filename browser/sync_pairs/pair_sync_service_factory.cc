// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/sync_pairs/pair_sync_service_factory.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "brave/browser/sync_pairs/pair_sync_bridge.h"
#include "brave/browser/sync_pairs/pair_sync_bridge_impl.h"
#include "brave/browser/sync_pairs/pair_sync_service_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/model_type_store_service_factory.h"
#include "chrome/common/channel_info.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/sync/base/report_unrecoverable_error.h"
#include "components/sync/model/client_tag_based_model_type_processor.h"
#include "components/sync/model/model_type_store_service.h"

// static
PairSyncServiceFactory* PairSyncServiceFactory::GetInstance() {
  return base::Singleton<PairSyncServiceFactory>::get();
}

// static
PairSyncService* PairSyncServiceFactory::GetForProfile(Profile* profile) {
  DCHECK(!profile->IsOffTheRecord());
  return static_cast<PairSyncService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}
PairSyncServiceFactory::PairSyncServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "PairSyncService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ModelTypeStoreServiceFactory::GetInstance());
}

PairSyncServiceFactory::~PairSyncServiceFactory() {}

KeyedService* PairSyncServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  LOG(INFO) << "PairSyncServiceFactory::BuildServiceInstanceFor";
  Profile* profile = static_cast<Profile*>(context);
  syncer::OnceModelTypeStoreFactory store_factory =
      ModelTypeStoreServiceFactory::GetForProfile(profile)->GetStoreFactory();

  auto change_processor =
      std::make_unique<syncer::ClientTagBasedModelTypeProcessor>(
          syncer::PAIRS, base::BindRepeating(&syncer::ReportUnrecoverableError,
                                             chrome::GetChannel()));
  auto pair_sync_bridge = std::make_unique<PairSyncBridgeImpl>(
      std::move(store_factory), std::move(change_processor));
  return new PairSyncServiceImpl(std::move(pair_sync_bridge));
}
