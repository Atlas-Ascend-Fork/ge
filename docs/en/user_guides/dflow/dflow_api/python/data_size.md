# data\_size

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:05:48.766Z pushedAt=2026-10-29T02:56:32.889Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtain the length of user\_data.

## Prototype

```python
@property
def data_size(self)
```

## Parameters

None

## Return Value

Return the length of user\_data.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
user_data_str = "UserData123"
result = graph.fetch_data() # Obtain the result asynchronously
flowinfo = result[1]
user_data_size = flowinfo.data_size
print(user_data_size)
```

## Constraints

None
