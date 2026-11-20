# transaction\_id

<!-- md-trans-meta sourceCommit=537d343d7af6b25d23bdc38c9e133ec1c0991c0d translatedAt=2026-11-16T09:40:38.689Z pushedAt=2026-11-17T03:39:12.064Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Reads and writes the transaction ID as an attribute.

## Prototype

```python
@property
def transaction_id(self)
@transaction_id.setter
def transaction_id(self, new_value)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| new_value | int | New value of the transaction ID to be set. When set to 0, the custom transId is not used, and a transId is automatically generated internally in an auto-increment manner. |

## Return Value

attribute of transaction\_id.

## Examples

```python
import dataflow as df
graph = df.FlowGraph(...)
flowinfo = df.FlowInfo(...)
flowinfo.transaction_id = 10
print(flowinfo.transaction_id)
```

## Constraints

- It takes effect only when the graph construction API sets set\_contains\_n\_mapping\_node to true.
- transaction\_id can only increase, not decrease. If it is not set externally, transaction\_id auto-increments starting from 1.
- An error is reported after transaction\_id reaches the uint64\_max value.
- When data alignment is enabled, ensure that the transaction\_id of each batch of input data is consistent; otherwise, data may be misaligned.
- Custom transaction\_id is enabled only when transaction\_id is set to a non-zero value.
