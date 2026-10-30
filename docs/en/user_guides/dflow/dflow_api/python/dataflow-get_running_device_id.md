# dataflow.get\_running\_device\_id

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:13:15.564Z pushedAt=2026-10-29T03:03:43.953Z nl=2 -->

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

This API must be used together with the pyflow decorator.
