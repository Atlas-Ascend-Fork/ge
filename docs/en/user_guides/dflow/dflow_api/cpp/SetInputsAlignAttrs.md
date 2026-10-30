# SetInputsAlignAttrs

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:46:07.557Z pushedAt=2026-10-29T02:30:30.436Z nl=2 -->

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

```cpp
FlowGraph &SetInputsAlignAttrs(uint32_t align_max_cache_num, int32_t align_timeout, bool dropout_when_not_align = false)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| align_max_cache_num | Input | Maximum number of caches for data alignment. The default value is 0, indicating that the data alignment function is disabled. A value greater than 0 indicates that the function is enabled, and the maximum value is 1024.<br>Each cache represents a group of inputs. |
| align_timeout | Input | Timeout for waiting for each group of data to be aligned, in ms.<br>-1 indicates that the operation never times out. The configured value must be greater than 0 and must not exceed 600 * 1000 ms (10 minutes). |
| dropout_when_not_align | Input | Whether to discard the data that is not aligned after a timeout or after the maximum number of caches is exceeded.<br><br>  - true: Yes<br>  - false: No<br><br>The default value is false. |

## Return Value

Returns the FlowGraph with the alignment attributes set.

## Exception Handling

None.

## Constraints

None.
