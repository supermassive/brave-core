/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/fil_address.h"

#include "base/check_op.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace {
#define ADDRESS_LEN_SECP256K 41
#define ADDRESS_LEN_BLS 86
}  // namespace

FilAddress::FilAddress(const std::vector<uint8_t>& bytes) : bytes_(bytes) {}
FilAddress::FilAddress() = default;
FilAddress::FilAddress(const FilAddress& other) = default;
FilAddress::~FilAddress() {}

bool FilAddress::operator==(const FilAddress& other) const {
  return std::equal(bytes_.begin(), bytes_.end(), other.bytes_.begin());
}

bool FilAddress::operator!=(const FilAddress& other) const {
  return !std::equal(bytes_.begin(), bytes_.end(), other.bytes_.begin());
}

// static
FilAddress FilAddress::FromPublicKey(const std::vector<uint8_t>& public_key) {
  if (public_key.size() != 64) {
    VLOG(1) << __func__ << ": public key size should be 64 bytes";
    return FilAddress();
  }

  return FilAddress(public_key);
}

// static
bool FilAddress::IsValidAddress(const std::string& input) {
  bool secp_address = input.size() == ADDRESS_LEN_SECP256K;
  bool bls_address = input.size() == ADDRESS_LEN_BLS;
  if (!secp_address && !bls_address) {
    VLOG(1) << __func__ << ": input should be " << ADDRESS_LEN_SECP256K
            << " or " << ADDRESS_LEN_BLS << " bytes long";
    return false;
  }
  bool valid_network = base::StartsWith(input, mojom::kFilecoinTestnet) ||
                       base::StartsWith(input, mojom::kFilecoinMainnet);
  auto protocol = secp_address ? mojom::FilecoinAddressProtocol::SECP256K1
                               : mojom::FilecoinAddressProtocol::BLS;
  std::string current{input[1]};
  bool valid_protocol = current == std::to_string(static_cast<int>(protocol));
  return valid_network && valid_protocol;
}

std::string FilAddress::ToHex() const {
  const std::string input(bytes_.begin(), bytes_.end());
  return ::brave_wallet::ToHex(input);
}

std::string FilAddress::ToChecksumAddress(uint256_t eip1191_chaincode) const {
  std::string result;
  /*
  std::string input;

  if (eip1191_chaincode == static_cast<uint256_t>(30) ||
      eip1191_chaincode == static_cast<uint256_t>(31)) {
    // TODO(jocelyn): We will need to revise this if there are supported chains
    // with ID larger than uint64_t.
    input +=
        base::NumberToString(static_cast<uint64_t>(eip1191_chaincode)) + "0x";
  }

  input += std::string(ToHex().data() + 2);

  const std::string hash_str(KeccakHash(input).data() + 2);
  const std::string address_str =
      base::ToLowerASCII(base::HexEncode(bytes_.data(), bytes_.size()));

  for (size_t i = 0; i < address_str.length(); ++i) {
    if (isdigit(address_str[i])) {
      result.push_back(address_str[i]);
    } else {  // address has already be validated
      int nibble = std::stoi(std::string(1, hash_str[i]), nullptr, 16);
      if (nibble > 7) {
        result.push_back(base::ToUpperASCII(address_str[i]));
      } else {
        result.push_back(address_str[i]);
      }
    }
  }*/
  return result;
}

}  // namespace brave_wallet
