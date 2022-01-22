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

#ifndef USER_CHANGE_MONITOR
#define USER_CHANGE_MONITOR

#include "runtime_context.h"
#include "db_types.h"
#include "platform_specific.h"
#include "macro_utils.h"

namespace DistributedDB {
class UserChangeMonitor final {
public:
    UserChangeMonitor();
    ~UserChangeMonitor();

    DISABLE_COPY_ASSIGN_MOVE(UserChangeMonitor);

    // Start the UserChangeMonitor
    int Start();

    // Stop the UserChangeMonitor
    void Stop();

    // Register a user changed lister, it will be callback when user changed.
    NotificationChain::Listener *RegisterUserChangedListerner(const UserChangedAction &action, bool isActiveEvent,
        int &errCode);

    // Notify USER_CHANGE_EVENT.
    void NotifyUserChanged() const;
private:
    static const EventType USER_ACTIVE_EVENT = 3;
    static const EventType USER_NON_ACTIVE_EVENT = 4;

    // prepare notifier chain
    int PrepareNotifierChain();

    mutable std::mutex userChangeMonitorLock_;
    NotificationChain *userNotifier_;
    bool isStarted_ = false;
};
} // namespace DistributedDB

#endif // USER_CHANGE_MONITOR