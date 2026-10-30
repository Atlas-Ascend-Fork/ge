# SetRole

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:24:33.956Z pushedAt=2026-10-29T03:25:50.755Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 inference products: Supported
<!-- end id2 -->
<!-- npu="910b" id3 -->
- Atlas A2 training products: Not supported
<!-- end id3 -->

## Function

Sets the role of LLM-DataDist. It is recommended to use this API only in PagedAttention scenarios.

## Prototype

```cpp
Status SetRole(LlmRole role, const std::map<AscendString, AscendString> &options = {})
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| role | Input | Role type, of the [LlmRole](LlmRole.md) type. |
| options | Input | Parameters for setting the role. For the currently supported parameters, see Table 1. |

**Table 1**  Configuration items

| configuration item | Optional/Required | Description |
| --- | --- | --- |
| OPTION_LISTEN_IP_INFO | Required when switching to Pormpt | Set to the IP address and port of the Device, for example, "192.168.1.1:26000" |

## Return Value

- LLM\_SUCCESS: Role set successfully
- LLM\_PARAM\_INVALID: Invalid parameter
- LLM\_FEATURE\_NOT\_ENABLED: The feature is not enabled
- LLM\_EXIST\_LINK: Residual link resources exist.
- Others: Failure.

## Exception Handling

None

## Constraints

During LLM-DataDist initialization, you need to set OPTION\_ENABLE\_SET\_ROLE to "1" before you can use this API.
