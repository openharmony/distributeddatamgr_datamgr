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

#include "user_change_monitor.h"

#include "db_errno.h"
#include "log_print.h"

namespace DistributedDB {
UserChangeMonitor::UserChangeMonitor()
    : userNotifier_(nullptr),
      isStarted_(false)
{
}

UserChangeMonitor::~UserChangeMonitor()
{
    Stop();
}

int UserChangeMonitor::Start()
{
    if (isStarted_) {
        return E_OK;
    }

    int errCode = PrepareNotifierChain();
    if (errCode != E_OK) {
        return errCode;
    }
    isStarted_ = true;
    return E_OK;
}

void UserChangeMonitor::Stop()
{
    if (!isStarted_) {
        return;
    }
    if (userNotifier_ == nullptr) {
        userNotifier_->UnRegisterEventType(USER_ACTIVE_EVENT);
        userNotifier_->UnRegisterEventType(USER_NON_ACTIVE_EVENT);
        RefObject::KillAndDecObjRef(userNotifier_);
        userNotifier_ = nullptr;
    }
    isStarted_ = false;
}

NotificationChain::Listener *UserChangeMonitor::RegisterUserChangedListerner(const UserChangedAction &action,
    bool isActiveEvent, int &errCode)
{
    EventType event;
    if (isActiveEvent) {
        event = USER_ACTIVE_EVENT;
    } else {
        event = USER_NON_ACTIVE_EVENT;
    }
    if (action == nullptr) {
        errCode = -E_INVALID_ARGS;
        return nullptr;
    }
    if (userNotifier_ == nullptr) {
        errCode = -E_NOT_INIT;
        return nullptr;
    }

    return userNotifier_->RegisterListener(event, action, nullptr, errCode);
}

int UserChangeMonitor::PrepareNotifierChain()
{
    int errCode = E_OK;
    std::lock_guard<std::mutex> autoLock(userChangeMonitorLock_);
    if (userNotifier_ != nullptr) {
        return E_OK;
    }
    if (userNotifier_ == nullptr) {
        userNotifier_ = new (std::nothrow) NotificationChain();
        if (userNotifier_ == nullptr) {
            return -E_OUT_OF_MEMORY;
        }
        errCode = userNotifier_->RegisterEventType(USER_ACTIVE_EVENT);
        if (errCode != E_OK) {
            RefObject::KillAndDecObjRef(userNotifier_);
            userNotifier_ = nullptr;
            return errCode;
        }
        errCode = userNotifier_->RegisterEventType(USER_NON_ACTIVE_EVENT);
        if (errCode != E_OK) {
            RefObject::KillAndDecObjRef(userNotifier_);
            userNotifier_ = nullptr;
            return errCode;
        }
    }
    return errCode;
}

void UserChangeMonitor::NotifyUserChanged() const
{
    std::lock_guard<std::mutex> lock(userChangeMonitorLock_);
    if (userNotifier_ == nullptr) {
        LOGD("NotifyUNotifyUserChangedserChange fail, userChangedNotifier is null.");
        return;
    }
    userNotifier_->NotifyEvent(USER_ACTIVE_EVENT, nullptr);
    userNotifier_->NotifyEvent(USER_NON_ACTIVE_EVENT, nullptr);
}
} // namespace DistributedDB