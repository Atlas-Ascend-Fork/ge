# aclopRegisterCompileFunc

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:50:25.061Z pushedAt=2026-10-29T03:53:43.712Z nl=2 -->

## Product Support

<!-- npu="950" id685 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id685 -->
<!-- npu="A3" id686 -->
- Atlas A3 products: Supported
<!-- end id686 -->
<!-- npu="910b" id687 -->
- Atlas A2 products: Supported
<!-- end id687 -->
<!-- npu="310b" id688 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id688 -->
<!-- npu="310p" id689 -->
- Atlas inference products: Supported
<!-- end id689 -->
<!-- npu="910" id690 -->
- Atlas training products: Supported
<!-- end id690 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopRegisterCompileFunc_res.md#id1 -->

## Feature Description

In dynamic shape scenarios, registers an operator selector to select the appropriate tiling strategy for different shapes during operator execution.

If an operator selector has already been registered for an operator, re-registration is not allowed. To change the operator selector, first call [aclopUnregisterCompileFunc](aclopUnregisterCompileFunc.md) to unregister it, and then call aclopRegisterCompileFunc to register a new one.

## Prototype

```c
aclError aclopRegisterCompileFunc(const char *opType, aclopCompileFunc func)
```

## Parameters

| Parameter Name | Input/Output | Description |
| --- | --- | --- |
| opType | Input | Pointer to the operator type. |
| func | Input | Operator selector callback function. |

<br>

The function is defined as follows:

```c
typedef aclError (*aclopCompileFunc)(int numInputs, const aclTensorDesc *const inputDesc[], int numOutputs, const aclTensorDesc *const outputDesc[], const aclopAttr *opAttr, aclopKernelDesc *aclopKernelDesc);
```

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
