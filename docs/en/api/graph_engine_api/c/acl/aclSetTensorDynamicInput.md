# aclSetTensorDynamicInput

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T16:01:21.631Z pushedAt=2026-10-29T03:59:28.899Z nl=2 -->

## Product Support

<!-- npu="950" id22 -->
- Ascend 950PR&950DT products: Supported
<!-- end id22 -->
<!-- npu="A3" id23 -->
- Atlas A3 products: Supported
<!-- end id23 -->
<!-- npu="910b" id24 -->
- Atlas A2 products: Supported
<!-- end id24 -->
<!-- npu="310b" id25 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id25 -->
<!-- npu="310p" id26 -->
- Atlas inference products: Supported
<!-- end id26 -->
<!-- npu="910" id27 -->
- Atlas training products: Supported
<!-- end id27 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorDynamicInput_res.md#id1 -->

## Feature Description

After you call [aclCreateTensorDesc](aclCreateTensorDesc.md) to create the tensor description, you can call aclSetTensorDynamicInput to set the group identifier for multiple dynamic inputs. For example, an operator has dynamic input x and required input y, where there are multiple dynamic inputs. Call aclSetTensorDynamicInput to set **dynamicInputName** to x. The API then indexes the corresponding multiple dynamic inputs by **dynamicInputName** to group them**.**

## Prototype

```c
aclError aclSetTensorDynamicInput(aclTensorDesc *desc, const char *dynamicInputName)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |
| dynamicInputName | Input | Pointer to the dynamic input group identifier in desc. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
