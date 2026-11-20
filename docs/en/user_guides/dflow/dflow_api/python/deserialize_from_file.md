# deserialize\_from\_file

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:23:05.629Z pushedAt=2026-11-16T07:53:43.093Z nl=2 -->

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
