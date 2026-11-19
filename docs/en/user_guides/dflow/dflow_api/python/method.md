# method

<!-- md-trans-meta sourceCommit=688a734ba16efd86f64585d92b25babcec3a2576 translatedAt=2026-11-16T08:12:34.514Z pushedAt=2026-11-16T08:41:16.149Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 series: supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 series: supported
<!-- end id2 -->

## Function

For complex scenarios, a class can be run as a pipeline task locally or remotely. To do so, use @pyflow to decorate the class and use @method to decorate the functions in the class, to express that the functions need to be run in pipeline mode. Multiple functions decorated with @method can exist in one class, to express that they can accept input and execute simultaneously. All functions decorated with @method must participate in constructing the FlowGraph. Functions that are not directly executed as a pipeline, such as internal functions, cannot be decorated with @method.

## Prototype

```python
Decorator @method
```

## Parameters

| Parameter | Data Type | Description |
| --- | --- | --- |
| num_returns | int | When the decorator decorates a function, it is used to express the number of outputs of the function. If this parameter is not set, the function returns one return value by default. Use either this parameter or the type annotations method to identify the number and types of function returns. |
| stream_input | str | Indicates that the input of the current func is streaming input (that is, the function input parameter is a queue). Currently only the "Queue" type is supported, and users can retrieve data from the input queue by themselves. |
| choice_output | function | Indicates that the current func is an optional output. Only outputs that meet the condition (the condition is a user-defined function) are returned. For example:<br>choice_output=lambda e: e is not None<br>This example indicates that only non-None outputs are returned. |

## Return Value

In normal scenarios, the decorated function is returned.

In exceptional cases, a DfException is thrown. The exception can be caught to obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Example

```python
import dataflow as df
@df.pyflow
class Foo():
    def __init__(self):
        pass
    # Use num_returns to express the number of outputs as 2.
    @df.method(num_returns=2)
    def func1(a, b):
        return a + b,a - b
    # Use typing to express the number of outputs as 2.
    @df.method()
    def func2(a, b) -> Tuple[int, int]:
        return a + b,a - b
    # Return 1 by default.
    @df.method()
    def func3(a, b):
        return a + b

    @df.method(stream_input='Queue')
    def func4(a, b):
        data1 = a.get()
        data2 = a.get()
        data3 = b.get()
        return data1 + data2 + data3

    @df.method(choice_output=lambda e: e is not None)
    def func5(self, a) -> Tuple[int, int]:
        return None, a  # Send only non-null values to the corresponding output according to the lambda function.
```

## Constraints

The environment must have the cloudpickle package installed for the corresponding Python version.

Functions decorated with @method must participate in the graph construction process. In the graph construction process of a class decorated with @pyflow, its own output cannot be used as its own input. If a function has default values, edge connection is still required during graph construction.

In the streaming input scenario, the DataFlow framework does not support data alignment and exception transaction processing.
