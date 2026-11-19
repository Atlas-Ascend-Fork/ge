# get\_user\_data (dataflow)

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:49:15.085Z pushedAt=2026-11-16T08:04:39.813Z nl=2 -->

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
get_user_data(self, size: int = 0, offset: int = 0) -> bytearray
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| size | int | Length of the user data. Value range: [0, 64]. |
| offset | int | Offset of the user data, which must comply with the following constraints.<br>[0, 64), size+offset<=64 |

## Return Value

In normal scenarios, a bytearray is returned.

In abnormal scenarios, an exception is thrown, and the exception type is dataflow.DfException. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
user_data_str = "UserData123"
result = graph.fetch_data() # Fetch the result asynchronously.
flowinfo = result[1]
fetch_user_data = flowinfo.get_user_data(len(user_data_str))
name = fetch_user_data.decode('utf-8')
print(name)
```

## Constraints

None
