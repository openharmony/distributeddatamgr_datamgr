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
#define LOG_TAG "KVDBExporter"
#include "kvdb_exporter.h"

#include "backup_manager.h"
#include "directory_manager.h"
#include "log_print.h"
#include "reporter.h"
#include "store_cache.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedDataDfx;
__attribute__((used)) KVDBExporter KVDBExporter::instance_;
KVDBExporter::KVDBExporter() noexcept
{
    auto fun = [](
        const StoreMetaData &meta, const std::vector<uint8_t> &pwd, const std::string &backupPath, Status &status) {
        DBManager manager(meta.appId, meta.user);
        auto path = DirectoryManager::GetInstance().GetStorePath(meta);
        manager.SetKvStoreConfig({ path });
        DBPassword dbPassword;
        dbPassword.SetValue(pwd.data(), pwd.size());
        auto dbOption = StoreCache::GetDBOption(meta, dbPassword);

        manager.GetKvStore(meta.storeId, dbOption, [&manager, &backupPath, &dbPassword, &status]
            (DistributedDB::DBStatus dbstatus, DistributedDB::KvStoreNbDelegate *delegate) {
            if (delegate == nullptr) {
                ZLOGE("Auto backup delegate is null");
                status = ERROR;
                return;
            }
            dbstatus = delegate->Export(backupPath, dbPassword);
            status = (dbstatus == DistributedDB::DBStatus::OK) ? SUCCESS : ERROR;
            manager.CloseKvStore(delegate);
        });

        std::string message;
        message.append(" backup name [")
            .append(backupPath)
            .append("], isEncrypt [")
            .append(std::to_string(meta.isEncrypt))
            .append("]")
            .append("], backup result status [")
            .append(std::to_string(status))
            .append("]");
        Reporter::GetInstance()->BehaviourReporter()->Report(
            { meta.account, meta.appId, meta.storeId, BehaviourType::DATABASE_BACKUP, message });
    };
    BackupManager::GetInstance().RegisterExporter(KvStoreType::SINGLE_VERSION, fun);
    BackupManager::GetInstance().RegisterExporter(KvStoreType::DEVICE_COLLABORATION, fun);
}
} // namespace OHOS::DistributedKv
