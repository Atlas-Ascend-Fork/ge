# dataflow.get\_running\_device\_id

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:16:39.561Z pushedAt=2026-11-16T07:51:26.741Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the device\_id of the current UDF during UDF execution. The information comes from the logic\_device\_list configuration in data\_flow\_deploy\_info.json.

## Prototype

```python
get_running_device_id()
```

## Parameters

None

## Return Value

Returns the device\_id of the current UDF, with the data type int.

## Examples

```python
import dataflow as df

@df.pyflow
def func():
    print('running device_id = ', df.get_running_device_id())
    return a + b
```

## Constraints

This function must be used together with the pyflow decorator.
