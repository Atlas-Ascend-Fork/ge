# enable\_switch\_role

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:35:23.191Z pushedAt=2026-10-29T03:25:50.795Z nl=2 -->

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

No value is returned in normal cases.

TypeError or ValueError may be thrown for invalid parameters.

## Constraints

None
