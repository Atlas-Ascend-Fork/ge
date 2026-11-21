/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "gert_source_op_json.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_map>

#include "securec.h"
#include "common/base64.h"
#include "exe_graph/lowering/bg_ir_attrs.h"
#include "exe_graph/runtime/compute_node_info.h"
#include "exe_graph/runtime/expand_dims_type.h"
#include "exe_graph/runtime/shape.h"
#include "framework/common/debug/ge_log.h"
#include "framework/common/json_file.h"
#include "graph/fast_graph/fast_node.h"
#include "graph/fast_graph/gert_graph_attributes.h"
#include "graph/tensor.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_adapter.h"
#include "graph/utils/type_utils.h"
#include "graph_metadef/base/attr/attrs_to_buffer.h"
#include "graph_metadef/graph/debug/ge_util.h"

namespace gert {
namespace {
using Json = ge::JsonFile::json;

void Require(bool condition, const std::string &path, const char *reason) {
  if (!condition) {
    throw std::invalid_argument(path + ": " + reason);
  }
}

std::string ItemPath(const std::string &path, size_t index) {
  return path + "[" + std::to_string(index) + "]";
}

void CheckObject(const Json &value, const std::string &path, std::initializer_list<const char *> required,
                 std::initializer_list<const char *> optional = {}) {
  Require(value.is_object(), path, "expected an object");
  for (const auto *key : required) {
    Require(value.contains(key), path + "." + key, "required field is missing");
  }
  for (auto it = value.begin(); it != value.end(); ++it) {
    const auto matches = [&it](const char *key) { return it.key() == key; };
    Require(std::any_of(required.begin(), required.end(), matches) ||
                std::any_of(optional.begin(), optional.end(), matches),
            path + "." + it.key(), "unknown field");
  }
}

template <typename T>
T ParseInteger(const Json &value, const std::string &path) {
  Require(value.is_number_integer(), path, "expected an integer");
  if (value.is_number_unsigned()) {
    Require(value.get<uint64_t>() <= static_cast<uint64_t>(std::numeric_limits<T>::max()), path,
            "integer out of range");
  } else {
    const auto number = value.get<int64_t>();
    Require((number < 0)
                ? (std::numeric_limits<T>::is_signed && number >= static_cast<int64_t>(std::numeric_limits<T>::min()))
                : (static_cast<uint64_t>(number) <= static_cast<uint64_t>(std::numeric_limits<T>::max())),
            path, "integer out of range");
  }
  return value.get<T>();
}

const std::string &Text(const Json &value, const std::string &path) {
  Require(value.is_string(), path, "expected a string");
  return value.get_ref<const std::string &>();
}

std::string ParseName(const Json &value, const std::string &path) {
  const auto &name = Text(value, path);
  Require(!name.empty() && name.find('\0') == std::string::npos, path, "expected a nonempty name without NUL bytes");
  return name;
}

bool ParseBool(const Json &value, const std::string &path) {
  Require(value.is_boolean(), path, "expected a boolean");
  return value.get<bool>();
}

float ParseFloat(const Json &value, const std::string &path) {
  if (value.is_number()) {
    const auto number = value.get<double>();
    Require(std::abs(number) <= static_cast<double>(std::numeric_limits<float>::max()), path, "float32 out of range");
    return static_cast<float>(number);
  }
  const auto &bits = Text(value, path);
  Require(bits.size() == 10U && bits.compare(0U, 2U, "0x") == 0 &&
              bits.find_first_not_of("0123456789abcdefABCDEF", 2U) == std::string::npos,
          path, "expected 0x followed by eight hexadecimal digits");
  const auto word = static_cast<uint32_t>(std::stoul(bits.substr(2U), nullptr, 16));
  float number = 0.0F;
  static_assert(sizeof(word) == sizeof(number), "float32 width");
  Require(memcpy_s(&number, sizeof(number), &word, sizeof(word)) == EOK, path, "failed to decode float32");
  return number;
}

ge::DataType ParseDtype(const Json &value, const std::string &path) {
  const auto &name = Text(value, path);
  const auto dtype = ge::TypeUtils::SerialStringToDataType(name);
  Require(ge::TypeUtils::DataTypeToSerialString(dtype) == name, path, "unknown DataType name");
  return dtype;
}

ge::Format ParseFormat(const Json &value, const std::string &path) {
  const auto format = ParseInteger<uint32_t>(value, path);
  // GE encodes the primary format, subformat and C0 in the low 28 bits.
  Require(format <= 0x0FFFFFFFU && ge::GetPrimaryFormat(static_cast<int32_t>(format)) < ge::FORMAT_MAX, path,
          "invalid GE format");
  return static_cast<ge::Format>(format);
}

std::string DecodeBytes(const Json &value, const std::string &path) {
  const auto &text = Text(value, path);
  std::string bytes;
  Require(ge::base64::DecodeFromBase64(text, bytes) == ge::SUCCESS && ge::base64::EncodeToBase64(bytes) == text, path,
          "invalid Base64");
  return bytes;
}

std::string ParseBytes(const Json &value, const std::string &path) {
  if (value.is_string()) {
    return Text(value, path);
  }
  CheckObject(value, path, {"base64"});
  return DecodeBytes(value["base64"], path + ".base64");
}

std::string ParseRuntimeString(const Json &value, const std::string &path) {
  auto bytes = ParseBytes(value, path);
  Require(bytes.find('\0') == std::string::npos, path, "RuntimeAttrs strings cannot contain embedded NUL bytes");
  return bytes;
}

template <typename Parse>
auto ParseArray(const Json &value, const std::string &path, Parse parse) -> std::vector<decltype(parse(value, path))> {
  Require(value.is_array(), path, "expected an array");
  std::vector<decltype(parse(value, path))> result;
  result.reserve(value.size());
  for (size_t i = 0U; i < value.size(); ++i) {
    result.emplace_back(parse(value[i], ItemPath(path, i)));
  }
  return result;
}

void ParsePort(const Json &value, const std::string &path, CompileTimeTensorDesc &desc) {
  CheckObject(value, path, {"dtype", "origin_format", "storage_format"}, {"expand_dims", "exists"});
  desc.SetDataType(ParseDtype(value["dtype"], path + ".dtype"));
  desc.SetOriginFormat(ParseFormat(value["origin_format"], path + ".origin_format"));
  desc.SetStorageFormat(ParseFormat(value["storage_format"], path + ".storage_format"));
  if (value.contains("expand_dims")) {
    const auto dims = Text(value["expand_dims"], path + ".expand_dims");
    Require(dims.size() <= ExpandDimsType::kMaxExpandSize && dims.find_first_not_of("01") == std::string::npos,
            path + ".expand_dims", "expected up to 56 binary digits");
    desc.SetExpandDimsType(ExpandDimsType(dims.c_str()));
  }
  desc.SetExist(!value.contains("exists") || ParseBool(value["exists"], path + ".exists"));
}

void ParseMapping(const Json &value, const std::string &path, size_t ports, AnchorInstanceInfo &info) {
  CheckObject(value, path, {"start", "count"});
  const auto start = ParseInteger<size_t>(value["start"], path + ".start");
  const auto count = ParseInteger<size_t>(value["count"], path + ".count");
  Require(start <= ports, path + ".start", "start exceeds actual port count");
  Require(count <= ports - start, path + ".count", "mapping exceeds actual port count");
  info.SetInstanceStart(start);
  info.SetInstantiationNum(count);
}

std::vector<int64_t> ParseShape(const Json &value, const std::string &path) {
  Require(value.is_array() && value.size() <= Shape::kMaxDimNum, path, "invalid tensor rank");
  return ParseArray(value, path, ParseInteger<int64_t>);
}

ge::GeTensor ParseTensor(const Json &value, const std::string &path) {
  Require(value.is_object() && value.contains("dtype"), path + ".dtype", "required field is missing");
  const auto dtype = ParseDtype(value["dtype"], path + ".dtype");
  const bool strings = dtype == ge::DT_STRING;
  const auto *payload = strings ? "strings" : "data_base64";
  CheckObject(value, path, {"dtype", "origin_shape", "storage_shape", "origin_format", "storage_format", payload});
  const auto origin = ParseShape(value["origin_shape"], path + ".origin_shape");
  const auto storage = ParseShape(value["storage_shape"], path + ".storage_shape");
  for (size_t i = 0U; i < origin.size(); ++i) {
    Require(origin[i] >= 0, ItemPath(path + ".origin_shape", i), "negative tensor dimension");
  }
  int64_t elements = std::find(storage.begin(), storage.end(), 0) == storage.end() ? 1 : 0;
  for (size_t i = 0U; i < storage.size(); ++i) {
    const auto dim = storage[i];
    Require(dim >= 0 && (dim == 0 || elements <= std::numeric_limits<int64_t>::max() / dim),
            ItemPath(path + ".storage_shape", i), "invalid tensor dimension or element count overflow");
    elements *= dim;
  }
  ge::GeTensorDesc desc(ge::GeShape(storage), ParseFormat(value["storage_format"], path + ".storage_format"), dtype);
  desc.SetOriginShape(ge::GeShape(origin));
  desc.SetOriginFormat(ParseFormat(value["origin_format"], path + ".origin_format"));
  const auto data_path = path + "." + payload;
  if (strings) {
    const auto data = ParseArray(value[payload], data_path, ParseBytes);
    Require(data.size() == static_cast<uint64_t>(elements), data_path, "string element count disagrees with shape");
    std::vector<ge::AscendString> values;
    values.reserve(data.size());
    for (const auto &str : data) {
      values.emplace_back(str.data(), str.size());
    }
    ge::Tensor tensor(ge::TensorAdapter::GeTensorDesc2TensorDesc(desc));
    Require(values.empty() || tensor.SetData(values) == ge::GRAPH_SUCCESS, data_path, "failed to encode string tensor");
    return ge::TensorAdapter::AsGeTensor(tensor);
  }
  const auto data = DecodeBytes(value[payload], data_path);
  const auto bytes = ge::GetSizeInBytes(elements, dtype);
  Require(bytes >= 0 && data.size() == static_cast<uint64_t>(bytes), data_path,
          "tensor byte count disagrees with shape or dtype");
  return ge::GeTensor(desc, reinterpret_cast<const uint8_t *>(data.data()), data.size());
}

template <typename T>
ge::AnyValue ParseScalarAttr(const Json &value, const std::string &path,
                             T (*parse)(const Json &, const std::string &)) {
  return ge::AnyValue::CreateFrom<T>(parse(value, path));
}

template <typename T>
ge::AnyValue ParseListAttr(const Json &value, const std::string &path, T (*parse)(const Json &, const std::string &)) {
  return ge::AnyValue::CreateFrom<std::vector<T>>(ParseArray(value, path, parse));
}

template <typename T, T (*Parse)(const Json &, const std::string &)>
std::vector<T> ParseList(const Json &value, const std::string &path) {
  return ParseArray(value, path, Parse);
}

ge::AnyValue ParseAttr(const Json &attr, const std::string &path) {
  static const std::unordered_map<std::string, ge::AnyValue::ValueType> kAttrTypes = {
      {"bool", ge::AnyValue::VT_BOOL},
      {"int64", ge::AnyValue::VT_INT},
      {"float32", ge::AnyValue::VT_FLOAT},
      {"string", ge::AnyValue::VT_STRING},
      {"dtype", ge::AnyValue::VT_DATA_TYPE},
      {"tensor", ge::AnyValue::VT_TENSOR},
      {"list<bool>", ge::AnyValue::VT_LIST_BOOL},
      {"list<int64>", ge::AnyValue::VT_LIST_INT},
      {"list<float32>", ge::AnyValue::VT_LIST_FLOAT},
      {"list<string>", ge::AnyValue::VT_LIST_STRING},
      {"list<dtype>", ge::AnyValue::VT_LIST_DATA_TYPE},
      {"list<list<int64>>", ge::AnyValue::VT_LIST_LIST_INT},
      {"list<list<float32>>", ge::AnyValue::VT_LIST_LIST_FLOAT},
  };
  CheckObject(attr, path, {"name", "type", "value"});
  (void)ParseName(attr["name"], path + ".name");
  const auto type = Text(attr["type"], path + ".type");
  const auto &value = attr["value"];
  const auto item = path + ".value";
  const auto it = kAttrTypes.find(type);
  switch (it == kAttrTypes.end() ? ge::AnyValue::VT_NONE : it->second) {
    case ge::AnyValue::VT_BOOL:
      return ParseScalarAttr(value, item, ParseBool);
    case ge::AnyValue::VT_INT:
      return ParseScalarAttr(value, item, ParseInteger<int64_t>);
    case ge::AnyValue::VT_FLOAT:
      return ParseScalarAttr(value, item, ParseFloat);
    case ge::AnyValue::VT_STRING:
      return ParseScalarAttr(value, item, ParseRuntimeString);
    case ge::AnyValue::VT_DATA_TYPE:
      return ParseScalarAttr(value, item, ParseDtype);
    case ge::AnyValue::VT_TENSOR:
      return ParseScalarAttr(value, item, ParseTensor);
    case ge::AnyValue::VT_LIST_BOOL:
      return ParseListAttr(value, item, ParseBool);
    case ge::AnyValue::VT_LIST_INT:
      return ParseListAttr(value, item, ParseInteger<int64_t>);
    case ge::AnyValue::VT_LIST_FLOAT:
      return ParseListAttr(value, item, ParseFloat);
    case ge::AnyValue::VT_LIST_STRING:
      return ParseListAttr(value, item, ParseRuntimeString);
    case ge::AnyValue::VT_LIST_DATA_TYPE:
      return ParseListAttr(value, item, ParseDtype);
    case ge::AnyValue::VT_LIST_LIST_INT:
      return ParseListAttr(value, item, ParseList<int64_t, ParseInteger<int64_t>>);
    case ge::AnyValue::VT_LIST_LIST_FLOAT:
      return ParseListAttr(value, item, ParseList<float, ParseFloat>);
    default:
      throw std::invalid_argument(path + ".type: unsupported attribute type " + type);
  }
}
void FillPorts(const Json &root, ComputeNodeInfo &info, bool input) {
  const std::string port_path = input ? "$.inputs" : "$.outputs";
  const std::string ir_path = input ? "$.ir_inputs" : "$.ir_outputs";
  const auto &ports = root[input ? "inputs" : "outputs"];
  const auto &mapping = root[input ? "ir_inputs" : "ir_outputs"];
  for (size_t i = 0U; i < ports.size(); ++i) {
    auto *desc = input ? info.MutableInputTdInfo(i) : info.MutableOutputTdInfo(i);
    ParsePort(ports[i], ItemPath(port_path, i), *desc);
  }
  for (size_t i = 0U; i < mapping.size(); ++i) {
    auto *instance = input ? info.MutableInputInstanceInfo(i) : info.MutableOutputInstanceInfo(i);
    ParseMapping(mapping[i], ItemPath(ir_path, i), ports.size(), *instance);
  }
}
}  // namespace

ge::graphStatus BuildComputeNodeInfoFromSourceOp(const ge::FastNode &node, bg::BufferPool &pool,
                                                 std::unique_ptr<uint8_t[]> &data, size_t &size) {
  std::string text;
  try {
    Require(ge::AttrUtils::GetStr(node.GetOpDescBarePtr(), attr::kSourceOp, text), "$",
            "expected a string node attribute");
    const ge::JsonFile document(reinterpret_cast<const uint8_t *>(text.data()), text.size());
    Require(document.IsValid(), "$", "invalid JSON text");
    const auto &root = document.Raw();
    CheckObject(root, "$", {"version", "name", "type", "inputs", "outputs", "ir_inputs", "ir_outputs", "attrs"});
    Require(ParseInteger<int64_t>(root["version"], "$.version") == 1, "$.version", "unsupported version");
    const auto name = ParseName(root["name"], "$.name");
    const auto type = ParseName(root["type"], "$.type");
    for (const auto *field : {"inputs", "outputs", "ir_inputs", "ir_outputs", "attrs"}) {
      Require(root[field].is_array(), std::string("$.") + field, "expected an array");
    }

    std::vector<std::vector<uint8_t>> attr_buffers;
    const auto &attrs = root["attrs"];
    attr_buffers.reserve(attrs.size());
    for (size_t i = 0U; i < attrs.size(); ++i) {
      const auto path = ItemPath("$.attrs", i);
      const auto attr_value = ParseAttr(attrs[i], path);
      const auto *tensor = attr_value.Get<ge::GeTensor>();
      const bool appended = tensor != nullptr
                                ? bg::AppendAttrTensorWithSize(*tensor, tensor->GetData().GetSize(), attr_buffers)
                                : bg::AppendAttr(attr_value, attr_buffers);
      Require(appended, path + ".value", "failed to encode runtime attribute");
    }
    size_t attrs_size = 0U;
    const auto attrs_data = bg::CreateAttrBuffer(attr_buffers, attrs_size);
    Require(attrs_data != nullptr, "$.attrs", "failed to build runtime attribute buffer");

    const auto inputs = root["inputs"].size();
    const auto outputs = root["outputs"].size();
    const auto ir_inputs = root["ir_inputs"].size();
    const auto ir_outputs = root["ir_outputs"].size();
    size_t base_size = 0U;
    size_t total_size = 0U;
    Require(ComputeNodeInfo::CalcSize(ir_inputs, ir_outputs, inputs, outputs, base_size) == ge::GRAPH_SUCCESS &&
                !ge::AddOverflow(base_size, attrs_size, total_size),
            "$", "ComputeNodeInfo size overflow");
    auto holder = ge::ComGraphMakeUnique<uint8_t[]>(total_size);
    Require(holder != nullptr, "$", "failed to allocate ComputeNodeInfo");
    std::fill_n(holder.get(), total_size, uint8_t{0});
    auto *info = reinterpret_cast<ComputeNodeInfo *>(holder.get());
    info->Init(ir_inputs, ir_outputs, inputs, outputs, attrs_size, nullptr, nullptr);
    FillPorts(root, *info, true);
    FillPorts(root, *info, false);
    Require(
        attrs_size == 0U || memcpy_s(info->MutableAttrs(), total_size - base_size, attrs_data.get(), attrs_size) == EOK,
        "$.attrs", "failed to copy runtime attributes");
    info->SetNodeName(reinterpret_cast<const char *>(pool.AddStr(name.c_str())));
    info->SetNodeType(reinterpret_cast<const char *>(pool.AddStr(type.c_str())));
    size = total_size;
    data = std::move(holder);
    return ge::GRAPH_SUCCESS;
  } catch (const Json::exception &error) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "%s %s $: %s", attr::kSourceOp, node.GetNamePtr(), error.what());
  } catch (const std::invalid_argument &error) {
    GELOGE(ge::GRAPH_PARAM_INVALID, "%s %s %s", attr::kSourceOp, node.GetNamePtr(), error.what());
  }
  return ge::GRAPH_PARAM_INVALID;
}
}  // namespace gert
