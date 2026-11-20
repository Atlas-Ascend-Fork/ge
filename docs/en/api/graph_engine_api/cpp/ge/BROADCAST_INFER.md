# BROADCAST\_INFER

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:15:04.515Z pushedAt=2026-10-28T07:40:22.136Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/operator\_reg.h\>

## Description

Provides a public function macro wrapper for operator developers to develop the **InferShape** function. This function sets the output **shape** based on the **shape** of the two inputs. This macro only sets the **shape** and does not set the **dtype**.

- If the **shape** of the two inputs is the same, the output **shape** is set based on the input **shape**.
- If the **shape**s of the two inputs are different, the union of the two input **shape**s is taken according to the **Broadcast** strategy.

    For example, if the input **shape** is (1,2,3,4) and (3,1,3,4), this macro sets the operator output **shape** to (3,2,3,4).

## Prototype

```c++
BROADCAST_INFER(in1_name, in2_name, out_name)
```

This function automatically calls the following functions:

```c++
graphStatus BroadCastInfer(const function<vector<int64_t>()> &get_in1_shape,
                           const function<vector<int64_t>()> &get_in2_shape,
                           const function<void(const std::vector<int64_t> &y_shape)> &set_out_shape);
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **in1_name** | Input | First input of the operator. |
| **in2_name** | Input | Second input of the operator. |
| **out_name** | Input | Output of the operator. |

## Return Value

Returns success or failure.

## Constraints

None

## Examples

```c++
IMPLEMT_INFERFUNC(RightShift, RightShiftInfer) {
  DataType type = op.GetInputDesc("x").GetDataType();
  SET_OUTPUT_TYPE(op, "z", type);
  return BROADCAST_INFER("x", "y", "z")(op);
}
```
