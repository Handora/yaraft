// Copyright 2017 Wu Tao
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <yaraft/logger.h>

namespace yaraft {

// StderrLogger is the default logger of yaraft, if `raftLogger` is not set.
class StderrLogger : public Logger {
 public:
  constexpr StderrLogger() noexcept = default;

  void Log(LogLevel level, int line, const char* file,
           string_view log) override;
};

}  // namespace yaraft
