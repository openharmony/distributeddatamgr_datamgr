/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <condition_variable>
#include <gtest/gtest.h>
#include <thread>

#include "db_constant.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "kv_store_nb_delegate.h"
#include "kv_virtual_device.h"
#include "platform_specific.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
    string g_testDir;
    VirtualCommunicatorAggregator* g_communicatorAggregator = nullptr;
}

class DistributedDBSingleVerDLPTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBSingleVerDLPTest::SetUpTestCase(void)
{
    /**
     * @tc.setup: Init datadir and Virtual Communicator.
     */
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);

    g_communicatorAggregator = new (std::nothrow) VirtualCommunicatorAggregator();
    ASSERT_TRUE(g_communicatorAggregator != nullptr);
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(g_communicatorAggregator);
}

void DistributedDBSingleVerDLPTest::TearDownTestCase(void)
{
    /**
     * @tc.teardown: Release virtual Communicator and clear data dir.
     */
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error!");
    }
    RuntimeContext::GetInstance()->SetCommunicatorAggregator(nullptr);
}

void DistributedDBSingleVerDLPTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
}

void DistributedDBSingleVerDLPTest::TearDown(void)
{
}

/**
 * @tc.name: SameDelegateTest001
 * @tc.desc: Test kv delegate open with diff instanceID.
 * @tc.type: FUNC
 * @tc.require: SR000H0JSC
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBSingleVerDLPTest, SameDelegateTest001, TestSize.Level1)
{
    KvStoreDelegateManager mgr1(APP_ID, USER_ID, INSTANCE_ID_1);
    std::string dbPath = g_testDir + "/dlp1";
    KvStoreConfig storeConfig;
    storeConfig.dataDir = dbPath;
    mgr1.SetKvStoreConfig(storeConfig);

    string dir = dbPath + "/single_ver";
    DIR* dirTmp = opendir(dir.c_str());
    if (dirTmp == nullptr) {
        OS::MakeDBDirectory(dir);
    } else {
        closedir(dirTmp);
    }

    KvStoreNbDelegate::Option option;
    KvStoreNbDelegate *delegatePtr = nullptr;
    mgr1.GetKvStore(STORE_ID_1, option, [&delegatePtr](DBStatus status, KvStoreNbDelegate *delegate) {
        delegatePtr = delegate;
    });
}