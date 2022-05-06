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

#ifndef KVDBSTORE_RESULT_SET_H
#define KVDBSTORE_RESULT_SET_H

#include "datashare_abstract_result_set.h"
#include "kvstore_result_set.h"
#include "single_kvstore.h"
#include "datashare_errno.h"

namespace OHOS {
namespace DistributedKv {
using namespace DataShare;
class KvDBStoreResultSet : public DataShareAbstractResultSet
{
public:
    KvDBStoreResultSet(std::shared_ptr<KvStoreResultSet> kvResultSet);

    ~KvDBStoreResultSet() = default;

    int GetRowCount(int &count) override;
    
    int GetAllColumnOrKeyName(std::vector<std::string> &columnOrKeyNames) override;
    
    bool OnGo(int oldRowIndex, int newRowIndex, const std::shared_ptr<DataShareBlockWriter> &writer) override;
    
private:
    int Count(); 

    bool FillBlock(int startRowIndex, const std::shared_ptr<DataShareBlockWriter> &writer);

    static constexpr int INVALID_COUNT = -1;

    int resultRowCount {INVALID_COUNT};

    std::shared_ptr<KvStoreResultSet> kvResultSet_;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif  // KVDBSTORE_RESULT_SET_H

