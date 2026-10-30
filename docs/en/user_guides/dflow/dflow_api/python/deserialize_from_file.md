# deserialize\_from\_file

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:18:31.454Z pushedAt=2026-10-29T03:06:21.341Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Deserializes a serialized pickle file to restore a Python object.

## Prototype

```python
deserialize_from_file(pkl_file, work_path=None)
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| pkl_file | str | Path of the serialized pickle file. |
| work_path | str | Working path used during serialization. |

## Return Value

Python object restored through deserialization.

## Examples

```python
import dataflow as df
obj = df.msg_type_register.deserialize_from_file('file1')
```

## Constraints

None
