# aclRecoverAllHcclTasks

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-17T15:16:29.476Z pushedAt=2026-11-18T06:14:52.226Z nl=2 -->

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
<!-- npu="IPV350" id7 -->
- IPV350: not supported
<!-- end id7 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclRecoverAllHcclTasks_res.md#id1 -->

## Function Description

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
