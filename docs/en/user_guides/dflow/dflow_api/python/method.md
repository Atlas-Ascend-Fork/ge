# method

<!-- md-trans-meta sourceCommit=8dfcb5a0693f337335319694ce183276aa0e59ef translatedAt=2026-10-28T13:05:18.999Z pushedAt=2026-10-29T03:17:05.277Z nl=2 -->

## Applicable Products

<!-- npu="A3" id1 -->
- Atlas A3 products: Supported
<!-- end id1 -->
<!-- npu="910b" id2 -->
- Atlas A2 products: Supported
<!-- end id2 -->

## Function

For complex scenarios, a class can be run as a pipeline task locally or remotely. To do so, use the @pyflow decorator on the class and the @method decorator on the functions in the class to indicate that these functions need to be run in pipeline mode. A class can contain multiple functions decorated with @method to indicate that they can accept inputs and be executed simultaneously. All functions decorated with @method must participate in constructing the FlowGraph. Functions that are not directly executed as a pipeline, such as internal functions, cannot be decorated with @method.

## Prototype

```python
Decorator @method
```

## Parameters

| Parameter | Data Type | Value Description |
| --- | --- | --- |
| num_returns | int | When the decorator decorates a function, this parameter indicates the number of outputs of the function. If this parameter is not set, the function returns one return value by default. You can choose either this parameter or the type annotations method to indicate the number and types of function return values. |
| stream_input | str | Indicates that the input of the current func is a stream input (that is, the function input parameter is a queue). Currently, only the "Queue" type is supported. Users can retrieve data from the input queue by themselves. |
| choice_output | function | Indicates that the current func is a choice output. Only outputs that meet the condition (a user-defined function) are returned. For example:<br>choice_output=lambda e: e is not None<br>This example indicates that only non-None outputs are returned. |

## Return Value

In normal scenarios, the decorated function is returned.

In abnormal scenarios, a DfException is thrown. You can catch the exception to obtain the error\_code and message in DfException to view the specific error code and error information. For details, see [DataFlow Error Codes](dataflow-error-code.md).

## Examples

```python
import dataflow as df
@df.pyflow
class Foo():
    def __init__(self):
        pass
    # Use num_returns to indicate that the number of outputs is 2.
    @df.method(num_returns=2)
    def func1(a, b):
        return a + b,a - b
    # Use typing to indicate that the number of outputs is 2.
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
        return None, a  # Send only non-null values to the corresponding output based on the lambda function.
```

## Constraints

The environment must have the cloudpickle package installed for the corresponding Python version.

Functions decorated with @method must participate in the graph construction process. For a class decorated with @pyflow, its own output cannot be used as its own input during graph construction. If a function has default values, edges must still be connected during graph construction.

In stream input scenarios, the DataFlow framework does not support data alignment or exception transaction handling.
