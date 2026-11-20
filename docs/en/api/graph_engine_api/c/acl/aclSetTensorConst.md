# aclSetTensorConst

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:17:27.791Z pushedAt=2026-11-18T06:15:03.545Z nl=2 -->

## Product Support

<!-- npu="950" id120 -->
- Ascend 950PR&950DT products: Supported
<!-- end id120 -->
<!-- npu="A3" id121 -->
- Atlas A3 products: Supported
<!-- end id121 -->
<!-- npu="910b" id122 -->
- Atlas A2 products: Supported
<!-- end id122 -->
<!-- npu="310b" id123 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id123 -->
<!-- npu="310p" id124 -->
- Atlas inference products: Supported
<!-- end id124 -->
<!-- npu="910" id125 -->
- Atlas training products: Supported
<!-- end id125 -->
<!-- npu="IPV350" id126 -->
- IPV350: not supported
<!-- end id126 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorConst_res.md#id1 -->

## Function Description

Sets a constant input for an operator, specifying the memory address and address length where the input tensor data is stored.

## Prototype

```c
aclError aclSetTensorConst(aclTensorDesc *desc, void *dataBuffer, size_t length)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| desc | Output | Pointer to the aclTensorDesc type.<br>Call [aclCreateTensorDesc](aclCreateTensorDesc.md) in advance to create the aclTensorDesc type. |
| dataBuffer | Input | Pointer to the memory address of the input tensor data. |
| length | Input | Length of the memory address, in bytes. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).

## Constraints

<!-- npu="950,A3,910b,910,310p,310b" id1 -->
In the Ascend EP form, memory on the Host must be allocated at dataBuffer.
<!-- end id1 -->

<!-- npu="310b" id2 -->
In the Ascend RC form, memory on the Device must be allocated at dataBuffer.
<!-- end id2 -->

<!-- npu="310p" id3 -->
In the Control CPU open form, memory on the Device must be allocated at dataBuffer.
<!-- end id3 -->

<!-- npu="IPV350" id4 -->
Memory on Device must be allocated for dataBuffer.
<!-- end id4 -->

<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclSetTensorConst_res.md#id2 -->
