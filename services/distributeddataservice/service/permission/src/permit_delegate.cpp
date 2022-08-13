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

#define LOG_TAG "PermitDelegate"
#include "permit_delegate.h"
#include "communication_provider.h"
#include "metadata/meta_data_manager.h"
#include "metadata/strategy_meta_data.h"
#include "permission/permission_validator.h"
#include "user_delegate.h"
#include "utils/anonymous.h"
#include "store_types.h"
#include "runtime_config.h"
#include "log_print.h"

namespace OHOS::DistributedData {
using DBStatus = DistributedDB::DBStatus;
using DBConfig = DistributedDB::RuntimeConfig;
using DBFlag = DistributedDB::PermissionCheckFlag;
using Commu = OHOS::AppDistributedKv::CommunicationProvider;
using PermissionValidator = OHOS::DistributedKv::PermissionValidator;

PermitDelegate::PermitDelegate()
{}

PermitDelegate::~PermitDelegate()
{}

PermitDelegate &PermitDelegate::GetInstance()
{
    static PermitDelegate permit;
    return permit;
}

void PermitDelegate::Init()
{
    auto activeCall = [this](const ActiveParam &param) -> bool {
        return SyncActivate(param);
    };
    DBStatus status = DBConfig::SetSyncActivationCheckCallback(activeCall);
    ZLOGI("set active callback status:%d.", status);

    auto permitCall = [this](const CheckParam &Param, uint8_t flag) -> bool {
        return VerifyPermission(Param, flag);
    };
    status = DBConfig::SetPermissionCheckCallback(permitCall);
    ZLOGI("set permission callback status:%d.", status);

    auto extraCall = [this](const CondParam &param) -> std::map<std::string, std::string> {
        return GetExtraCondition(param);
    };
    status = DBConfig::SetPermissionConditionCallback(extraCall);
    ZLOGI("set extra condition call status:%d.", status);
}

bool PermitDelegate::SyncActivate(const ActiveParam &param)
{
    ZLOGD("user:%{public}s, app:%{public}s, store:%{public}s, instanceId:%{public}d",
        param.userId.c_str(), param.appId.c_str(), param.storeId.c_str(), param.instanceId);
    if (param.instanceId != 0) {
        return false;
    }
    std::set<std::string> activeUsers = UserDelegate::GetInstance().GetLocalUsers();
    return activeUsers.count(param.userId);
}

bool PermitDelegate::VerifyPermission(const CheckParam &param, uint8_t flag)
{
    ZLOGI("user:%s, appId:%s, storeId:%s, remote devId:%s, instanceId:%d, flag:%u", param.userId.c_str(),
        param.appId.c_str(), param.storeId.c_str(), Anonymous::Change(param.deviceId).c_str(), param.instanceId, flag);

    auto devId = Commu::GetInstance().GetLocalDevice().uuid;
    StoreMetaData data;
    data.user = param.userId;
    data.bundleName = param.appId;
    data.storeId = param.storeId;
    data.deviceId = param.deviceId;
    data.instanceId = param.instanceId;
    auto key = data.GetKey();
    bool result = false;
    permitMap_.Compute(key, [&](const auto &, PermitState &value) {
        if (value.SyncPermit()) {
            result = true;
            return true;
        }
        if (flag == DBFlag::CHECK_FLAG_RECEIVE && !value.extraConditionAllow) {
            value.extraConditionAllow = VerifyExtraCondition(param.extraConditions);
        }
        StoreMetaData loadMeta;
        auto prefix = StoreMetaData::GetPrefix({ devId, param.userId, "default" });
        if (LoadStoreMeta(prefix, param, loadMeta) != Status::SUCCESS) {
            result = false;
            return true;
        }
        if (loadMeta.appType.compare("default") == 0) {
            value.defaultAllow = true;
            result = true;
            return true;
        }
        if (!value.strategyAllow) {
            auto status = VerifyStrategy(loadMeta, param.deviceId);
            if (status != Status::SUCCESS) {
                ZLOGW("verify strategy fail, status:%d.", status);
                result = false;
                return true;
            }
            value.strategyAllow = true;
        }
        if (!value.privilegeAllow) {
            value.privilegeAllow = PermissionValidator::GetInstance().CheckSyncPermission(loadMeta.tokenId);
        }
        result = value.SyncPermit();
        return true;
    });
    return result;
}

Status PermitDelegate::LoadStoreMeta(const std::string &prefix, const CheckParam &param, StoreMetaData &data) const
{
    std::vector<StoreMetaData> metaData;
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, metaData)) {
        ZLOGE("load data failed.");
        return Status::NOT_FOUND;
    }
    for (const auto &item : metaData) {
        if (item.appId == param.appId && item.storeId == param.storeId && item.instanceId == param.instanceId) {
            data = item;
            return Status::SUCCESS;
        }
    }
    return Status::NOT_FOUND;
}

bool PermitDelegate::VerifyExtraCondition(const std::map<std::string, std::string> &cond) const
{
    (void)cond;
    return true;
}

std::map<std::string, std::string> PermitDelegate::GetExtraCondition(const CondParam &param)
{
    (void)param;
    return {};
}

Status PermitDelegate::VerifyStrategy(const StoreMetaData &data, const std::string &rmdevId) const
{
    StrategyMeta local(data.deviceId, data.user, data.bundleName, data.storeId);
    MetaDataManager::GetInstance().LoadMeta(local.GetKey(), local);
    StrategyMeta remote(rmdevId, data.user, data.bundleName, data.storeId);
    MetaDataManager::GetInstance().LoadMeta(remote.GetKey(), remote);
    if (!local.IsEffect() || !remote.IsEffect()) {
        ZLOGD("no range, sync permission success.");
        return Status::SUCCESS;
    }
    auto lremotes = local.capabilityRange.remoteLabel;
    auto rlocals = remote.capabilityRange.localLabel;
    for (const auto &lrmote : lremotes) {
        if (std::find(rlocals.begin(), rlocals.end(), lrmote) != rlocals.end()) {
            ZLOGD("find range, sync permission success.");
            return Status::SUCCESS;
        }
    }
    return Status::ERROR;
}
} // namespace OHOS::DistributedData