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

#define LOG_TAG "PermissionValidator"

#include "permission_validator.h"
#include <regex>
#include <string>
#include "accesstoken_kit.h"
#include "checker/checker_manager.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
using namespace Security::AccessToken;
using namespace OHOS::DistributedData;
// initialize auto launch enabled applications white list.
std::set<std::string> PermissionValidator::autoLaunchEnableList_ = {};

// check whether the client process have enough privilege to share data with the other devices.
bool PermissionValidator::CheckSyncPermission(std::uint32_t tokenId)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) == TOKEN_NATIVE) {
        return true;
    }
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) == TOKEN_HAP) {
        return (AccessTokenKit::VerifyAccessToken(tokenId, DISTRIBUTED_DATASYNC) == PERMISSION_GRANTED);
    }

    ZLOGI("invalid tokenid:%u", tokenId);
    return false;
}

// Check whether the bundle name is in the system service list.
bool PermissionValidator::IsSystemService(const std::string &bundleName, pid_t uid, std::uint32_t tokenId)
{
    (void)bundleName;
    (void)uid;
    (void)tokenId;
    return false;
}

// Check whether the app with this bundle name is auto launch enabled.
bool PermissionValidator::IsAutoLaunchEnabled(const std::string &bundleName) const
{
    for (auto it : autoLaunchEnableList_) {
        size_t pos = bundleName.rfind(it);
        if (pos != std::string::npos) {
            return true;
        }
    }
    ZLOGD("AppId:%s is not allowed.", bundleName.c_str());
    return false;
}
} // namespace DistributedKv
} // namespace OHOS
