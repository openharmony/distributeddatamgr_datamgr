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
#include <gtest/gtest.h>

#include <condition_variable>
#include <vector>

#include "kvdb_service_client.h"
#include "types.h"
using namespace testing::ext;
using namespace OHOS::DistributedKv;
class SingleStoreImplTest : public testing::Test {
public:
    class TestObserver : public KvStoreObserver {
    public:
        bool IsChanged()
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return isChanged_; });
            bool current = isChanged_;
            isChanged_ = false;
            cv_.notify_one();
            return current;
        }

        void OnChange(const ChangeNotification &notification) override
        {
            insert_ = notification.GetInsertEntries();
            update_ = notification.GetUpdateEntries();
            delete_ = notification.GetDeleteEntries();
            deviceId_ = notification.GetDeviceId();
            {
                std::lock_guard<std::mutex> lock(mutex_);
                isChanged_ = true;
                cv_.notify_one();
            }
        }
        std::vector<Entry> insert_;
        std::vector<Entry> update_;
        std::vector<Entry> delete_;
        std::string deviceId_;

    private:
        std::mutex mutex_;
        std::condition_variable cv_;
        bool isChanged_ = false;
    };

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    std::shared_ptr<SingleKvStore> kvStore_;
};

void SingleStoreImplTest::SetUpTestCase(void)
{
}

void SingleStoreImplTest::TearDownTestCase(void)
{
}

void SingleStoreImplTest::SetUp(void)
{
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S1;
    AppId appId = { "LocalSingleKVStore" };
    StoreId storeId = { "LocalSingleKVStore" };
    std::string path = "";
    Status status = KVDBServiceClient::GetInstance()->Delete(appId, storeId, path);
    kvStore_ = KVDBServiceClient::GetInstance()->GetKVStore(appId, storeId, options, path, status);
    ASSERT_EQ(status, SUCCESS);
}

void SingleStoreImplTest::TearDown(void)
{
    AppId appId = { "LocalSingleKVStore" };
    StoreId storeId = { "LocalSingleKVStore" };
    std::string path = "";
    Status status = KVDBServiceClient::GetInstance()->Delete(appId, storeId, path);
    ASSERT_EQ(status, SUCCESS);
}

/**
* @tc.name: GetStoreId
* @tc.desc: get the store id of the kv store
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Sven Wang
*/
HWTEST_F(SingleStoreImplTest, GetStoreId, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto storeId = kvStore_->GetStoreId();
    ASSERT_EQ(storeId.storeId, "LocalSingleKVStore");
}

/**
* @tc.name: Put
* @tc.desc: put key-value data to the kv store
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Sven Wang
*/
HWTEST_F(SingleStoreImplTest, Put, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto status = kvStore_->Put({ "Put Test" }, { "Put Value" });
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->Put({ "   Put Test" }, { "Put2 Value" });
    ASSERT_EQ(status, SUCCESS);
    Value value;
    status = kvStore_->Get({ "Put Test" }, value);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(value.ToString(), "Put2 Value");
}

/**
* @tc.name: PutBatch
* @tc.desc: put some key-value data to the kv store
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Sven Wang
*/
HWTEST_F(SingleStoreImplTest, PutBatch, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::vector<Entry> entries;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        entries.push_back(entry);
    }
    auto status = kvStore_->PutBatch(entries);
    ASSERT_EQ(status, SUCCESS);
}

/**
* @tc.name: Delete
* @tc.desc: delete the value of the key
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Sven Wang
*/
HWTEST_F(SingleStoreImplTest, Delete, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto status = kvStore_->Put({ "Put Test" }, { "Put Value" });
    ASSERT_EQ(status, SUCCESS);
    Value value;
    status = kvStore_->Get({ "Put Test" }, value);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(std::string("Put Value"), value.ToString());
    status = kvStore_->Delete({ "Put Test" });
    ASSERT_EQ(status, SUCCESS);
    value = {};
    status = kvStore_->Get({ "Put Test" }, value);
    ASSERT_EQ(status, KEY_NOT_FOUND);
    ASSERT_EQ(std::string(""), value.ToString());
}

/**
* @tc.name: DeleteBatch
* @tc.desc: delete the values of the keys
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Sven Wang
*/
HWTEST_F(SingleStoreImplTest, DeleteBatch, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::vector<Entry> entries;
    for (int i = 0; i < 10; ++i) {
        Entry entry;
        entry.key = std::to_string(i).append("_k");
        entry.value = std::to_string(i).append("_v");
        entries.push_back(entry);
    }
    auto status = kvStore_->PutBatch(entries);
    ASSERT_EQ(status, SUCCESS);
    std::vector<Key> keys;
    for (int i = 0; i < 10; ++i) {
        Key key = std::to_string(i).append("_k");
        keys.push_back(key);
    }
    status = kvStore_->DeleteBatch(keys);
    ASSERT_EQ(status, SUCCESS);
    for (int i = 0; i < 10; ++i) {
        Value value;
        status = kvStore_->Get(keys[i], value);
        ASSERT_EQ(status, KEY_NOT_FOUND);
        ASSERT_EQ(value.ToString(), std::string(""));
    }
}

/**
* @tc.name: Transaction
* @tc.desc: do transaction
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Sven Wang
*/
HWTEST_F(SingleStoreImplTest, Transaction, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto status = kvStore_->StartTransaction();
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->Commit();
    ASSERT_EQ(status, SUCCESS);

    status = kvStore_->StartTransaction();
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->Rollback();
    ASSERT_EQ(status, SUCCESS);
}

/**
* @tc.name: SubscribeKvStore
* @tc.desc: subscribe local
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Sven Wang
*/
HWTEST_F(SingleStoreImplTest, SubscribeKvStore, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto observer = std::make_shared<TestObserver>();
    auto status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(status, STORE_ALREADY_SUBSCRIBE);
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    ASSERT_EQ(status, STORE_ALREADY_SUBSCRIBE);
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(status, STORE_ALREADY_SUBSCRIBE);
    status = kvStore_->SubscribeKvStore(DEFAULT, observer);
    ASSERT_EQ(status, STORE_ALREADY_SUBSCRIBE);
    status = kvStore_->Put({ "Put Test" }, { "Put Value" });
    ASSERT_EQ(status, SUCCESS);
    ASSERT_TRUE(observer->IsChanged());
    ASSERT_EQ(observer->insert_.size(), 1);
    ASSERT_EQ(observer->update_.size(), 0);
    ASSERT_EQ(observer->delete_.size(), 0);
    status = kvStore_->Put({ "Put Test" }, { "Put Value1" });
    ASSERT_EQ(status, SUCCESS);
    ASSERT_TRUE(observer->IsChanged());
    ASSERT_EQ(observer->insert_.size(), 0);
    ASSERT_EQ(observer->update_.size(), 1);
    ASSERT_EQ(observer->delete_.size(), 0);
    status = kvStore_->Delete({ "Put Test" });
    ASSERT_EQ(status, SUCCESS);
    ASSERT_TRUE(observer->IsChanged());
    ASSERT_EQ(observer->insert_.size(), 0);
    ASSERT_EQ(observer->update_.size(), 0);
    ASSERT_EQ(observer->delete_.size(), 1);
}

/**
* @tc.name: UnsubscribeKvStore
* @tc.desc: unsubscribe
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Sven Wang
*/
HWTEST_F(SingleStoreImplTest, UnsubscribeKvStore, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    auto observer = std::make_shared<TestObserver>();
    auto status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->UnSubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->UnSubscribeKvStore(SUBSCRIBE_TYPE_REMOTE, observer);
    ASSERT_EQ(status, STORE_NOT_SUBSCRIBE);
    status = kvStore_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->UnSubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(status, STORE_NOT_SUBSCRIBE);
    status = kvStore_->UnSubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(status, STORE_NOT_SUBSCRIBE);
    status = kvStore_->SubscribeKvStore(SUBSCRIBE_TYPE_LOCAL, observer);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore_->UnSubscribeKvStore(SUBSCRIBE_TYPE_ALL, observer);
    ASSERT_EQ(status, SUCCESS);
}

/**
* @tc.name: GetEntries
* @tc.desc: unsubscribe
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Sven Wang
*/
HWTEST_F(SingleStoreImplTest, GetEntries_Prefix, TestSize.Level0)
{
    ASSERT_NE(kvStore_, nullptr);
    std::vector<Entry> entries;
    auto status = kvStore_->GetEntries({ "" }, entries);
    ASSERT_EQ(status, SUCCESS);
}