# enable\_switch\_role

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-17T03:46:28.506Z pushedAt=2026-11-17T10:24:26.917Z nl=2 -->

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

Configures whether smooth role switching is supported, corresponding to the underlying llm.EnableSwitchRole configuration item.

## Prototype

```python
enable_switch_role(self, enable_switch_role: bool)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| enable_switch_role | bool | Enables role switching.<br><br>  - True: Supported<br>  - False: Not supported<br><br>If not configured, the default value is Not supported.<br>Related API: [switch_role](switch_role.md). |

## Examples

```python
from llm_datadist import LLMConfig
llm_config = LLMConfig()
llm_config.enable_switch_role = True
```

## Return Value

No value is returned under normal conditions.

TypeError or ValueError may be thrown for invalid parameters.

## Constraints

None
