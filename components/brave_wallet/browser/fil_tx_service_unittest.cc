/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_tx_service.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/asset_ratio_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/fil_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

void AddUnapprovedTransactionSuccessCallback(bool* callback_called,
                                             std::string* tx_meta_id,
                                             bool success,
                                             const std::string& id,
                                             const std::string& error_message) {
  EXPECT_TRUE(success);
  EXPECT_FALSE(id.empty());
  EXPECT_TRUE(error_message.empty());
  *callback_called = true;
  *tx_meta_id = id;
}

void AddUnapprovedTransactionFailureCallback(bool* callback_called,
                                             bool success,
                                             const std::string& id,
                                             const std::string& error_message) {
  EXPECT_FALSE(success);
  EXPECT_TRUE(id.empty());
  EXPECT_FALSE(error_message.empty());
  *callback_called = true;
}

}  // namespace

class FilTxServiceUnitTest : public testing::Test {
 public:
  FilTxServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        browser_context_(new content::TestBrowserContext()),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  void SetUp() override {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();

          if (request.url.spec().find("action=gasoracle") !=
              std::string::npos) {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"payload\": {\"status\": \"1\", \"message\": \"\", "
                "\"result\": {\"LastBlock\": \"13243541\", \"SafeGasPrice\": "
                "\"47\", \"ProposeGasPrice\": \"48\", \"FastGasPrice\": "
                "\"49\", \"suggestBaseFee\": \"46.574033786\", "
                "\"gasUsedRatio\": "
                "\"0.27036175840958,0.0884828740801432,0.0426623303159149,0."
                "972173412918789,0.319781207901446\"}}, \"lastUpdated\": "
                "\"2021-09-22T21:45:40.015Z\"}");
            return;
          }

          base::StringPiece request_string(request.request_body->elements()
                                               ->at(0)
                                               .As<network::DataElementBytes>()
                                               .AsStringPiece());
          absl::optional<base::Value> request_value =
              base::JSONReader::Read(request_string);
          std::string* method = request_value->FindStringKey("method");
          ASSERT_TRUE(method);

          if (*method == "eth_estimateGas") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "\"0x00000000000009604\"}");
          } else if (*method == "eth_gasPrice") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "\"0x17fcf18321\"}");
          } else if (*method == "eth_getTransactionCount") {
            url_loader_factory_.AddResponse(
                request.url.spec(),
                "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
                "\"1\"}");
          }
        }));

    user_prefs::UserPrefs::Set(browser_context_.get(), &prefs_);
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    json_rpc_service_.reset(
        new JsonRpcService(shared_url_loader_factory_, &prefs_));
    keyring_service_.reset(new KeyringService(&prefs_));
    asset_ratio_service_.reset(
        new AssetRatioService(shared_url_loader_factory_));

    auto tx_state_manager =
        std::make_unique<EthTxStateManager>(&prefs_, json_rpc_service_.get());
    auto nonce_tracker = std::make_unique<FilNonceTracker>(
        tx_state_manager.get(), json_rpc_service_.get());
    auto pending_tx_tracker = std::make_unique<EthPendingTxTracker>(
        tx_state_manager.get(), json_rpc_service_.get(), nullptr);

    fil_tx_service_.reset(new FilTxService(
        json_rpc_service_.get(), keyring_service_.get(),
        asset_ratio_service_.get(), std::move(tx_state_manager),
        std::move(nonce_tracker), std::move(pending_tx_tracker), &prefs_));

    base::RunLoop run_loop;
    json_rpc_service_->SetNetwork(brave_wallet::mojom::kLocalhostChainId,
                                  base::BindLambdaForTesting([&](bool success) {
                                    EXPECT_TRUE(success);
                                    run_loop.Quit();
                                  }));
    run_loop.Run();
    keyring_service_->CreateWallet("testing123", base::DoNothing());
    base::RunLoop().RunUntilIdle();
    keyring_service_->AddAccount("Account 1", mojom::CoinType::ETH,
                                 base::DoNothing());
    base::RunLoop().RunUntilIdle();

    ASSERT_TRUE(base::HexStringToBytes(
        "095ea7b3000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
        "0f0000000000000000000000000000000000000000000000003fffffffffffffff",
        &data_));
  }

  std::string from() {
    return keyring_service_
        ->GetHDKeyringById(brave_wallet::mojom::kFilecoinKeyringId)
        ->GetAddress(0);
  }

  FilTxService* fil_tx_service() { return fil_tx_service_.get(); }

  PrefService* GetPrefs() { return &prefs_; }

  void SetInterceptor(const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, content](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), content);
        }));
  }

  void SetErrorInterceptor() {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), "",
                                          net::HTTP_REQUEST_TIMEOUT);
        }));
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<FilTxService> fil_tx_service_;
  std::unique_ptr<AssetRatioService> asset_ratio_service_;
  std::vector<uint8_t> data_;
};

TEST_F(FilTxServiceUnitTest,
       AddUnapprovedTransactionWithoutGasPriceAndGasLimit) {
  auto tx_data =
      mojom::TxData::New("0x06", "" /* gas_price */, "" /* gas_limit */,
                         "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c",
                         "0x016345785d8a0000", data_);
  bool callback_called = false;
  std::string tx_meta_id;

  fil_tx_service_->AddUnapprovedTransaction(
      tx_data.Clone(), from(),
      base::BindOnce(&AddUnapprovedTransactionSuccessCallback, &callback_called,
                     &tx_meta_id));

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
  auto tx_meta = fil_tx_service_->GetTxForTesting(tx_meta_id);
  EXPECT_TRUE(tx_meta);

  uint256_t gas_price_value;
  uint256_t gas_limit_value;
  EXPECT_TRUE(HexValueToUint256("0x17fcf18321", &gas_price_value));
  EXPECT_TRUE(HexValueToUint256("0x9604", &gas_limit_value));
  EXPECT_EQ(tx_meta->tx->gas_price(), gas_price_value);
  EXPECT_EQ(tx_meta->tx->gas_limit(), gas_limit_value);

  SetErrorInterceptor();
  callback_called = false;
  fil_tx_service_->AddUnapprovedTransaction(
      std::move(tx_data), from(),
      base::BindOnce(&AddUnapprovedTransactionFailureCallback,
                     &callback_called));
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(callback_called);
}

}  //  namespace brave_wallet
