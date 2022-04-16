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

#ifndef PERMISSION_VALIDATOR_H
#define PERMISSION_VALIDATOR_H
#include <set>
#include <string>
#include "types.h"
#include "visibility.h"

namespace OHOS {
namespace DistributedKv {
const std::string DISTRIBUTED_DATASYNC = "ohos.permission.DISTRIBUTED_DATASYNC";
class PermissionValidator {
public:
    API_EXPORT static PermissionValidator &GetInstance()
    {
        static PermissionValidator permissionValidator;
        return permissionValidator;
    }
    // check whether the client process have enough privilege to share data with the other devices.
    // tokenId: client process tokenId
    API_EXPORT  bool CheckSyncPermission(std::uint32_t tokenId);

    // Check whether the bundle name is in the system service list.
    API_EXPORT  bool IsSystemService(const std::string &bundleName, pid_t uid, std::uint32_t tokenId);

    // Check  the app with this bundle name is auto launch enabled.
    API_EXPORT  bool IsAutoLaunchEnabled(const std::string &bundleName) const;

private:
    static std::set<std::string> autoLaunchEnableList_; // the list for auto launch enabled app.
};
} // namespace DistributedKv
} // namespace OHOS
#endif // PERMISSION_VALIDATOR_H
