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
#define LOG_TAG "AccountDelegateDefaultImpl"

#include "account_delegate_default_impl.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
namespace {
    const std::string DEFAULT_OHOS_ACCOUNT_UID = ""; // default UID
}

AccountDelegate::BaseInstance AccountDelegate::getInstance_ = AccountDelegateDefaultImpl::GetBaseInstance;

AccountDelegateDefaultImpl *AccountDelegateDefaultImpl::GetInstance()
{
    static AccountDelegateDefaultImpl accountDelegate;
    return &accountDelegate;
}

AccountDelegate *AccountDelegateDefaultImpl::GetBaseInstance()
{
    return AccountDelegateDefaultImpl::GetInstance();
}

std::string AccountDelegateDefaultImpl::GetCurrentAccountId(const std::string &bundleName) const
{
    ZLOGD("no account part, return default. bundlename:%s", bundleName.c_str());
    return DEFAULT_OHOS_ACCOUNT_UID;
}

std::string AccountDelegateDefaultImpl::GetDeviceAccountIdByUID(int32_t uid) const
{
    ZLOGD("no account part, return default. uid:%d", uid);
    return std::to_string(0);
}

bool AccountDelegateDefaultImpl::QueryUsers(std::vector<int> &users)
{
    ZLOGD("no account part.");
    users.emplace_back(0); // default user
    return true;
}

void AccountDelegateDefaultImpl::SubscribeAccountEvent()
{
    ZLOGD("no account part.");
}

AccountDelegateDefaultImpl::~AccountDelegateDefaultImpl()
{
    ZLOGD("destruct");
    observerMap_.Clear();
}
}  // namespace DistributedKv
}  // namespace OHOS