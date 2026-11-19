# IrInputDefV2 Constructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:46:51.477Z pushedAt=2026-10-28T07:40:22.392Z nl=2 -->

## Applicable Products

Supported on all chips.

## Description

Constructor and destructor of **IrInputDefV2**.

## Prototype

```c++
IrInputDefV2()
IrInputDefV2(const char_t *name, IrInputType ir_input_type, const char_t *symbol_id)
~IrInputDefV2()
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Name of the node's input port, used to identify the position of this input in the operator. |
| ir_input_type | Input | IR type of the input, determining whether the input is required, optional, or dynamic. |
| symbol_id | Input | Symbol identifier of the input, used for dynamic shape inference or symbolic expression. |

## Return Value

Constructs and returns an **IrInputDefV2** object.

## Constraints

None
