# aclopSetMaxOpQueueNum

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-27T15:57:31.575Z pushedAt=2026-10-29T03:58:11.115Z nl=2 -->

## Product Support

<!-- npu="950" id776 -->
- Ascend 950PR&950DT products: Not supported
<!-- end id776 -->
<!-- npu="A3" id777 -->
- Atlas A3 products: Supported
<!-- end id777 -->
<!-- npu="910b" id778 -->
- Atlas A2 products: Supported
<!-- end id778 -->
<!-- npu="310b" id779 -->
- Atlas 200I/500 A2 inference product: Supported
<!-- end id779 -->
<!-- npu="310p" id780 -->
- Atlas inference products: Supported
<!-- end id780 -->
<!-- npu="910" id781 -->
- Atlas training products: Supported
<!-- end id781 -->
<!-- @ref: ge/res/docs/zh/api/graph_engine_api/c/acl/aclopSetMaxOpQueueNum_res.md#id1 -->

## Feature Description

Configures operator cache aging when executing a single operator through the single-operator model, to save memory and balance invocation performance.

## Prototype

```c
aclError aclopSetMaxOpQueueNum(uint64_t maxOpNum)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| maxOpNum | Input | Maximum length of the "operator type - single-operator model" mapping queue.<br>When the maximum length is reached, the mapping information and single-operator models in the cache that have not been used for a long time are deleted first, and then the latest mapping information and the corresponding single-operator models are loaded.<br>When executing a single operator in single-operator model mode (except when executing a single operator through the aclopUpdateParams API), if this API is not called to configure the maximum length of the mapping queue, the default maximum length is 20000.<br>When executing a single operator through the aclopUpdateParams API, if this API is not called to configure the maximum length of the mapping queue, operator cache information does not need to be aged. |

## Return Value

Returns 0 on success; otherwise, returns a failure value. See [aclError](aclError.md).
