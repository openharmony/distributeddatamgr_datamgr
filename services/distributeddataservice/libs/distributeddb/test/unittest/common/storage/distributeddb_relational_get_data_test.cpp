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
#ifdef RELATIONAL_STORE
#include <gtest/gtest.h>

#include "data_transformer.h"
#include "db_common.h"
#include "db_constant.h"
#include "db_errno.h"
#include "db_types.h"
#include "log_print.h"
#include "distributeddb_data_generate_unit_test.h"
#include "distributeddb_tools_unit_test.h"
#include "generic_single_ver_kv_entry.h"
#include "kvdb_properties.h"
#include "relational_store_delegate.h"
#include "relational_store_instance.h"
#include "relational_store_manager.h"
#include "relational_store_sqlite_ext.h"
#include "relational_sync_able_storage.h"
#include "sqlite_relational_store.h"
#include "sqlite_utils.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
string g_testDir;
string g_storePath;
const string g_tableName { "data" };
DistributedDB::RelationalStoreManager g_mgr(APP_ID, USER_ID);
RelationalStoreDelegate *g_delegate = nullptr;

void CreateDBAndTable()
{
    sqlite3 *db = nullptr;
    int errCode = sqlite3_open(g_storePath.c_str(), &db);
    if (errCode != SQLITE_OK) {
        LOGE("open db failed:%d", errCode);
        sqlite3_close(db);
        return;
    }

    const string sql { "CREATE TABLE " + g_tableName + "(key BLOB PRIMARY KEY NOT NULL, value BLOB);" };
    char *zErrMsg = nullptr;
    errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &zErrMsg);
    if (errCode != SQLITE_OK) {
        LOGE("sql error:%s",zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sqlite3_close(db);
}

int InitRelationalStore()
{
    RelationalStoreDelegate *delegate = nullptr;
    DBStatus dbStatus = DBStatus::DB_ERROR;
    g_mgr.OpenStore(g_storePath, RelationalStoreDelegate::Option {},
        [&delegate, &dbStatus] (DBStatus status, RelationalStoreDelegate *ptr) {
            delegate = ptr;
            dbStatus = status;
        }
    );
    if (dbStatus == DBStatus::OK) {
        g_delegate = delegate;
        g_delegate->CreateDistributedTable(g_tableName, RelationalStoreDelegate::TableOption());
    }
    return dbStatus;
}

int AddOrUpdateRecord(const Key &key, const Value &value) {
    static const string SQL = "INSERT OR REPLACE INTO " + g_tableName + " VALUES(?,?);";
    sqlite3 *db = nullptr;
    sqlite3_stmt *statement = nullptr;
    int errCode = sqlite3_open(g_storePath.c_str(), &db);
    if (errCode != SQLITE_OK) {
        LOGE("open db failed:%d", errCode);
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
        goto END;
    }
    errCode = SQLiteUtils::GetStatement(db, SQL, statement);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, key, false); // 1 means key's index
    if (errCode != E_OK) {
        goto END;
    }
    errCode = SQLiteUtils::BindBlobToStatement(statement, 2, value, true); // 2 means value's index
    if (errCode != E_OK) {
        goto END;
    }
    errCode = SQLiteUtils::StepWithRetry(statement, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    }

END:
    SQLiteUtils::ResetStatement(statement, true, errCode);
    sqlite3_close(db);
    return errCode;
}

int GetLogData(const Key &key, uint64_t &flag, TimeStamp &timestamp, const DeviceID &device = "")
{
    string tableName = g_tableName;
    if (!device.empty()) {
    }
    const string sql = "SELECT timestamp, flag \
        FROM " + g_tableName + " as a, " + DBConstant::RELATIONAL_PREFIX + g_tableName + "_log as b \
        WHERE a.key=? AND a.rowid=b.data_key;";

    sqlite3 *db = nullptr;
    sqlite3_stmt *statement = nullptr;
    int errCode = sqlite3_open(g_storePath.c_str(), &db);
    if (errCode != SQLITE_OK) {
        LOGE("open db failed:%d", errCode);
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
        goto END;
    }
    errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, key, false); // 1 means key's index
    if (errCode != E_OK) {
        goto END;
    }
    errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = SQLiteUtils::StepWithRetry(statement, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        timestamp = static_cast<TimeStamp>(sqlite3_column_int64(statement, 0));
        flag = static_cast<TimeStamp>(sqlite3_column_int64(statement, 1));
        errCode = E_OK;
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = -E_NOT_FOUND;
    }

END:
    sqlite3_close(db);
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}

void InitStoreProp(const RelationalStoreDelegate::Option &option, const std::string &storePath,
    const std::string appId, const std::string &userId, DBProperties &properties)
{
    properties.SetBoolProp(KvDBProperties::CREATE_IF_NECESSARY, option.createIfNecessary);
    properties.SetStringProp(KvDBProperties::DATA_DIR, storePath);
    properties.SetStringProp(KvDBProperties::APP_ID, appId);
    properties.SetStringProp(KvDBProperties::USER_ID, userId);
    properties.SetStringProp(KvDBProperties::STORE_ID, storePath);
    std::string identifier = userId + "-" + appId + "-" + storePath;
    std::string hashIdentifier = DBCommon::TransferHashString(identifier);
    properties.SetStringProp(KvDBProperties::IDENTIFIER_DATA, hashIdentifier);
}

const RelationalSyncAbleStorage *GetRelationalStore()
{
    DBProperties properties;
    InitStoreProp(RelationalStoreDelegate::Option {}, g_storePath, APP_ID, USER_ID, properties);
    int errCode = E_OK;
    auto store = RelationalStoreInstance::GetDataBase(properties, errCode);
    if (store == nullptr) {
        LOGE("Get db failed:%d", errCode);
        return nullptr;
    }
    return static_cast<SQLiteRelationalStore *>(store)->GetStorageEngine();
}
}

class DistributedDBRelationalGetDataTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBRelationalGetDataTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    g_storePath = g_testDir + "/getDataTest.db";
    LOGI("The test db is:%s", g_testDir.c_str());
}

void DistributedDBRelationalGetDataTest::TearDownTestCase(void)
{}

void DistributedDBRelationalGetDataTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    CreateDBAndTable();
}

void DistributedDBRelationalGetDataTest::TearDown(void)
{
    if (g_delegate != nullptr) {
        EXPECT_EQ(g_mgr.CloseStore(g_delegate), DBStatus::OK);
        EXPECT_EQ(g_mgr.DeleteStore(g_storePath), DBStatus::OK);
        g_delegate = nullptr;
    }
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
    return;
}

/**
 * @tc.name: LogTbl1
 * @tc.desc: When put sync data to relational store, trigger generate log.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalGetDataTest, LogTbl1, TestSize.Level1)
{
    ASSERT_EQ(InitRelationalStore(), DBStatus::OK);

    /**
     * @tc.steps: step1. Put data.
     * @tc.expected: Succeed, return OK.
     */
    Key insertKey = KEY_1;
    Value insertValue = VALUE_1;
    EXPECT_EQ(AddOrUpdateRecord(insertKey, insertValue), E_OK);

    /**
     * @tc.steps: step2. Check log record.
     * @tc.expected: Record exists.
     */
    Value value;
    uint64_t flag = 0;
    TimeStamp timestamp1 = 0;
    EXPECT_EQ(GetLogData(insertKey, flag, timestamp1), E_OK);
    EXPECT_EQ(flag, DataItem::LOCAL_FLAG);
    EXPECT_NE(timestamp1, 0ULL);
}

/**
 * @tc.name: GetSyncData1
 * @tc.desc: GetSyncData interface
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
  */
HWTEST_F(DistributedDBRelationalGetDataTest, GetSyncData1, TestSize.Level1)
{
    ASSERT_EQ(InitRelationalStore(), DBStatus::OK);

    /**
     * @tc.steps: step1. Put 500 records.
     * @tc.expected: Succeed, return OK.
     */
    const int RECORD_COUNT = 500;
    for (int i = 0; i < RECORD_COUNT; ++i) {
        string key = "key_" + to_string(i);
        string value = "value_" + to_string(i);
        EXPECT_EQ(AddOrUpdateRecord(Key(key.data(), key.data() + key.size()),
                                    Value(value.data(), value.data() + value.size())), E_OK);
    }

    /**
     * @tc.steps: step2. Get all data.
     * @tc.expected: Succeed and the count is right.
     */
    auto store = GetRelationalStore();
    ContinueToken token = nullptr;
    QueryObject query(Query::Select(g_tableName));
    std::vector<SingleVerKvEntry *> entries;
    DataSizeSpecInfo sizeInfo {MTU_SIZE, 50};

    int errCode = store->GetSyncData(query, SyncTimeRange {}, sizeInfo, token, entries);
    int count = entries.size();
    SingleVerKvEntry::Release(entries);
    EXPECT_EQ(errCode, -E_UNFINISHED);
    while (token != nullptr) {
        errCode = store->GetSyncDataNext(entries, token, sizeInfo);
        count += entries.size();
        SingleVerKvEntry::Release(entries);
        EXPECT_TRUE(errCode == E_OK || errCode == -E_UNFINISHED);
    }
    EXPECT_EQ(count, RECORD_COUNT);
}

/**
 * @tc.name: GetQuerySyncData1
 * @tc.desc: GetSyncData interface.
 * @tc.type: FUNC
 * @tc.require: AR000GK58H
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalGetDataTest, GetQuerySyncData1, TestSize.Level1)
{
    ASSERT_EQ(InitRelationalStore(), DBStatus::OK);

    /**
     * @tc.steps: step1. Put 100 records.
     * @tc.expected: Succeed, return OK.
     */
    const int RECORD_COUNT = 100; // 100 records.
    for (int i = 0; i < RECORD_COUNT; ++i) {
        string key = "k" + to_string(i);
        string value = "v" + to_string(i);
        EXPECT_EQ(AddOrUpdateRecord(Key(key.data(), key.data() + key.size()),
                                    Value(value.data(), value.data() + value.size())), E_OK);
    }

    /**
     * @tc.steps: step2. Get data limit 80, offset 30.
     * @tc.expected: Get 70 records.
     */
    auto store = GetRelationalStore();
    ContinueToken token = nullptr;
    const unsigned int LIMIT = 80; // limit as 80.
    const unsigned int OFFSET = 30; // offset as 30.
    const unsigned int EXPECT_COUNT = RECORD_COUNT - OFFSET; // expect 70 records.
    QueryObject query(Query::Select(g_tableName).Limit(LIMIT, OFFSET));
    std::vector<SingleVerKvEntry *> entries;

    int errCode = store->GetSyncData(query, SyncTimeRange {}, DataSizeSpecInfo {}, token, entries);
    EXPECT_EQ(entries.size(), EXPECT_COUNT);
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(token, nullptr);
    SingleVerKvEntry::Release(entries);
}
#endif