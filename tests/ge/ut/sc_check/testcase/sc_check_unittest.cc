/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include <gtest/gtest.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <unistd.h>
#include <vector>

namespace {
bool IsDirectory(const std::string &path) {
  struct stat buffer;
  return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

bool IsRegularFile(const std::string &path) {
  struct stat buffer;
  return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

void CountFilesAndDirs(const std::string &path, int &max_cnt) {
  int file_count = 0;
  int dir_count = 0;

  DIR *dir = opendir(path.c_str());
  if (dir == nullptr) {
    return;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    std::string entry_path = path + "/" + entry->d_name;
    if (IsDirectory(entry_path)) {
      dir_count++;
      // 递归调用以检查子目录
      CountFilesAndDirs(entry_path, max_cnt);
    } else if (IsRegularFile(entry_path)) {
      file_count++;
    }
  }
  closedir(dir);

  const auto count = file_count + dir_count;
  if (count > 50) {
    throw std::runtime_error("Directory " + path +
                             " exceeds the limit of 50 files and directories number "
                             ":" +
                             std::to_string(count));
  }
  if (count > max_cnt) {
    max_cnt = count;
    std::cout << path << "/ with files and direct sub directories number is " << count << ", update max_cnt."
              << std::endl;
  }
}

constexpr size_t LICENSE_HEADER_SCAN_BYTES = 1024;
// NBSP(U+00A0) 的 UTF-8 编码，出现在许可头里即为隐形字符缺陷
const std::string NBSP_UTF8 = "\xc2\xa0";

bool IsSkippedDir(const std::string &name) {
  static const std::set<std::string> kSkippedDirs = {".git",        "build",       "build_ut", "build_st",
                                                     "build_out",   "build_gcov",  "output",   "cov",
                                                     "third_party", "oat_reports", "deps"};
  return kSkippedDirs.count(name) > 0;
}

bool IsCppSourceFile(const std::string &name) {
  static const std::set<std::string> kSuffixes = {".c", ".h", ".cc", ".cpp", ".hpp", ".cxx", ".hxx", ".asc"};
  size_t dot = name.rfind('.');
  return dot != std::string::npos && kSuffixes.count(name.substr(dot)) > 0;
}

bool HasNbspInLicenseHeader(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  std::string head(LICENSE_HEADER_SCAN_BYTES, '\0');
  in.read(&head[0], static_cast<std::streamsize>(LICENSE_HEADER_SCAN_BYTES));
  head.resize(static_cast<size_t>(in.gcount()));
  return head.find(NBSP_UTF8) != std::string::npos;
}

void CollectNbspLicenseHeaders(const std::string &path, std::vector<std::string> &offenders) {
  DIR *dir = opendir(path.c_str());
  if (dir == nullptr) {
    return;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    std::string entry_path = path + "/" + entry->d_name;
    if (IsDirectory(entry_path)) {
      // 递归调用以检查子目录，构建产物与三方代码不在看护范围内
      if (!IsSkippedDir(entry->d_name)) {
        CollectNbspLicenseHeaders(entry_path, offenders);
      }
    } else if (IsRegularFile(entry_path) && IsCppSourceFile(entry->d_name) && HasNbspInLicenseHeader(entry_path)) {
      offenders.push_back(entry_path);
    }
  }
  closedir(dir);
}

void ReportOffenders(const std::vector<std::string> &offenders) {
  for (const std::string &offender : offenders) {
    std::cout << "NBSP(U+00A0) found in license header: " << offender << std::endl;
  }
}
}  // namespace
namespace SC {
/*
 * SC要求每个目录的直属子目录和文件之和不超过50
 * 因此添加这个ut来看护代码仓目录
 */
TEST(FileCount, CheckFileCount) {
  int max_cnt = 0;
  // 待rts_engine整改后取消注释
  // std::string dir0(std::string(TOP_DIR).append("/compiler"));
  // EXPECT_NO_THROW(CountFilesAndDirs(dir0, max_cnt));

  std::string dir1(std::string(TOP_DIR).append("/inc"));
  EXPECT_NO_THROW(CountFilesAndDirs(dir1, max_cnt));

  std::string dir2(std::string(TOP_DIR).append("/parser"));
  EXPECT_NO_THROW(CountFilesAndDirs(dir2, max_cnt));

  std::string dir3(std::string(TOP_DIR).append("/api/session"));
  EXPECT_NO_THROW(CountFilesAndDirs(dir3, max_cnt));

  std::string dir4(std::string(TOP_DIR).append("/runtime"));
  EXPECT_NO_THROW(CountFilesAndDirs(dir4, max_cnt));

  std::string dir5(std::string(TOP_DIR).append("/api/python"));
  EXPECT_NO_THROW(CountFilesAndDirs(dir5, max_cnt));

  std::string dir6(std::string(TOP_DIR).append("/base/common"));
  EXPECT_NO_THROW(CountFilesAndDirs(dir6, max_cnt));

  std::cout << "max_cnt is " << max_cnt << std::endl;
}
}  // namespace SC
namespace LicenseHeader {
/*
 * SC要求许可头行首使用普通空格，历史上曾有 100 个 C/C++ 文件的许可头行首被写成 NBSP(U+00A0)，
 * 导致按原文精确匹配的 OAT/license 扫描识别不出许可声明，因此添加这个ut来看护代码仓许可头
 */
TEST(InvisibleChar, NoNbspInLicenseHeader) {
  std::vector<std::string> offenders;
  CollectNbspLicenseHeaders(std::string(TOP_DIR), offenders);
  ReportOffenders(offenders);
  EXPECT_TRUE(offenders.empty());
}

/*
 * 看护用例自身的检出能力：行首为 NBSP 的许可头必须被抓出，普通空格许可头与不存在的目录不得误报
 */
TEST(InvisibleChar, DetectNbspInLicenseHeader) {
  char dir_template[] = "/tmp/ge_sc_check_XXXXXX";
  ASSERT_NE(mkdtemp(dir_template), nullptr);
  std::string dir(dir_template);
  std::string nbsp_file = dir + "/nbsp_header.h";
  std::string normal_file = dir + "/normal_header.h";
  std::ofstream nbsp_out(nbsp_file, std::ios::binary);
  nbsp_out << "/**\r\n" << NBSP_UTF8 << "* Copyright (c) 2025 Huawei Technologies Co., Ltd.\r\n */\r\n";
  nbsp_out.close();
  std::ofstream normal_out(normal_file, std::ios::binary);
  normal_out << "/**\r\n * Copyright (c) 2025 Huawei Technologies Co., Ltd.\r\n */\r\n";
  normal_out.close();

  std::vector<std::string> offenders;
  CollectNbspLicenseHeaders(dir, offenders);
  ReportOffenders(offenders);
  ASSERT_EQ(offenders.size(), 1UL);
  EXPECT_EQ(offenders[0], nbsp_file);

  std::vector<std::string> absent_offenders;
  CollectNbspLicenseHeaders(dir + "/absent_dir", absent_offenders);
  EXPECT_TRUE(absent_offenders.empty());

  remove(nbsp_file.c_str());
  remove(normal_file.c_str());
  rmdir(dir.c_str());
}
}  // namespace LicenseHeader
