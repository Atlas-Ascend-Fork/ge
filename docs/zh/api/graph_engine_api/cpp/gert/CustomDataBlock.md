# CustomDataBlock

## 产品支持情况

全量芯片支持。

## 头文件/库文件

- 头文件：\#include <exe\_graph/runtime/annotated\_args\_context.h\>
- 库文件：liblowering.so

## 功能说明

`gert::CustomDataBlock`是变长自定义数据描述符，作为[`AnnotatedKernelArgs::AppendArg`](AnnotatedKernelArgs/AppendArg.md)的参数，用于向kernel args中追加一段长度不固定的只读字节数据，典型场景为tiling数据、常量查找表与描述符数组。

数据随模型下发，在模型加载期一次性上传至device；kernel args中仅占用一个8-byte槽位，槽值为该数据的device地址，kernel侧需要从device地址读取实际自定义数据。

`CustomDataBlock`与`CustomValue`（即`AppendArg(uint64_t)`）的区别：`CustomValue`把8-byte数值直接内联在kernel args的槽位中，kernel按值接收、无需解引用；`CustomDataBlock`的槽位是该数据的device地址，kernel需按地址解引用读取，数据只读，且长度不随kernel args下发。

## 函数原型

```c++
namespace gert {
struct CustomDataBlock {
  const void *data;    // 数据起始地址（host），追加时深拷贝，返回后调用方内存可立即修改或释放
  size_t size;         // 数据长度，单位为字节，必须大于0
  // 二进制兼容保留字段，增加字段时缩减本数组长度以保持sizeof(CustomDataBlock)不变；
  // 当前版本必须全部置0
  uint8_t reserved[8] = {0U};
};
}  // namespace gert
```

## 参数说明

| 参数名 | 类型 | 说明 |
| --- | --- | --- |
| `data` | `const void *` | 数据起始地址，指向host侧内存。该内存在`AppendArg`返回前已被深拷贝，返回后调用方可以立即修改或释放。`size`大于0时不得为空。 |
| `size` | `size_t` | 数据长度，单位为byte，必须大于0。该长度仅用于host侧的内存分配与数据上传，不会写入kernel args，也不随kernel args下发给kernel。 |
| `reserved` | `uint8_t[8]` | 二进制兼容保留字段，当前版本必须全部置0，框架暂不使用；非0时`AppendArg`返回`GRAPH_FAILED`。后续启用预留字段时新增字段会缩减本数组长度，以保持`sizeof(CustomDataBlock)`不变。 |

## 返回值说明

不涉及。

## 约束说明

- `data`指向的字节在追加时深拷贝、编译期固化，执行期只读；需要执行期改写内容的场景请使用[`AnnotatedArgsContext::MallocWorkSpace`](AnnotatedArgsContext/MallocWorkSpace.md)。
- `size`不随kernel args下发，kernel侧需按数据形态自行确定长度（三种方式的写法见「调用示例」）：
  - 定长数据：host侧与kernel侧共用同一份结构体定义，kernel侧按`sizeof(结构体)`还原；
  - 变长数据（推荐）：首个字段或头部结构体写入magic与总长度（或条目数），kernel侧先校验再读取；
  - 长度槽：在`AppendArg(const CustomDataBlock &)`之后再调用`AppendArg(uint64_t{size})`，kernel侧额外接收一个长度参数，多占用一个8-byte槽位。
- host侧与kernel侧必须使用同源的数据布局定义（同一头文件、同一编译选项），否则长度与字段偏移不一致。
- 当前版本中各数据块（blob）的device地址按512-byte对齐，blob之间及末尾的填充字节为0；框架不做越界访问校验，kernel侧必须依据长度字段限定访问范围。越过blob末尾时读到的是填充0，但越过最后一个blob的填充区后属于非法访存（可能读到其他数据或触发异常）。
- 单个blob与单次launch的累计数据量均不得超过`UINT32_MAX`；单次launch的args槽位数不得超过8192。
- 以下情形不支持变长自定义数据，编译期显式报错：
  - 端侧离线模型（.omc）；
  - OM2 模型。
- `reserved`必须全部置0。该字段为后续版本扩展预留，非0时`AppendArg`返回`GRAPH_FAILED`，避免历史脏值在后续版本被误读为有效语义。

## 调用示例

`size`不随kernel args下发，按数据形态选择以下任一方式。

### 定长数据：共用结构体

```c++
struct MyTiling {
  uint32_t totalLength;  // 数据总长度，供kernel侧校验
  uint32_t tileNum;
};

const auto *input = ctx.GetInputTensor(0U);
const auto *output = ctx.GetOutputTensor(0U);
if ((input == nullptr) || (output == nullptr)) {
  return ge::GRAPH_FAILED;
}

MyTiling tiling{static_cast<uint32_t>(sizeof(MyTiling)), 8U};
gert::AnnotatedKernelArgs args;
if (args.AppendArg(gert::InputAddr{0U, input->GetAddr()}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(gert::CustomDataBlock{&tiling, sizeof(tiling)}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(gert::OutputAddr{0U, output->GetAddr()}) != ge::GRAPH_SUCCESS) {
  return ge::GRAPH_FAILED;
}
// args按上述顺序占用3个8-byte槽位，其中第2个槽位的值为tiling数据的device地址。
return ctx.AddLaunch(launch_info, std::move(args));
```

对应的kernel入口函数共有3个参数，第2个参数接收该数据的device地址：

```c++
extern "C" __global__ __aicore__ void my_kernel(GM_ADDR x, GM_ADDR tiling, GM_ADDR y) {
  // 长度不随args下发，按host侧与device侧共用的结构体定义还原数据，并先校验长度再读取。
  __gm__ const MyTiling *const tilingData = reinterpret_cast<__gm__ const MyTiling *>(tiling);
  if (tilingData->totalLength != static_cast<uint32_t>(sizeof(MyTiling))) {
    return;
  }
  // 校验通过后按tilingData->tileNum访问其余数据。
}
```

### 变长数据：自描述头部

数据首部写入magic与条目数，kernel侧先校验头部、再按数据自身携带的长度定位变长区：

```c++
constexpr uint32_t kTableMagic = 0x5A5A0000U;
constexpr uint32_t kMaxTableLen = 16U;  // 条目数上限，host侧构造时须保证不超过该值
struct MyTableHeader {
  uint32_t magic;     // 供kernel侧校验数据布局
  uint32_t tableLen;  // 变长区条目数
};

const auto *input = ctx.GetInputTensor(0U);
if (input == nullptr) {
  return ge::GRAPH_FAILED;
}

const std::vector<int32_t> table = {1, 2, 3, 4, 5, 6};  // 条目数由业务决定，编译期固化
std::vector<uint8_t> blob(sizeof(MyTableHeader) + table.size() * sizeof(int32_t), 0U);
auto *const header = reinterpret_cast<MyTableHeader *>(blob.data());
header->magic = kTableMagic;
header->tableLen = static_cast<uint32_t>(table.size());
std::copy(table.begin(), table.end(), reinterpret_cast<int32_t *>(header + 1));

gert::AnnotatedKernelArgs args;
if (args.AppendArg(gert::InputAddr{0U, input->GetAddr()}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(gert::CustomDataBlock{blob.data(), blob.size()}) != ge::GRAPH_SUCCESS) {
  return ge::GRAPH_FAILED;
}
return ctx.AddLaunch(launch_info, std::move(args));
```

对应的kernel入口函数第2个参数接收该数据的device地址，变长区位置由头部信息推导：

```c++
extern "C" __global__ __aicore__ void my_kernel(GM_ADDR x, GM_ADDR table) {
  // 先校验头部（kTableMagic/kMaxTableLen来自host与kernel共用的头文件），再用blob自带的条目数访问变长区。
  __gm__ const MyTableHeader *const header = reinterpret_cast<__gm__ const MyTableHeader *>(table);
  if ((header->magic != kTableMagic) || (header->tableLen > kMaxTableLen)) {
    return;
  }
  __gm__ const int32_t *const values = reinterpret_cast<__gm__ const int32_t *>(header + 1);
  // 按values[0 .. header->tableLen)访问变长区。
}
```

### 长度槽：追加uint64_t参数

数据本身不带长度字段时，在`CustomDataBlock`之后再追加一个`uint64_t`长度槽（额外占用一个8-byte槽位），kernel侧多接收一个长度参数：

```c++
struct MyParams {  // host侧与device侧共用的结构体定义
  uint32_t tileNum;
  uint32_t blockNum;
};

const auto *input = ctx.GetInputTensor(0U);
const auto *output = ctx.GetOutputTensor(0U);
if ((input == nullptr) || (output == nullptr)) {
  return ge::GRAPH_FAILED;
}

MyParams params{8U, 8U};
gert::AnnotatedKernelArgs args;
if (args.AppendArg(gert::InputAddr{0U, input->GetAddr()}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(gert::CustomDataBlock{&params, sizeof(params)}) != ge::GRAPH_SUCCESS ||
    args.AppendArg(uint64_t{sizeof(params)}) != ge::GRAPH_SUCCESS ||  // 长度槽
    args.AppendArg(gert::OutputAddr{0U, output->GetAddr()}) != ge::GRAPH_SUCCESS) {
  return ge::GRAPH_FAILED;
}
// args共4个槽位，依次为输入地址、params地址、params长度、输出地址。
return ctx.AddLaunch(launch_info, std::move(args));
```

```c++
extern "C" __global__ __aicore__ void my_kernel(GM_ADDR x, GM_ADDR params, uint64_t paramsLen, GM_ADDR y) {
  // 长度由独立槽位传入，数据本身无需携带长度字段。
  __gm__ const MyParams *const paramsData = reinterpret_cast<__gm__ const MyParams *>(params);
  if (paramsLen != sizeof(MyParams)) {
    return;
  }
  // 按paramsData->tileNum与paramsData->blockNum访问。
}
```
