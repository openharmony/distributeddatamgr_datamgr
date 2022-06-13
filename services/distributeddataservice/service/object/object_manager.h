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

#ifndef DISTRIBUTEDDATAMGR_OBJECT_MANAGER_H
#define DISTRIBUTEDDATAMGR_OBJECT_MANAGER_H

#include <atomic>

#include "communication_provider.h"
#include "iobject_callback.h"
#include "kvstore_sync_callback.h"
#include "types.h"
#include "kv_store_delegate_manager.h"
#include "object_common.h"

namespace OHOS {
namespace DistributedObject {
using SyncCallBack = std::function<void(const std::map<std::string, int32_t> &results)>;

class SequenceSyncManager {
public:
    enum Result {
        SUCCESS_USER_IN_USE,
        SUCCESS_USER_HAS_FINISHED,
        ERR_SID_NOT_EXIST
    };
    static SequenceSyncManager *GetInstance()
    {
        static SequenceSyncManager sequenceSyncManager;
        return &sequenceSyncManager;
    }

    uint64_t AddNotifier(const std::string &userId, SyncCallBack &callback);
    Result DeleteNotifier(uint64_t sequenceId, std::string &userId);
    Result Process(
        uint64_t sequenceId, const std::map<std::string, DistributedDB::DBStatus> &results, std::string &userId);

private:
    Result DeleteNotifierNoLock(uint64_t sequenceId, std::string &userId);
    std::mutex notifierLock_;
    std::map<std::string, std::vector<uint64_t>> userIdSeqIdRelations_;
    std::map<uint64_t, SyncCallBack> seqIdCallbackRelations_;
};

class ObjectStoreManager {
public:
    ObjectStoreManager();
    static ObjectStoreManager *GetInstance()
    {
        static ObjectStoreManager *manager = new ObjectStoreManager();
        return manager;
    }
    int32_t Save(const std::string &appId, const std::string &sessionId,
        const std::map<std::string, std::vector<uint8_t>> &data, const std::vector<std::string> &deviceList,
        sptr<IObjectSaveCallback> &callback);
    int32_t RevokeSave(
        const std::string &appId, const std::string &sessionId, sptr<IObjectRevokeSaveCallback> &callback);
    int32_t Retrieve(const std::string &appId, const std::string &sessionId, sptr<IObjectRetrieveCallback> callback);
    void SetData(const std::string &dataDir, const std::string &userId);
    int32_t Clear();
    int32_t DeleteByAppId(const std::string &appId);
private:
    enum Status {
        SUCCESS,
        FAILED
    };
    constexpr static const char *SEPERATOR = "_";
    constexpr static const char *LOCAL_DEVICE = "local";
    constexpr static int8_t MAX_OBJECT_SIZE_PER_APP = 16;
    constexpr static int8_t DECIMAL_BASE = 10;
    DistributedDB::KvStoreNbDelegate *OpenObjectKvStore();
    void FlushClosedStore();
    int32_t Open();
    int32_t Close();
    int32_t SetSyncStatus(bool status);
    int32_t SaveToStore(const std::string &appId, const std::string &sessionId, const std::string &toDeviceId,
                        const std::map<std::string, std::vector<uint8_t>> &data);
    int32_t SyncOnStore(const std::string &prefix, const std::vector<std::string> &deviceList, SyncCallBack &callback);
    int32_t RevokeSaveToStore(const std::string &prefix);
    int32_t RetrieveFromStore(
        const std::string &appId, const std::string &sessionId, std::map<std::string, std::vector<uint8_t>> &results);
    void SyncCompleted(const std::map<std::string, DistributedDB::DBStatus> &results, uint64_t sequenceId);
    void ProcessKeyByIndex(std::string &key, uint8_t index);
    std::string GetPropertyName(const std::string &key);
    std::string GetSessionId(const std::string &key);
    int64_t GetTime(const std::string &key);
    void ProcessOldEntry(const std::string &appId);
    void ProcessSyncCallback(const std::map<std::string, int32_t> &results, const std::string &appId,
        const std::string &sessionId, const std::string &deviceId);
    inline std::string GetPropertyPrefix(const std::string &appId, const std::string &sessionId)
    {
        return appId + SEPERATOR + sessionId + SEPERATOR
               + AppDistributedKv::CommunicationProvider::GetInstance().GetLocalDevice().udid + SEPERATOR;
    };
    inline std::string GetPropertyPrefix(
        const std::string &appId, const std::string &sessionId, const std::string &toDeviceId)
    {
        return appId + SEPERATOR + sessionId + SEPERATOR
               + AppDistributedKv::CommunicationProvider::GetInstance().GetLocalDevice().udid + SEPERATOR + toDeviceId
               + SEPERATOR;
    };
    inline std::string GetPrefixWithoutDeviceId(const std::string &appId, const std::string &sessionId)
    {
        return appId + SEPERATOR + sessionId + SEPERATOR;
    };
    std::mutex kvStoreMutex_;
    DistributedDB::KvStoreDelegateManager *kvStoreDelegateManager_ = nullptr;
    DistributedDB::KvStoreNbDelegate *delegate_ = nullptr;
    uint32_t syncCount_ = 0;
    std::string userId_;
    std::atomic<bool> isSyncing_ = false;
};
} // namespace DistributedObject
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_OBJECT_MANAGER_H
