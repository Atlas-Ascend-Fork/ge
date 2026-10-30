# set\_exception\_catch

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:13:28.188Z pushedAt=2026-10-29T03:22:03.865Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets whether the user exception catching function is enabled.

## Prototype

```python
set_exception_catch(self, enable_exception_catch: bool = False)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| enable_exception_catch | bool | Whether to enable the user exception capture function. The value can be:<br><br>  - True: Yes, enable the exception function.<br>  - False: No, disable the exception function.<br><br>Default value: False. |

## Return Value

None.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
graph.enable_exception_catch(True)
```

## Constraints

When enabling the exception function, you must also use the [set\_inputs\_align\_attrs](set_inputs_align_attrs.md) API to enable the data alignment function; otherwise, a compilation error occurs.
