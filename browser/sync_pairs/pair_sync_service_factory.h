// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_FACTORY_H_

#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename t>
struct DefaultSingletonTraits;
}

class Profile;
class PairSyncService;

class PairSyncServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the singleton instance of PairSyncServiceFactory.
  static PairSyncServiceFactory* GetInstance();

  // Returns the PairSyncService associated with |profile|.
  static PairSyncService* GetForProfile(Profile* profile);

  PairSyncServiceFactory(const PairSyncServiceFactory&) = delete;
  PairSyncServiceFactory& operator=(const PairSyncServiceFactory&) = delete;

 private:
  friend struct base::DefaultSingletonTraits<PairSyncServiceFactory>;

  PairSyncServiceFactory();
  ~PairSyncServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
};
#endif  // BRAVE_BROWSER_SYNC_PAIRS_PAIR_SYNC_SERVICE_FACTORY_H_
