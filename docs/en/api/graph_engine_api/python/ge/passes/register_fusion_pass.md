# register\_fusion\_pass

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:50:39.591Z pushedAt=2026-10-16T10:53:07.598Z -->

## Applicable Products

Supported on all chips.

## Description

A class decorator for registering fusion passes, used to register a **FusionBasePass** or **PatternFusionPass** subclass into the GE compilation process.

## Prototype

```python
register_fusion_pass(*, name: str, stage: PassStage, kind: Optional[str] = None) -> callable
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| name | Input | Pass name, a string type, which must be unique and cannot duplicate the name of an already registered pass. |
| stage | Input | Pass execution stage, of the **PassStage** enumeration type. |
| kind | Input | Pass type identifier, an optional parameter. If not specified, it is automatically set to **"pattern_fusion"** when the decorated class is a subclass of **PatternFusionPass**; otherwise, it is set to **"fusion_base"**. |

## Return Value

Returns a class decorator function. The decorated class is registered in the Pass registry and gets the **\_\_ge\_pass\_descriptor\_\_** attribute attached.

## Constraints

None
