/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <gtest/gtest.h>
#include <set>
#include <string>
#include <vector>
#include "common/convert/pb2json.h"
#include "proto/tensorflow/node_def.pb.h"

namespace ge {
class STestPb2Json : public testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(STestPb2Json, DictInitMalformedValues) {
  std::vector<std::string> idx2name;
  std::vector<std::string> idx2value;
  std::vector<bool> use_string_val;

  Json json;
  json["attr"] = Json::array();

  Json item1;
  item1["key"] = "attr_name_enum";
  item1["value"] = "not_an_object";
  json["attr"].push_back(item1);

  Json item2;
  item2["key"] = "attrs_use_string_value";
  item2["value"] = 123;
  json["attr"].push_back(item2);

  Json item3;
  item3["key"] = "attr_value_enum";
  item3["value"]["list"] = "not_an_object";
  json["attr"].push_back(item3);

  Json item4;
  item4["key"] = "attrs_use_string_value";
  item4["value"]["list"] = 5;
  json["attr"].push_back(item4);

  Json item5;
  item5["key"] = "attr_name_enum";
  item5["value"]["list"]["s"] = 7;
  json["attr"].push_back(item5);

  Json item6;
  item6["key"] = "attrs_use_string_value";
  item6["value"]["list"]["b"] = 7;
  json["attr"].push_back(item6);

  EXPECT_EQ(Pb2Json::DictInit(json, idx2name, idx2value, use_string_val), 0);
  EXPECT_TRUE(idx2name.empty());
  EXPECT_TRUE(use_string_val.empty());
}

TEST_F(STestPb2Json, Message2JsonNodeDef) {
  domi::tensorflow::NodeDef node_def;
  node_def.set_name("st_pb2json_node");
  node_def.set_op("Add");
  (*node_def.mutable_attr())["T"].set_i(1);

  std::set<std::string> black_fields;
  Json json;
  Pb2Json::Message2Json(node_def, black_fields, json, false, 0);
  EXPECT_EQ(json["name"], "st_pb2json_node");
  EXPECT_EQ(json["op"], "Add");
}

// 覆盖率补充用例：覆盖 DictInit 中压缩版本 / 枚举名 / 枚举值 / string 取值标志的正常取值路径。
TEST_F(STestPb2Json, DictInitWellFormedValues) {
  std::vector<std::string> idx2name;
  std::vector<std::string> idx2value;
  std::vector<bool> use_string_val;

  Json json;
  json["attr"] = Json::array();

  Json item_version;
  item_version["key"] = "om_compress_version";
  item_version["value"]["i"] = 1;
  json["attr"].push_back(item_version);

  Json item_name;
  item_name["key"] = "attr_name_enum";
  Json name_list;
  name_list["s"] = Json::array({"name_a", "name_b"});
  item_name["value"]["list"] = name_list;
  json["attr"].push_back(item_name);

  Json item_value;
  item_value["key"] = "attr_value_enum";
  Json value_list;
  value_list["s"] = Json::array({"value_a"});
  item_value["value"]["list"] = value_list;
  json["attr"].push_back(item_value);

  Json item_flag;
  item_flag["key"] = "attrs_use_string_value";
  Json flag_list;
  flag_list["b"] = Json::array({true, false});
  item_flag["value"]["list"] = flag_list;
  json["attr"].push_back(item_flag);

  EXPECT_EQ(Pb2Json::DictInit(json, idx2name, idx2value, use_string_val), 1);
  EXPECT_EQ(idx2name.size(), 2U);
  EXPECT_EQ(idx2value.size(), 1U);
  EXPECT_EQ(use_string_val.size(), 2U);
  EXPECT_TRUE(json["attr"].empty());
}

}  // namespace ge
