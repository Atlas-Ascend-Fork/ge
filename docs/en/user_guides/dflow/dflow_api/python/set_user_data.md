# set\_user\_data

<!-- md-trans-meta sourceCommit=537d343d7af6b25d23bdc38c9e133ec1c0991c0d translatedAt=2026-11-16T09:36:41.516Z pushedAt=2026-11-17T03:39:12.055Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Sets user information.

## Prototype

```python
set_user_data(self, user_data: bytearray, offset: int = 0)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| user_data | bytearray | User-defined data. |
| offset | int | Offset of the user data, which must meet the following constraints.<br>[0, 64), size + offset <= 64 |

## Return Value

None is returned in normal scenarios.

In abnormal scenarios, an exception is thrown, and the exception type is dataflow.DfException. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
import dataflow as df
flow_info = df.FlowInfo(...)
user_data_str = "UserData123"
user_data_array = bytearray(user_data_str, 'utf-8')
flow_info.set_user_data(user_data_array)
```

## Constraints

None
