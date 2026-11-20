# IMPLEMT\_INFERFUNC

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T11:20:11.128Z pushedAt=2026-10-28T07:40:22.156Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File

\#include <graph/operator\_reg.h\>

## Description

Encapsulates the **InferShape** function of the operator.

The **OpType** passed to this function is a subclass derived from the **Operator** class. An object **op** of this subclass is automatically generated, and the member functions of the subclass can be used to obtain the input and output descriptions, thereby implementing **InferShape**.

The member functions of the subclass **op** derived from **OpType** are as follows:

- **op.set\_input\__x_\(Operator &v, const string &srcName\)**: Sets the output **srcName** of operator **v** in the network as the input **x** of the current operator.
- **op.get\_input\_desc\__x_\(\)**: Obtains the description of the input **x** of the operator. The returned object is of the **TensorDesc** type.

    **op.update\_input\_desc\__x_\(const TensorDesc& tensorDesc\)**: Updates the description of the input **x**, including shape, datatype, and format.

- **op.get\_output\_desc\__y_\(\)**: Obtains the description of the output **y** of the operator. The returned object is of the **TensorDesc** type.
- **op.update\_output\_desc\__y_\(const TensorDesc& tensorDesc\)**: Updates the description of the output **y**, including shape, datatype, and format.
- **op.get\_attr\__attr1_\(AscendString &val\)**: Obtains the value **val** of the operator attribute **attr1**.

## Prototype

```c++
IMPLEMT_INFERFUNC(op_name, func_name)
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| **op_name** | Input | Operator type. |
| **func_name** | Input | **InferShape** function name, user-defined. |

## Return Value

None

## Constraints

None
