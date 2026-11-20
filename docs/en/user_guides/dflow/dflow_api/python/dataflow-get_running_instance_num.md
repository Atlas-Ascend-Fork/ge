# dataflow.get\_running\_instance\_num

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T05:17:28.881Z pushedAt=2026-11-16T07:51:34.117Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

Obtains the number of running instances of the current UDF during UDF execution. This information comes from the data\_flow\_deploy\_info.json configuration in logic\_device\_list.

## Prototype

```python
get_running_instance_num()
```

## Parameters

None

## Return Value

Returns the number of running instances of the current UDF. The data type is int.

## Examples

```python
import dataflow as df

@df.pyflow
def func():
    print('running instance_num = ', df.get_running_instance_num())
    return a + b
```

## Constraints

This function must be used together with the pyflow decorator.
