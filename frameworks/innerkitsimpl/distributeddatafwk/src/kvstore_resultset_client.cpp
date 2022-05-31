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

#define LOG_TAG "KvStoreResultSetClient"

#include "kvstore_resultset_client.h"

namespace OHOS::DistributedKv {
KvStoreResultSetClient::KvStoreResultSetClient(sptr<IKvStoreResultSet> kvStoreProxy)
    :kvStoreResultSetProxy_(kvStoreProxy)
{}

int KvStoreResultSetClient::GetCount() const
{
    return kvStoreResultSetProxy_->GetCount();
}

int KvStoreResultSetClient::GetPosition() const
{
    return kvStoreResultSetProxy_->GetPosition();
}

bool KvStoreResultSetClient::MoveToFirst()
{
    return kvStoreResultSetProxy_->MoveToFirst();
}

bool KvStoreResultSetClient::MoveToLast()
{
    return kvStoreResultSetProxy_->MoveToLast();
}

bool KvStoreResultSetClient::MoveToNext()
{
    return kvStoreResultSetProxy_->MoveToNext();
}

bool KvStoreResultSetClient::MoveToPrevious()
{
    return kvStoreResultSetProxy_->MoveToPrevious();
}

bool KvStoreResultSetClient::Move(int offset)
{
    return kvStoreResultSetProxy_->Move(offset);
}

bool KvStoreResultSetClient::MoveToPosition(int position)
{
    return kvStoreResultSetProxy_->MoveToPosition(position);
}

bool KvStoreResultSetClient::IsFirst() const
{
    return kvStoreResultSetProxy_->IsFirst();
}

bool KvStoreResultSetClient::IsLast() const
{
    return kvStoreResultSetProxy_->IsLast();
}

bool KvStoreResultSetClient::IsBeforeFirst() const
{
    return kvStoreResultSetProxy_->IsBeforeFirst();
}

bool KvStoreResultSetClient::IsAfterLast() const
{
    return kvStoreResultSetProxy_->IsAfterLast();
}

Status KvStoreResultSetClient::GetEntry(Entry &entry) const
{
    return kvStoreResultSetProxy_->GetEntry(entry);
}

Status KvStoreResultSetClient::Close()
{
    return NOT_SUPPORT;
}

sptr<IKvStoreResultSet> KvStoreResultSetClient::GetKvStoreResultSetProxy() const
{
    return kvStoreResultSetProxy_;
}
} // namespace OHOS::DistributedKv
