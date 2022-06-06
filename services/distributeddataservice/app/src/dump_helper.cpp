/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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

#include "dump_helper.h"
#include "kvstore_data_service.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
namespace {
constexpr const int32_t MAX_RECORED_ERROR = 10;
constexpr const int32_t FIRST_PARAM = 0;
constexpr const int32_t SECOND_PARAM = 1;
constexpr const int32_t ONE_COMMEND_PARAM = 1;
constexpr const int32_t TWO_COMMEND_PARAM = 2;
constexpr const char *ARGS_HELP = "-h";
constexpr const char *ARGS_USER_INFO = "-userInfo";
constexpr const char *ARGS_APP_INFO = "-appInfo";
constexpr const char *ARGS_STORE_INFO = "-storeInfo";
constexpr const char *ARGS_ERROR_INFO = "-errorInfo";
constexpr const char *ILLEGAL_INFOMATION = "The arguments are illegal and you can enter '-h' for help.\n";
}

void DumpHelper::AddErrorInfo(const std::string &error)
{
    std::lock_guard<std::mutex> lock(hidumperMutex_);
    if (g_errorInfo.size() + 1 > MAX_RECORED_ERROR) {
        g_errorInfo.pop_front();
        g_errorInfo.push_back(error);
    } else {
        g_errorInfo.push_back(error);
    }
}

void DumpHelper::ShowError(int fd)
{
    dprintf(fd, "The number of recent errors recorded is %d\n", g_errorInfo.size());
    int i = 0;
    for (const auto &it : g_errorInfo) {
        dprintf(fd, "Error ID: %d        ErrorInfo: %s\n", ++i, it.c_str());
    }
}

DumpFlag DumpHelper::Dump(int fd, const std::vector<std::string> &args, std::string &options)
{
    std::string command = "";

    if (args.size() == ONE_COMMEND_PARAM) {
        command = args.at(FIRST_PARAM);
    } else if (args.size() == TWO_COMMEND_PARAM) {
        command = args.at(FIRST_PARAM);
        options = args.at(SECOND_PARAM);
    } else {
        ShowError(fd);
        return DumpFlag::DUMP_ALL;
    }

    if (command == ARGS_HELP) {
        ShowHelp(fd);
    } else if (command ==ARGS_ERROR_INFO) {
        ShowError(fd);
    } else if (command == ARGS_USER_INFO) {
        return DumpFlag::DUMP_USER_INFO;
    } else if (command == ARGS_APP_INFO) {
        return DumpFlag::DUMP_APP_INFO;
    } else if (command == ARGS_STORE_INFO) {
        return DumpFlag::DUMP_STORE_INFO;
    } else {
        ShowIllealInfomation(fd);
    }
    return DumpFlag::DUMP_DONE;
}

void DumpHelper::ShowHelp(int fd)
{
    std::string result;
    result.append("Usage:dump  <command> [options]\n")
          .append("Description:\n")
          .append(ARGS_USER_INFO)
		  .append("            ")
          .append("dump all user information in the system\n")
          .append(ARGS_APP_INFO)
		  .append("             ")
          .append("dump list of all app information in the system\n")
          .append(ARGS_APP_INFO)
		  .append(" [appID]     ")
          .append("dump information about the specified app in the system\n")
          .append(ARGS_STORE_INFO)
		  .append("           ")
          .append("dump list of all store information in the system\n")
          .append(ARGS_STORE_INFO)
		  .append(" [storeID] ")
          .append("dump information about the specified store in the system\n")
          .append(ARGS_ERROR_INFO)
		  .append("           ")
          .append("dump the recent errors information in the system\n");
    dprintf(fd, "%s\n", result.c_str());
}

void DumpHelper::ShowIllealInfomation(int fd)
{
    dprintf(fd, "%s\n", ILLEGAL_INFOMATION);
}
}  // namespace DistributedKv
}  // namespace OHOS

