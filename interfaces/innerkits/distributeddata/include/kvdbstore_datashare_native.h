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

#ifndef KVDBSTORE_DATASHARE_NATIVE_H
#define KVDBSTORE_DATASHARE_NATIVE_H

#include "types.h"
#include "single_kvstore.h"
#include "datashare_predicates.h"
#include "data_query.h"
#include "datashare_abstract_result_set.h"

namespace OHOS {
namespace DistributedKv {
using namespace DataShare;
class kvDBDataShareNative {

public:
    kvDBDataShareNative(std::shared_ptr<SingleKvStore> kvStoreClient);

    ~kvDBDataShareNative() = default;

    std::shared_ptr<DataShareAbstractResultSet> GetDataShareResult(const DataSharePredicates &predicate);

private:
    std::shared_ptr<SingleKvStore> kvStoreClient_ = nullptr;

};
}  // namespace DistributedKv
}  // namespace OHOS
#endif // KVDBSTORE_DATASHARE_NATIVE_H