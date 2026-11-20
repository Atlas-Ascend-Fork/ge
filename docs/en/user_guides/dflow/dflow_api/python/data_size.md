# data\_size

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:09:05.853Z pushedAt=2026-11-16T07:45:38.345Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the length of user\_data.

## Prototype

```python
@property
def data_size(self)
```

## Parameters

None

## Return Value

Returns the length of user\_data.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
user_data_str = "UserData123"
result = graph.fetch_data() # Fetch the result asynchronously.
flowinfo = result[1]
user_data_size = flowinfo.data_size
print(user_data_size)
```

## Constraints

None
