/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "pass_bindings.h"

#include <string>

#include "binding_log.h"

namespace ge {
namespace python_pass_native {
namespace {
constexpr int kPythonLogDebug = 10;
constexpr int kPythonLogInfo = 20;
constexpr int kPythonLogWarning = 30;

// Python logging levels are intentionally accepted as integers so callers can
// pass logging.DEBUG/INFO/WARNING/ERROR without importing a C++ enum.  Unknown
// and severe levels are routed to the error level, preserving visibility of failures.
void EmitGeLog(int level, const std::string &message, const std::string &filename, int line,
               const std::string &function) {
  int32_t dlog_level = DLOG_ERROR;
  if (level <= kPythonLogDebug) {
    dlog_level = DLOG_DEBUG;
  } else if (level <= kPythonLogInfo) {
    dlog_level = DLOG_INFO;
  } else if (level <= kPythonLogWarning) {
    dlog_level = DLOG_WARN;
  }
  // Match GELOG*: DEBUG/INFO/WARNING check the module level; ERROR is
  // submitted directly. Python's thread ident is not the native GE thread id.
  if ((dlog_level != DLOG_ERROR) && (CheckLogLevel(GE_PYPASS_MODULE_NAME, dlog_level) != 1)) {
    return;
  }
  const char *file = filename.empty() ? "python" : filename.c_str();
  const char *func = function.empty() ? "log" : function.c_str();
  // The GE_PYPASS_LOG* macros would inject this C++ bridge's __FILE__/__LINE__/
  // __FUNCTION__.  Supply the original Python call site to the same backend.
  // GELOGE's ErrorNo field is deliberately dropped: the bridge cannot know the
  // Python-side code, so it could only ever emit the constant ge::FAILED, and
  // reading that description needs GE's StatusFactory from the GE source tree.
  DlogRecord(GE_PYPASS_MODULE_NAME, dlog_level, "[%s:%d]%" PRIu64 " %s:%s", file, line, BindingLog::GetTid(), func,
             message.c_str());
}
}  // namespace

void BindGeLog(py::module_ &m) {
  m.def("emit_ge_log", &EmitGeLog, py::arg("level"), py::arg("message"), py::arg("filename") = "",
        py::arg("lineno") = 0, py::arg("funcname") = "");
  m.attr("GE_LOG_SOURCE_SUPPORTED") = true;
}
}  // namespace python_pass_native
}  // namespace ge
