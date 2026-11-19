# switch\_role

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T04:00:41.173Z pushedAt=2026-11-17T11:32:11.916Z nl=2 -->

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

Switches the role of the current LLMDataDist. It is recommended to use this API only in PagedAttention scenarios.

## Prototype

```python
switch_role(self, role: LLMRole, switch_options: Optional[Dict[str, str]] = None)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| role | [LLMRole](LLMRole.md) | Target role to switch to. |
| switch_options | options: Dict[str, str] | Configuration items for role switching.<br>Optional parameter. The default value is None.<br>This parameter must be set when switching to Prompt, and it must contain the [listen_ip_info](listen_ip_info.md) configuration item. |

## Examples

```python
from llm_datadist import LLMDataDist, LLMRole
llm_datadist = LLMDataDist(LLMRole.DECODER, 0)
...
switch_options = { 'llm.listenIpInfo': '127.0.0.1:26000' }
llm_datadist.switch_role(LLMRole.PROMPT, switch_options)
```

## Return Value

- No value is returned under normal conditions.
- If the input data type is incorrect, a TypeError or ValueError exception is thrown.
- If [enable\_switch\_role](enable_switch_role.md) is not enabled in LLMConfig when LLMDataDist is initialized, calling this API throws [LLMException](LLMException.md), with status\_code being LLM\_FEATURE\_NOT\_ENABLED.
- If residual link resources exist during switch\_role, [LLMException](LLMException.md) is thrown, with status\_code being LLM\_EXIST\_LINK.
- If the target role of switch\_role is the same as the current role, [LLMException](LLMException.md) is thrown, with status\_code being LLM\_PARAM\_INVALID.
- In single-process multi-device mode, calling this API is not supported.

## Constraints

None
