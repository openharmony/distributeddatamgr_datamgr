/*
* Copyright (c) 2022 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "checker/checker_manager.h"
#include "utils/crypto.h"
#include "nativetoken_kit.h"
#include "accesstoken_kit.h"
#include <gtest/gtest.h>
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
class CheckerManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};
/**
* @tc.name: checkers
* @tc.desc: checker the bundle name of the system abilities.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(CheckerManagerTest, Checkers, TestSize.Level0)
{
    auto *checker = CheckerManager::GetInstance().GetChecker("SystemChecker");
    ASSERT_NE(checker, nullptr);
    checker = CheckerManager::GetInstance().GetChecker("BundleChecker");
    ASSERT_NE(checker, nullptr);
    checker = CheckerManager::GetInstance().GetChecker("OtherChecker");
    ASSERT_EQ(checker, nullptr);
}

/**
* @tc.name: SystemChecker bms
* @tc.desc: checker the bundle name of the system abilities.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(CheckerManagerTest, SystemCheckerBMS, TestSize.Level0)
{
    CheckerManager::StoreInfo info;
    info.uid = 1000;
    info.tokenId = GetAccessTokenId("foundation", nullptr, 0, "system_core");
    info.bundleName = "bundle_manager_service";
    ASSERT_EQ("bundle_manager_service", CheckerManager::GetInstance().GetAppId(info));
    ASSERT_TRUE(CheckerManager::GetInstance().IsValid(info));
}

/**
* @tc.name: SystemChecker form
* @tc.desc: checker the bundle name of the system abilities.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(CheckerManagerTest, SystemCheckerForm, TestSize.Level0)
{
    CheckerManager::StoreInfo info;
    info.uid = 1000;
    info.tokenId = GetAccessTokenId("foundation", nullptr, 0, "system_core");
    info.bundleName = "form_storage";
    ASSERT_EQ("form_storage", CheckerManager::GetInstance().GetAppId(info));
    ASSERT_TRUE(CheckerManager::GetInstance().IsValid(info));
}

/**
* @tc.name: SystemChecker ivi
* @tc.desc: checker the bundle name of the system abilities.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(CheckerManagerTest, SystemCheckerIVI, TestSize.Level0)
{
    CheckerManager::StoreInfo info;
    info.uid = 1000;
    info.tokenId = GetAccessTokenId("foundation", nullptr, 0, "system_core");
    info.bundleName = "ivi_config_manager";
    ASSERT_EQ("ivi_config_manager", CheckerManager::GetInstance().GetAppId(info));
    ASSERT_TRUE(CheckerManager::GetInstance().IsValid(info));
}

/**
* @tc.name: BundleChecker
* @tc.desc: checker the bundle name of the bundle abilities.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(CheckerManagerTest, BundleChecker, TestSize.Level0)
{
    CheckerManager::StoreInfo info;
    info.uid = 2000000;
    info.tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.test.demo", 0);
    info.bundleName = "ohos.test.demo";
    ASSERT_EQ(Crypto::Sha256("ohos.test.demo"), CheckerManager::GetInstance().GetAppId(info));
    ASSERT_TRUE(CheckerManager::GetInstance().IsValid(info));
}