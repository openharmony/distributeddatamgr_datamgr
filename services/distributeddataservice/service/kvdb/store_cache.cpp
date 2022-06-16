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

#include "store_cache.h"

#include "crypto_manager.h"
#include "directory_manager.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "types.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedData;
constexpr int64_t StoreCache::INTERVAL;
constexpr size_t StoreCache::TIME_TASK_NUM;
StoreCache::DBStore *StoreCache::GetStore(const StoreMetaData &data, std::shared_ptr<Observers> observers,
    DBStatus &status)
{
    DBStore *store = nullptr;
    status = DBStatus::NOT_FOUND;
    stores_.Compute(data.tokenId, [this, &store, &data, &status, &observers](auto &key, auto &stores) {
        auto it = stores.find(data.storeId);
        if (it != stores.end()) {
            it->second.SetObservers(observers);
            store = it->second;
            return true;
        }

        DBManager manager(data.appId, data.user);
        manager.SetKvStoreConfig({ DirectoryManager::GetInstance().GetStorePath(data) });
        manager.GetKvStore(data.storeId, GetDBOption(data), [&status, &store](auto dbStatus, auto *tmpStore) {
            status = dbStatus;
            store = tmpStore;
        });

        if (store != nullptr) {
            stores.emplace(std::piecewise_construct, std::forward_as_tuple(data.storeId),
                std::forward_as_tuple(store, observers));
        }
        return !stores.empty();
    });

    scheduler_.At(std::chrono::system_clock::now() + std::chrono::minutes(INTERVAL),
        std::bind(&StoreCache::CollectGarbage, this));

    return store;
}

void StoreCache::CollectGarbage()
{
    DBManager manager("", "");
    auto current = std::chrono::system_clock::now();
    stores_.EraseIf([&manager, &current](auto &key, std::map<std::string, DBStoreDelegate> &delegates) {
        for (auto it = delegates.begin(); it != delegates.end();) {
            // if the kv store is BUSY we wait more INTERVAL minutes again
            if ((it->second < current) || !it->second.Close(manager)) {
                ++it;
            } else {
                it = delegates.erase(it);
            }
        }
        return delegates.empty();
    });

    if (!stores_.Empty()) {
        scheduler_.At(current + std::chrono::minutes(INTERVAL), std::bind(&StoreCache::CollectGarbage, this));
    }
}

StoreCache::DBOption StoreCache::GetDBOption(const StoreMetaData &data) const
{
    DBOption dbOption;
    dbOption.syncDualTupleMode = true; // tuple of (appid+storeid)
    dbOption.createIfNecessary = false;
    dbOption.isMemoryDb = false;
    dbOption.isEncryptedDb = data.isEncrypt;
    if (data.isEncrypt) {
        dbOption.cipher = DistributedDB::CipherType::AES_256_GCM;
        SecretKeyMetaData secretKey;
        secretKey.storeType = data.storeType;
        auto storeKey = SecretKeyMetaData::GetKey({ data.user, "default", data.bundleName, data.storeId });
        MetaDataManager::GetInstance().SaveMeta(storeKey, secretKey, true);
        std::vector<uint8_t> password;
        CryptoManager::GetInstance().Decrypt(secretKey.sKey, password);
        dbOption.passwd.SetValue(password.data(), password.size());
        password.assign(password.size(), 0);
    }

    if (data.storeType == KvStoreType::SINGLE_VERSION) {
        dbOption.conflictResolvePolicy = DistributedDB::LAST_WIN;
    } else if (data.storeType == KvStoreType::DEVICE_COLLABORATION) {
        dbOption.conflictResolvePolicy = DistributedDB::DEVICE_COLLABORATION;
    }

    dbOption.schema = data.schema;
    dbOption.createDirByStoreIdOnly = true;
    dbOption.secOption = GetDBSecurity(data.securityLevel);
    return dbOption;
}

StoreCache::DBSecurity StoreCache::GetDBSecurity(int32_t secLevel) const
{
    if (secLevel < SecurityLevel::NO_LABEL || secLevel > SecurityLevel::S4) {
        return { DistributedDB::NOT_SET, DistributedDB::ECE };
    }
    if (secLevel == SecurityLevel::S3) {
        return { DistributedDB::S3, DistributedDB::SECE };
    }
    if (secLevel == SecurityLevel::S4) {
        return { DistributedDB::S4, DistributedDB::ECE };
    }
    return { secLevel, DistributedDB::ECE };
}

StoreCache::DBStoreDelegate::DBStoreDelegate(DBStore *delegate, std::shared_ptr<Observers> observers)
    : delegate_(delegate), observers_(std::move(observers))
{
    time_ = std::chrono::system_clock::now() + std::chrono::minutes(INTERVAL);
    if (observers_ != nullptr && !observers_->empty()) {
        delegate_->RegisterObserver({}, DistributedDB::OBSERVER_CHANGES_FOREIGN, this);
    }
}

StoreCache::DBStoreDelegate::~DBStoreDelegate()
{
    delegate_ = nullptr;
}

StoreCache::DBStoreDelegate::operator DBStore *()
{
    time_ = std::chrono::system_clock::now() + std::chrono::minutes(INTERVAL);
    if (observers_ != nullptr && !observers_->empty()) {
        delegate_->RegisterObserver({}, DistributedDB::OBSERVER_CHANGES_FOREIGN, this);
    }
    return delegate_;
}

bool StoreCache::DBStoreDelegate::operator<(const Time &time) const
{
    return time_ < time;
}

bool StoreCache::DBStoreDelegate::Close(DBManager &manager)
{
    if (delegate_ != nullptr) {
        delegate_->UnRegisterObserver(this);
    }

    auto status = manager.CloseKvStore(delegate_);
    if (status == DBStatus::BUSY) {
        return false;
    }
    delegate_ = nullptr;
    return true;
}

void StoreCache::DBStoreDelegate::OnChange(const DistributedDB::KvStoreChangedData &data)
{
    if (observers_ == nullptr) {
        return;
    }

    const auto &dbInserts = data.GetEntriesInserted();
    const auto &dbUpdates = data.GetEntriesUpdated();
    const auto &dbDeletes = data.GetEntriesDeleted();
    ChangeNotification change(Convert(dbInserts), Convert(dbUpdates), Convert(dbDeletes), std::string(), false);
    for (auto &observer : *observers_) {
        observer->OnChange(change);
    }
}

void StoreCache::DBStoreDelegate::SetObservers(std::shared_ptr<Observers> observers)
{
    if (observers_ != observers && observers != nullptr) {
        observers_ = observers;
    }
}
std::vector<Entry> StoreCache::DBStoreDelegate::Convert(const std::list<DBEntry> &dbEntries)
{
    std::vector<Entry> entries;
    for (const auto &entry : dbEntries) {
        Entry tmpEntry;
        tmpEntry.key = entry.key;
        tmpEntry.value = entry.value;
        entries.push_back(tmpEntry);
    }
    return entries;
}
}; // namespace OHOS::DistributedKv
