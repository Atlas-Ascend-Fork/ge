# aclopCreateKernel

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:46:31.629Z pushedAt=2026-10-29T03:43:21.047Z nl=2 -->

## Product Support

<!-- npu="950" id64 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id64 -->
<!-- npu="A3" id65 -->
- Atlas A3 products: Supported
<!-- end id65 -->
<!-- npu="910b" id66 -->
- Atlas A2 products: Supported
<!-- end id66 -->
<!-- npu="310b" id67 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id67 -->
<!-- npu="310p" id68 -->
- Atlas inference products: Supported
<!-- end id68 -->
<!-- npu="910" id69 -->
- Atlas training products: Supported
<!-- end id69 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopCreateKernel_res.md#id1 -->

## Feature Description

In dynamic shape scenarios, registers the operator with the system for use when running the operator.

## Prototype

```c
aclError aclopCreateKernel(const char *opType,
const char *kernelId,
const char *kernelName,
void *binData,
int binSize,
aclopEngineType enginetype,
aclDataDeallocator deallocator)
```

## Parameters

| parameter name | Input/Output | Description |
| --- | --- | --- |
| opType | Input | Pointer to the operator type. |
| kernelId | Input | Pointer to the kernel ID to be specified when the operator is executed. |
| kernelName | Input | Pointer to the operator kernel name, which must be consistent with the kernelName in the operator binary file. |
| binData | Input | Pointer to the memory address of the operator kernel file. |
| binSize | Input | Memory size of the operator kernel file, in bytes. |
| enginetype | Input | Operator execution engine. For the type definition, see [aclopEngineType](aclopEngineType.md). |
| deallocator | Input | Callback function for releasing the binData memory. The caller sets the corresponding release function based on how binData is constructed. Set this parameter to null if the data is released by the caller. |

<br>

The callback function prototype is as follows:

```c
typedef void (*aclDataDeallocator)(void *data, size_t length);
```

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
