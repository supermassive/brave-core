/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_transaction.h"

#include <memory>

#include "base/base64.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/solana_account_meta.h"
#include "brave/components/brave_wallet/browser/solana_instruction.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

const char kMnemonic[] =
    "divide cruise upon flag harsh carbon filter merit once advice bright "
    "drive";

}  // namespace

class SolanaTransactionUnitTest : public testing::Test {
 public:
  SolanaTransactionUnitTest()
      : browser_context_(new content::TestBrowserContext()) {
    user_prefs::UserPrefs::Set(browser_context_.get(), &prefs_);
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    keyring_service_.reset(new KeyringService(&prefs_));
  }

  ~SolanaTransactionUnitTest() override = default;

  KeyringService* keyring_service() { return keyring_service_.get(); }

 private:
  content::BrowserTaskEnvironment browser_task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<KeyringService> keyring_service_;
};

TEST_F(SolanaTransactionUnitTest, GetSignedTransaction) {
  keyring_service()->RestoreWallet(kMnemonic, "brave", false,
                                   base::DoNothing());
  base::RunLoop().RunUntilIdle();
  keyring_service()->AddAccount("Account 1", mojom::CoinType::SOL,
                                base::DoNothing());
  keyring_service()->AddAccount("Account 2", mojom::CoinType::SOL,
                                base::DoNothing());

  std::string from_account = "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";
  std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
  std::string recent_blockhash = "9sHcv6xwn9YkB8nxTUGKDwPwNnmqVp5oAXxU8Fdkm4J6";

  SolanaInstruction instruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, false, true)},
      // Data
      {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
  SolanaTransaction transaction(recent_blockhash, from_account, {instruction});

  std::vector<uint8_t> expected_bytes = {
      // Signature compact array
      1,  // num of signatures
      // signature byte array
      75, 128, 163, 160, 211, 120, 107, 238, 150, 64, 174, 92, 74, 8, 2, 134,
      237, 91, 221, 42, 91, 63, 197, 4, 181, 31, 247, 134, 38, 14, 244, 79, 131,
      43, 237, 96, 10, 161, 181, 22, 196, 36, 116, 73, 185, 231, 151, 96, 119,
      245, 215, 94, 171, 25, 159, 127, 33, 119, 208, 242, 179, 63, 36, 15,
      // Message header
      1,  // num_required_signatures
      0,  // num_readonly_signed_accounts
      1,  // num_readonly_unsigned_accounts

      // Account addresses compact array
      3,  // account addresses array length
      // account_addresses[0]: base58-decoded from account
      161, 51, 89, 91, 115, 210, 217, 212, 76, 159, 171, 200, 40, 150, 157, 70,
      197, 71, 24, 44, 209, 108, 143, 4, 58, 251, 215, 62, 201, 172, 159, 197,
      // account_addresses[1]: base58-decoded to account
      255, 224, 228, 245, 94, 238, 23, 132, 206, 40, 82, 249, 219, 203, 103,
      158, 110, 219, 93, 249, 143, 134, 207, 172, 179, 76, 67, 6, 169, 164, 149,
      38,
      // account_addresses[2]: base58-decoded program ID in the instruction
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0,

      // Recent blockhash, base58-decoded
      131, 191, 83, 201, 108, 193, 222, 255, 176, 67, 136, 209, 219, 42, 6, 169,
      240, 137, 142, 185, 169, 6, 17, 87, 123, 6, 42, 55, 162, 64, 120, 91,

      // Instructions compact array
      1,                                        // instructions array length
      2,                                        // program id index
      2,                                        // length of accounts
      0, 1,                                     // account indices
      12,                                       // data length
      2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0  // data
  };
  std::string expected_tx = base::Base64Encode(expected_bytes);
  EXPECT_EQ(transaction.GetSignedTransaction(keyring_service(), ""),
            expected_tx);

  // Test two signers.
  instruction = SolanaInstruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, true, true)},
      // Data
      {2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0});
  transaction =
      SolanaTransaction(recent_blockhash, from_account, {instruction});

  expected_bytes = std::vector<uint8_t>({
      // Signature compact array
      2,  // num of signatures
      // first account's signature
      204, 127, 175, 133, 20, 97, 41, 39, 106, 79, 38, 41, 221, 89, 38, 223,
      218, 63, 117, 68, 237, 45, 169, 94, 53, 56, 233, 159, 107, 110, 171, 152,
      241, 104, 11, 121, 164, 73, 210, 252, 42, 235, 214, 82, 107, 225, 218, 70,
      128, 175, 10, 17, 45, 190, 13, 100, 169, 164, 104, 207, 112, 145, 133, 2,
      // second account's signature
      54, 115, 88, 109, 108, 123, 97, 39, 185, 100, 244, 248, 224, 182, 51, 40,
      54, 151, 223, 15, 86, 126, 161, 53, 72, 107, 159, 23, 72, 82, 18, 31, 99,
      52, 175, 135, 38, 202, 71, 215, 64, 171, 122, 99, 178, 217, 144, 109, 88,
      75, 198, 137, 92, 222, 109, 229, 52, 138, 101, 182, 42, 134, 216, 4,
      // Message header
      2,  // num_required_signatures
      0,  // num_readonly_signed_accounts
      1,  // num_readonly_unsigned_accounts

      // Account addresses compact array
      3,  // account addresses array length
      // account_addresses[0]: base58-decoded from account
      161, 51, 89, 91, 115, 210, 217, 212, 76, 159, 171, 200, 40, 150, 157, 70,
      197, 71, 24, 44, 209, 108, 143, 4, 58, 251, 215, 62, 201, 172, 159, 197,
      // account_addresses[1]: base58-decoded to account
      255, 224, 228, 245, 94, 238, 23, 132, 206, 40, 82, 249, 219, 203, 103,
      158, 110, 219, 93, 249, 143, 134, 207, 172, 179, 76, 67, 6, 169, 164, 149,
      38,
      // account_addresses[2]: base58-decoded program ID in the instruction
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0,

      // Recent blockhash, base58-decoded
      131, 191, 83, 201, 108, 193, 222, 255, 176, 67, 136, 209, 219, 42, 6, 169,
      240, 137, 142, 185, 169, 6, 17, 87, 123, 6, 42, 55, 162, 64, 120, 91,

      // Instructions compact array
      1,                                        // instructions array length
      2,                                        // program id index
      2,                                        // length of accounts
      0, 1,                                     // account indices
      12,                                       // data length
      2, 0, 0, 0, 128, 150, 152, 0, 0, 0, 0, 0  // data
  });
  expected_tx = base::Base64Encode(expected_bytes);
  EXPECT_EQ(transaction.GetSignedTransaction(keyring_service(), ""),
            expected_tx);

  // Test key_service is nullptr.
  EXPECT_TRUE(transaction.GetSignedTransaction(nullptr, "").empty());

  std::vector<uint8_t> oversized_data(1232, 1);
  instruction = SolanaInstruction(
      // Program ID
      kSolanaSystemProgramId,
      // Accounts
      {SolanaAccountMeta(from_account, true, true),
       SolanaAccountMeta(to_account, false, true)},
      oversized_data);
  transaction =
      SolanaTransaction(recent_blockhash, from_account, {instruction});
  EXPECT_TRUE(transaction.GetSignedTransaction(keyring_service(), "").empty());
}

}  // namespace brave_wallet
