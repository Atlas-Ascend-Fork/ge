# set\_inputs\_align\_attrs

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T09:32:35.899Z pushedAt=2026-11-17T03:39:12.043Z nl=2 -->

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
