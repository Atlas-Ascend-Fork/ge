/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef GE_COMMON_PLUGIN_OPS_DELIVERABLES_PARSER_H_
#define GE_COMMON_PLUGIN_OPS_DELIVERABLES_PARSER_H_

#include <map>
#include <string>
#include <vector>

namespace ge {
// Lightweight parser for the ops deliverables manifest. Unlike OpsDeliverablesManager it never resolves
// placeholder values itself: the caller always supplies the variable map, so the parser only depends on json
// parsing and libc and can be reused from graph_base consumers.
// Two-stage usage keeps repeated lookups cheap:
//   1. Load() parses and stores the manifest located under the ASCEND_OPP_PATH environment variable.
//   2. GetPaths() resolves the named deliverable against caller-supplied variables and returns every
//      candidate that points to an existing readable regular file, in the configured order; a missing
//      candidate is skipped so entries can list fallback paths.
class OpsDeliverablesParser {
 public:
  // Parses and stores ${ASCEND_OPP_PATH}/conf/ops_deliverables.json. Returns false and fills `reason` when
  // ASCEND_OPP_PATH is unset or the file is absent or invalid; a failed Load always leaves the parser
  // without any stored entry.
  bool Load(std::string &reason);
  // Resolves `key` to every usable path, in the configured order. `{placeholder}` tokens in the manifest are
  // replaced with `variables`; a token missing (or empty) in `variables` only invalidates the candidate that
  // uses it, later candidates are still tried. Candidates resolving to the same canonical path are
  // deduplicated. Returns false and clears `paths` when the manifest is not loaded, `key` is not defined, or
  // no candidate resolves to an existing readable regular file; on success `paths` holds absolute canonical
  // paths.
  bool GetPaths(const std::string &key, const std::map<std::string, std::string> &variables,
                std::vector<std::string> &paths) const;
  // Convenience form of GetPaths() returning only the first usable candidate; false when none resolves.
  bool GetPath(const std::string &key, const std::map<std::string, std::string> &variables, std::string &path) const;

 private:
  bool loaded_ = false;
  // Deliverable name -> ordered candidate templates, kept as parsed from the manifest.
  std::map<std::string, std::vector<std::string>> path_templates_;
};
}  // namespace ge
#endif
