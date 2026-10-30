# set\_inputs\_align\_attrs

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:14:52.586Z pushedAt=2026-10-29T03:22:19.854Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets the input alignment properties in a FlowGraph.

## Prototype

```python
set_inputs_align_attrs(self, align_max_cache_num: int, align_timeout: int, dropout_when_not_align: bool = False)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| align_max_cache_num | int | Maximum number of cached data groups for alignment. The default value is 0, indicating that the data alignment function is disabled. A value greater than 0 enables the function, and the maximum value is 1024.<br>Each cache represents one group of inputs. |
| align_timeout | int | Timeout for waiting for each group of data to be aligned, in ms.<br>-1 indicates that the operation never times out. The configured value must be greater than 0 and must not exceed 600*1000 ms (10 minutes). |
| dropout_when_not_align | bool | Whether to discard data that is not aligned after a timeout or after the maximum number of caches is exceeded.<br><br>  - True: Yes<br>  - False: No<br><br>The default value is False. |

## Return Value

None.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
graph.set_inputs_align_attrs(256, 600 * 1000, False)
```

## Constraints

None
