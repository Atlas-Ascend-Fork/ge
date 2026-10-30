# aclmdlGetOpAttr

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:20:46.925Z pushedAt=2026-10-29T03:19:23.864Z nl=2 -->

## Product Support

<!-- npu="950" id1007 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1007 -->
<!-- npu="A3" id1008 -->
- Atlas A3 products: Supported
<!-- end id1008 -->
<!-- npu="910b" id1009 -->
- Atlas A2 products: Supported
<!-- end id1009 -->
<!-- npu="310b" id1010 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id1010 -->
<!-- npu="310p" id1011 -->
- Atlas inference products: Supported
<!-- end id1011 -->
<!-- npu="910" id1012 -->
- Atlas training products: Supported
<!-- end id1012 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclmdlGetOpAttr_res.md#id1 -->

## Feature Description

Obtains the value of an attribute of an operator in a model in the entire network.

## Prototype

```c
const char *aclmdlGetOpAttr(aclmdlDesc *modelDesc, const char *opName, const char *attr)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| modelDesc | Input | Pointer to the aclmdlDesc type.<br>Call [aclmdlCreateDesc](aclmdlCreateDesc.md) to create the aclmdlDesc type data in advance, and then call [aclmdlGetDesc](aclmdlGetDesc.md) to obtain the corresponding aclmdlDesc type data based on the model ID. |
| opName | Input | Pointer to the operator name. |
| attr | Input | Pointer to the operator attribute.<br>Currently, only the _datadump_original_op_names attribute is supported, which records the operators from which a certain operator is fused. The value of the _datadump_original_op_names attribute obtained through this API is in the format [opName1_len]opName1.....[opNameN_len]opNameN, where opNameN_len indicates the length of the operator name string.<br>The following is an example of the_datadump_original_op_names attribute value, indicating that a fused operator is formed by fusing the five operators scale2c_branch2c, bn2c_branch2c, res2c_branch2c, res2c, and res2c_relu, with operator name string lengths of 16, 13, 14, 5, and 10 respectively:<br>[16]scale2c_branch2c[13]bn2c_branch2c[14]res2c_branch2c[5]res2c[10]res2c_relu |

## Return Value

Returns a string pointer to the attribute value. The lifetime of this pointer is the same as that of modelDesc. If the modelDesc resource is destroyed, the content pointed to by this pointer is automatically destroyed as well. If the opName or attr attribute does not exist, or if the attr attribute value is empty, a pointer to an empty string is returned.

If the API fails to be called, nullptr is returned.
