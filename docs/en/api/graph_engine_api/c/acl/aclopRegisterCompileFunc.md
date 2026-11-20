# aclopRegisterCompileFunc

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:05:10.672Z pushedAt=2026-11-18T03:42:52.307Z nl=2 -->

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
<!-- npu="IPV350" id691 -->
- IPV350: not supported
<!-- end id691 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopRegisterCompileFunc_res.md#id1 -->

## Function Description

In dynamic shape scenarios, registers an operator selector to select the appropriate tiling strategy for different shapes during operator execution.

If an operator selector has already been registered for an operator, re-registration is not allowed. To change the operator selector, first call [aclopUnregisterCompileFunc](aclopUnregisterCompileFunc.md) to unregister it, and then call aclopRegisterCompileFunc to register a new one.

## Prototype

```c
aclError aclopRegisterCompileFunc(const char *opType, aclopCompileFunc func)
```

## Parameters

| Parameter | Input/Output | Description |
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
