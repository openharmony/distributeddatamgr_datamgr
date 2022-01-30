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

#include "checker/default/bundle_checker.h"
#include <memory>
#include "bundlemgr/bundle_mgr_client.h"
namespace OHOS {
namespace DistributedData {
using namespace AppExecFwk;
BundleChecker BundleChecker::instance_;
constexpr pid_t BundleChecker::SYSTEM_UID;
BundleChecker::BundleChecker()
{
    CheckerManager::GetInstance().RegisterPlugin(
        "BundleChecker", [this]() -> auto { return this; });
}

BundleChecker::~BundleChecker()
{
}

void BundleChecker::Initialize()
{
}

bool BundleChecker::SetTrustInfo(const CheckerManager::Trust &trust)
{
    trusts_[trust.bundleName] = trust.appId;
    return true;
}

std::string BundleChecker::GetAppId(pid_t uid, const std::string &bundleName)
{
    BundleMgrClient bmsClient;
    std::string bundle = bundleName;
    if (uid != CheckerManager::INVALID_UID) {
        auto success = bmsClient.GetBundleNameForUid(uid, bundle);
        if (uid < SYSTEM_UID || !success || bundle != bundleName) {
            return "";
        }
    }

    auto bundleInfo = std::make_unique<BundleInfo>();
    auto success = bmsClient.GetBundleInfo(bundle, BundleFlag::GET_BUNDLE_DEFAULT, *bundleInfo);
    if (!success) {
        return "";
    }
    auto it = trusts_.find(bundleName);
    if (it != trusts_.end() && (it->second == bundleInfo->appId)) {
        return bundleName;
    }

    return bundleInfo->appId;
}

bool BundleChecker::IsValid(pid_t uid, const std::string &bundleName)
{
    BundleMgrClient bmsClient;
    std::string bundle = bundleName;
    auto success = bmsClient.GetBundleNameForUid(uid, bundle);
    if (uid < SYSTEM_UID || !success || bundle != bundleName) {
        return false;
    }

    auto bundleInfo = std::make_unique<BundleInfo>();
    success = bmsClient.GetBundleInfo(bundle, BundleFlag::GET_BUNDLE_DEFAULT, *bundleInfo);
    if (!success) {
        return false;
    }
    auto it = trusts_.find(bundleName);
    if (it != trusts_.end() && (it->second == bundleInfo->appId)) {
        return true;
    }

    return true;
}
}
}