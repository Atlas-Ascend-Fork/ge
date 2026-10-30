# aclRecoverAllHcclTasks

<!-- md-trans-meta sourceCommit=ccb602e2af7c5423735601d41dd725c68afe7021 translatedAt=2026-10-27T15:59:22.151Z pushedAt=2026-10-29T03:59:05.993Z nl=2 -->

**Notice: This API is an experimental feature and may change in later versions. It is not supported for use in production environments.**

## Product Support

<!-- npu="950" id1 -->
- Ascend 950PR&950DT products: Supported
<!-- end id1 -->
<!-- npu="A3" id2 -->
- Atlas A3 products: Not supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 products: Not supported
<!-- end id3 -->
<!-- npu="310b" id4 -->
- Atlas 200I/500 A2 inference product: Not supported
<!-- end id4 -->
<!-- npu="310p" id5 -->
- Atlas inference products: Not supported
<!-- end id5 -->
<!-- npu="910" id6 -->
- Atlas training products: Not supported
<!-- end id6 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclRecoverAllHcclTasks_res.md#id1 -->

## Feature Description

In maintenance and test scenarios, this API recovers all collective communication tasks on the specified Device.

## Prototype

```c
aclError aclRecoverAllHcclTasks(int32_t deviceId)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| deviceId | Input | Device ID.<br>After you call aclrtGetDeviceCount to obtain the number of available devices, the value range of this Device ID is [0, (number of available devices - 1)]. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
