# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T13:08:52.660Z pushedAt=2026-10-28T07:40:22.474Z nl=2 -->

A graph builder class for C++ users to build computation graphs. This class provides methods for creating graph elements such as inputs, as well as functions for setting private attributes, building the final graph, and serving the inline functions of the C++ ES generated API. These inline functions internally call the C ES generated API conversion logic.

## Header File to Include

```c++
#include "es_graph_builder.h"
```

## Public Member Functions

```c++
explicit EsGraphBuilder(const char *name);
template <typename T>
std::unique_ptr<Tensor> CreateTensor(const std::vector<T> &value, const std::vector<int64_t> &dims, DataType dt,Format format = FORMAT_ND);
std::unique_ptr<Tensor> CreateBoolTensor(const std::vector<bool> &value, const std::vector<int64_t> &dims, Format format = FORMAT_ND);
EsTensorHolder CreateInput(int64_t index, const char *name, const char *type);
template <size_t inputs_num>
std::array<EsTensorHolder, inputs_num> CreateInputs(size_t start_index = 0);
std::vector<EsTensorHolder> CreateInputs(size_t inputs_num, size_t start_index = 0);
EsTensorHolder CreateInput(int64_t index);
EsTensorHolder CreateInput(int64_t index, const char *name);
EsTensorHolder CreateInput(int64_t index, const char *name, ge::DataType data_type, ge::Format format, const std::vector<int64_t> &shape);
EsTensorHolder CreateConst(const std::vector<int64_t> &value, const std::vector<int64_t> dims);
EsTensorHolder CreateConst(const std::vector<int32_t> &value, const std::vector<int64_t> dims);
EsTensorHolder CreateConst(const std::vector<uint64_t> &value, const std::vector<int64_t> dims);
EsTensorHolder CreateConst(const std::vector<uint32_t> &value, const std::vector<int64_t> dims);
EsTensorHolder CreateConst(const std::vector<float> &value, const std::vector<int64_t> dims);
template <typename T>
EsTensorHolder CreateConst(const std::vector<T> &value, const std::vector<int64_t> &dims, ge::DataType dt,ge::Format format = FORMAT_ND);
template <typename T>
EsTensorHolder CreateConstV2(const std::vector<T> &value, const std::vector<int64_t> &dims, ge::DataType dt, ge::Format format = FORMAT_ND)
EsTensorHolder CreateVector(const std::vector<int64_t> &value);
EsTensorHolder CreateVector(const std::vector<int32_t> &value);
EsTensorHolder CreateVector(const std::vector<uint64_t> &value);
EsTensorHolder CreateVector(const std::vector<uint32_t> &value);
EsTensorHolder CreateVector(const std::vector<float> &value);
EsTensorHolder CreateScalar(int64_t value);
EsTensorHolder CreateScalar(int32_t value);
EsTensorHolder CreateScalar(uint64_t value);
EsTensorHolder CreateScalar(uint32_t value);
EsTensorHolder CreateScalar(float value);
EsTensorHolder CreateVariable(int32_t index, const char *name);
Status SetAttr(const char *attr_name, int64_t value);
Status SetAttr(const char *attr_name, const char *value);
Status SetAttr(const char *attr_name, bool value);
static int64_t SetOutput(const EsTensorHolder &tensor, int64_t index);
std::unique_ptr<Graph> BuildAndReset() const;
std::unique_ptr<Graph> BuildAndReset(const std::vector<EsTensorHolder> &outputs);
EsCGraphBuilder *GetCGraphBuilder() const;

```

## External Functions

```c++
template <typename T>
std::unique_ptr<Tensor> CreateTensor(const T *value, const int64_t *dims, int64_t dim_num, DataType dt, Format format = FORMAT_ND);
template <typename T>
std::unique_ptr<Tensor> CreateTensorFromFile(const char *data_file_path, const std::vector<int64_t> &dims, DataType dt, Format format = FORMAT_ND);
template <typename T>
EsCTensorHolder *EsCreateConst(EsCGraphBuilder *graph, const T *value, const int64_t *dims, int64_t dim_num, ge::DataType dt, ge::Format format = FORMAT_ND);
template <typename T>
EsCTensorHolder *EsCreateConstV2(EsCGraphBuilder *graph, const T *value, const int64_t *dims, int64_t dim_num, ge::DataType dt, ge::Format format = FORMAT_ND)
inline std::vector<EsCTensorHolder *> TensorsToEsCTensorHolders(const std::vector<EsTensorHolder> &tensors)
inline std::vector<C_DataType> DataTypesToEsCDataTypes(const std::vector<ge::DataType> &data_types)
template <typename T>
std::pair<std::vector<const T *>, std::vector<int64_t>> ListListTypeToPtrAndCounts(const std::vector<std::vector<T>> &list_list_type)
inline std::vector<EsCGraph *> GeGraphsToEsCGraphs(std::vector<std::unique_ptr<ge::Graph>> graphs)
```
