/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_PREFS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_PREFS_H_

#define RegisterProfilePrefs                                       \
  RegisterProfilePrefs_ChromiumImpl(PrefRegistrySimple* registry); \
  static void RegisterProfilePrefs

#define GarbageCollectExpiredCacheGuids   \
  SetResetDevicesProgressTokenDone();     \
  bool IsResetDevicesProgressTokenDone(); \
  void GarbageCollectExpiredCacheGuids

#include "src/components/sync_device_info/device_info_prefs.h"

#undef GarbageCollectExpiredCacheGuids
#undef RegisterProfilePrefs

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DEVICE_INFO_DEVICE_INFO_PREFS_H_
