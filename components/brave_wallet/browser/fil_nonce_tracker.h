/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_NONCE_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_NONCE_TRACKER_H_

#include <string>

#include "base/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/fil_address.h"

namespace brave_wallet {

class EthAddress;
class JsonRpcService;
class EthTxStateManager;

class FilNonceTracker {
 public:
  FilNonceTracker(EthTxStateManager* tx_state_manager,
                  JsonRpcService* json_rpc_service);
  ~FilNonceTracker();
  FilNonceTracker(const FilNonceTracker&) = delete;
  FilNonceTracker operator=(const FilNonceTracker&) = delete;

  using GetNextNonceCallback =
      base::OnceCallback<void(bool success, uint256_t nonce)>;
  void GetNextNonce(const FilAddress& from, GetNextNonceCallback callback);

  base::Lock* GetLock() { return &nonce_lock_; }

 private:
  void OnGetNetworkNonce(FilAddress from,
                         GetNextNonceCallback callback,
                         uint256_t result,
                         mojom::ProviderError error,
                         const std::string& error_message);

  raw_ptr<EthTxStateManager> tx_state_manager_ = nullptr;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;

  base::Lock nonce_lock_;

  base::WeakPtrFactory<FilNonceTracker> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_NONCE_TRACKER_H_
