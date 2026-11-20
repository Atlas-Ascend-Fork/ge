# IrOutputDefV2 Constructor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T12:49:16.334Z pushedAt=2026-10-28T07:40:22.403Z nl=2 -->

## Applicable Products

Supported on all chips.

## Description

**IrOutputDefV2** constructor and destructor.

## Prototype

```c++
IrOutputDefV2()
IrOutputDefV2(const char_t *name, IrOutputType ir_output_type, const char_t *symbol_id)
~IrOutputDefV2()
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Output port name, which identifies the position of this output in the operator. |
| ir_output_type | Input | Output type, which determines whether the output is required or dynamic. |
| symbol_id | Input | Symbol identifier, used for dynamic shape inference. |

## Return Value

Constructs and returns an **IrOutputDefV2** object.

## Constraints

None
