# aclopLoad

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:50:15.913Z pushedAt=2026-10-29T03:53:37.549Z nl=2 -->

## Product Support

<!-- npu="950" id797 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id797 -->
<!-- npu="A3" id798 -->
- Atlas A3 products: Supported
<!-- end id798 -->
<!-- npu="910b" id799 -->
- Atlas A2 products: Supported
<!-- end id799 -->
<!-- npu="310b" id800 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id800 -->
<!-- npu="310p" id801 -->
- Atlas inference products: Supported
<!-- end id801 -->
<!-- npu="910" id802 -->
- Atlas training products: Supported
<!-- end id802 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopLoad_res.md#id1 -->

## Feature Description

Loads single-operator model data from memory (single-operator model data refers to the data obtained by compiling a single operator into a \*.om file and then reading the om file into memory). The memory is managed by you.

## Prototype

```c
aclError aclopLoad(const void *model,  size_t modelSize)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| model | Input | Memory address pointer to the single-operator model data. |
| modelSize | Input | Length of the model data in memory, in bytes. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

The operator library version in the model loading environment must be consistent with that in the model compilation environment; otherwise, an error is reported when loading operators. You can view the operator library version through the version field in the \$\{INSTALL\_DIR\}/opp/version.info file. \$\{INSTALL\_DIR\} must be replaced with the file storage path after CANN software installation. Taking root user installation as an example, the default file storage path after installation is /usr/local/Ascend/cann.

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
Before loading, evaluate whether the memory space is sufficient based on the size of the single-operator .om file. Insufficient memory space causes application exceptions. The maximum number of operators being executed concurrently in a process varies by product model:
<!-- end id1 -->

<!-- npu="950,A3,910b" id2 -->
- For Ascend 950PR&950DT products, Atlas A3 products, and Atlas A2 products, the maximum number is 2000000.
<!-- end id2 -->
<!-- npu="910,310b" id3 -->
- For Atlas 200I/500 A2 inference product and Atlas training products, the maximum number is 40000000.
<!-- end id3 -->
<!-- npu="310p" id4 -->
- For Atlas inference products, the upper limit is 40000000 in Ascend EP form, and 2000000 in Control CPU open form.
<!-- end id4 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopLoad_res.md#id2 -->
