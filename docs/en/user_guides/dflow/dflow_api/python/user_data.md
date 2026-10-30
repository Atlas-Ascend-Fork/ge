# user\_data

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T12:46:13.738Z pushedAt=2026-10-29T03:25:50.662Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains user information.

## Prototype

```python
@property
def user_data(self)
```

## Parameters

None

## Return Value

Returns the user\_data object as a property.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
user_data_str = "UserData123"
result = graph.fetch_data() # Obtain the result asynchronously
flowinfo = result[1]
fetch_user_data = flowinfo.user_data[0:len(user_data_str)]
name = fetch_user_data.decode('utf-8')
print(name)
```

## Constraints

None
