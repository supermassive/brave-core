/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/solana_account_meta.h"

namespace brave_wallet {

class SolanaInstruction {
 public:
  SolanaInstruction(const std::string& program_id,
                    const std::vector<SolanaAccountMeta>& accounts,
                    const std::vector<uint8_t>& data);
  ~SolanaInstruction();

  SolanaInstruction(const SolanaInstruction&);

  bool Serialize(const std::vector<SolanaAccountMeta>& account_metas,
                 std::vector<uint8_t>* bytes) const;

  const std::vector<SolanaAccountMeta>& GetAccounts() const {
    return accounts_;
  }
  const std::string& GetProgramId() const { return program_id_; }

 private:
  std::string program_id_;
  std::vector<SolanaAccountMeta> accounts_;
  std::vector<uint8_t> data_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_INSTRUCTION_H_
