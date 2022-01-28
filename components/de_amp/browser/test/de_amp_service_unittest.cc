/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/de_amp/browser/de_amp_service.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace de_amp {

class DeAmpServiceUnitTest : public testing::Test {
 public:
  DeAmpServiceUnitTest() {}
  ~DeAmpServiceUnitTest() override = default;

  void SetUp() override {
    pref_service_ = std::make_unique<TestingPrefServiceSimple>();
    DeAmpService::RegisterProfilePrefs(pref_service_->registry());
    service_ = std::make_unique<DeAmpService>(pref_service_.get());
  }

  bool CheckIfAmpDetected(const std::string body, std::string* canonical_link) {
    return service_->FindCanonicalLinkIfAMP(body, canonical_link);
  }

  void CheckFindCanonicalLinkResult(const std::string expected_link,
                                    const std::string body,
                                    const bool expected_detect_amp) {
    std::string actual_link;
    const bool actual_detect_amp = CheckIfAmpDetected(body, &actual_link);
    EXPECT_EQ(expected_detect_amp, actual_detect_amp);
    if (expected_detect_amp) {
      EXPECT_EQ(expected_link, actual_link);
    }
  }

  void CheckCheckCanonicalLinkResult(const std::string canonical_link,
                                     const std::string original,
                                     const bool expected) {
    GURL canonical_url(canonical_link), original_url(original);
    EXPECT_EQ(expected,
              service_->VerifyCanonicalLink(canonical_url, original_url));
  }

 protected:
  std::unique_ptr<TestingPrefServiceSimple> pref_service_;
  std::unique_ptr<de_amp::DeAmpService> service_;
};

TEST_F(DeAmpServiceUnitTest, DetectAmpWithEmoji) {
  const std::string body =
      "<html ⚡><head><link rel=\"canonical\" "
      "href=\"abc\"/></head><body></body></html>";
  CheckFindCanonicalLinkResult("abc", body, true);
}

TEST_F(DeAmpServiceUnitTest, DetectAmpWithWordAmp) {
  const std::string body =
      "<html amp>\n<head><link rel=\"author\" href=\"xyz\"/>\n<link "
      "rel=\"canonical\" href=\"abc\"/></head><body></body></html>";
  CheckFindCanonicalLinkResult("abc", body, true);
}

TEST_F(DeAmpServiceUnitTest, DetectAmpWithWordAmpNotAtEnd) {
  const std::string body =
      "<html amp xyzzy>\n<head><link rel=\"author\" href=\"xyz\"/>\n<link "
      "rel=\"canonical\" href=\"abc\"/></head><body></body></html>";
  CheckFindCanonicalLinkResult("abc", body, true);
}

TEST_F(DeAmpServiceUnitTest, DetectAmpMixedCase) {
  const std::string body =
      "<DOCTYPE! html><html AmP xyzzy>\n<head><link rel=\"author\" "
      "href=\"xyz\"/>\n<link rel=\"canonical\" "
      "href=\"abc\"/></head><body></body></html>";
  CheckFindCanonicalLinkResult("abc", body, true);
}

TEST_F(DeAmpServiceUnitTest, NegativeDetectAmp) {
  // Put AMP attribute in a different tag than html
  const std::string body =
      "<html xyzzy>\n<head><link amp rel=\"author\" href=\"xyz\"/>\n<link "
      "rel=\"canonical\" href=\"abc\"/></head><body></body></html>";
  CheckFindCanonicalLinkResult("", body, false);
}

TEST_F(DeAmpServiceUnitTest, DetectAmpButNoCanonicalLink) {
  // Put AMP attribute in a different tag than html
  const std::string body =
      "<html xyzzy>\n<head><link amp rel=\"author\" href=\"xyz\"/>\n<link "
      "rel=\"canonical\" href=\"abc\"/></head><body></body></html>";
  CheckFindCanonicalLinkResult("", body, false);
}

TEST_F(DeAmpServiceUnitTest, MalformedHtmlDoc) {
  const std::string body =
      "<xyz html amp xyzzy>\n<head><link amp rel=\"author\" "
      "href=\"xyz\"/>\n<link rel=\"canonical\" "
      "href=\"abc\"/></head><body></body></html>";
  CheckFindCanonicalLinkResult("", body, false);
}

TEST_F(DeAmpServiceUnitTest, LinkRelNotInSameTag) {
  // Checking to make sure a random "canonical" does not confused parser
  const std::string body =
      "<html amp>\n<head><link rel=\"author\" href=\"xyz\"/>\n<body> "
      "\"canonical\"> href=\"abc\"/></head><body></body></html>";
  CheckFindCanonicalLinkResult("", body, false);
}

TEST_F(DeAmpServiceUnitTest, SingleQuotes) {
  const std::string body =
      "<DOCTYPE! html><html AMP xyzzy>\n<head><link rel='author' "
      "href='xyz'/>\n<link rel='canonical' "
      "href='abc'></head><body></body></html>";
  CheckFindCanonicalLinkResult("abc", body, true);
}

TEST_F(DeAmpServiceUnitTest, CanonicalLinkMalformed) {
  CheckCheckCanonicalLinkResult("xyz.com", "https://amp.xyz.com", false);
}

TEST_F(DeAmpServiceUnitTest, CanonicalLinkCorrect) {
  CheckCheckCanonicalLinkResult("https://xyz.com", "https://amp.xyz.com", true);
}

TEST_F(DeAmpServiceUnitTest, CanonicalLinkSameAsOriginal) {
  CheckCheckCanonicalLinkResult("https://amp.xyz.com", "https://amp.xyz.com",
                                false);
}

TEST_F(DeAmpServiceUnitTest, CanonicalLinkNotHttp) {
  CheckCheckCanonicalLinkResult("ftp://xyz.com", "https://amp.xyz.com", false);
}

}  // namespace de_amp
