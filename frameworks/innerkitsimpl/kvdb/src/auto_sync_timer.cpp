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

#define LOG_TAG "AutoSyncTimer"

#include <set>
#include "kvdb_service_client.h"
#include "auto_sync_timer.h"
namespace OHOS::DistributedKv {
AutoSyncTimer &AutoSyncTimer::GetInstance()
{
    static AutoSyncTimer instance;
    return instance;
}

void AutoSyncTimer::AddAutoSyncStore(const std::string &appId, const std::set<StoreId> &storeIds)
{
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    if (forceSyncTask_ == SchedulerTask()) {
        auto expiredTime = std::chrono::system_clock::now() + std::chrono::milliseconds(FORCE_SYNC__DELAY_MS);
        delaySyncTask_ = scheduler_.At(expiredTime, ProcessTask());
    }
    if (delaySyncTask_ == SchedulerTask()) {
        auto expiredTime = std::chrono::system_clock::now() + std::chrono::milliseconds(SYNC_DELAY_MS);
        delaySyncTask_ = scheduler_.At(expiredTime, ProcessTask());
    } else {
        delaySyncTask_ = scheduler_.Reset(delaySyncTask_, delaySyncTask_->first,
                                          std::chrono::milliseconds(SYNC_DELAY_MS));
    }
    if (stores_.Contains(appId)) {
        stores_[appId].insert(storeIds.begin(), storeIds.end());
    } else {
        stores_.Insert(appId, storeIds);
    }
}

ConcurrentMap<std::string, std::set<StoreId>> AutoSyncTimer::GetStoreIds(ConcurrentMap<std::string, std::set<StoreId>> &remain)
{
    ConcurrentMap<std::string, std::set<StoreId>> stores;
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    int count = SYNC_STORE_NUM;
    stores_.EraseIf([this, &stores, &count, &remain](const std::string &key, std::set<StoreId> &value) {
        int size = stores_[key].size();
        if (size <= count) {
            stores.Insert(key, value);
            count = count - size;
        } else {
            auto iter = value.begin();
            if (count != 0) {
                while (count > 0) {
                    iter++;
                }
                std::set<StoreId> ids(value.begin(), iter);
                value.erase(value.begin(), iter);
                stores.Insert(key, ids);
                remain.Insert(key, value);
            } else {
                remain.Insert(key, value);
            }
        }
        return true;
    });
    return stores;
}

std::function<void()> AutoSyncTimer::ProcessTask()
{
    return [this]() {
        auto service = KVDBServiceClient::GetInstance();
        if (service == nullptr) {
            return;
        }
        {
            std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
            scheduler_.Clean();
            forceSyncTask_ = {};
            delaySyncTask_ = {};
        }
        ConcurrentMap<std::string, std::set<StoreId>> remains;
        ConcurrentMap<std::string, std::set<StoreId>> ids = GetStoreIds(remains);

        KVDBService::SyncInfo syncInfo;
        ids.ForEach([this, &service, &syncInfo](const std::string &key, const std::set<StoreId> &value){
            for (auto &storeId : value) {
                service->Sync({ key }, storeId, syncInfo);
            }
            return false;
        });
        if (!remains.Empty()) {
            remains.ForEach([this, &service, &syncInfo](const std::string &key, const std::set<StoreId> &value){
                AddAutoSyncStore(key, value);
                return false;
            });
        }
    };
}
}

