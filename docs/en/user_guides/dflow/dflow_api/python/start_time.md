# start\_time

<!-- md-trans-meta sourceCommit=537d343d7af6b25d23bdc38c9e133ec1c0991c0d translatedAt=2026-11-16T09:38:36.532Z pushedAt=2026-11-17T03:39:12.057Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Reads and sets the start time of FlowInfo as a property.

## Prototype

```python
@property
def start_time(self)
@start_time.setter
def start_time(self, new_value)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| new_value | int | Value of the start time of FlowInfo to be set. |

## Return Value

start\_time property.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
flowinfo = df.FlowInfo(...)
flowinfo.start_time = 100
print(flowinfo.start_time)
```

## Constraints

None
