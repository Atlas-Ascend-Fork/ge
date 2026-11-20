# format

<!-- md-trans-meta sourceCommit=1e1ed868460cbcf8fd60b8a11149c952406370ba translatedAt=2026-10-15T04:11:15.533Z pushedAt=2026-10-16T10:53:07.695Z -->

## Applicable Products

Supported on all chips.

## Description

Obtains the **Format** description of a **Tensor**, which stores the **OriginFormat**, **StorageFormat**, and dimension expansion rules.

## Prototype

```python
tensor.format -> StorageFormat
```

## Parameters

None

## Return Value

Returns the `StorageFormat` object of the current Tensor. This object is provided by the runtime and is valid only during the current execution callback.

## Constraints

- The returned object can be used only during the current execution callback.
- When applying for an output tensor, you can pass this object to `EagerOpExecutionContext.malloc_output_tensor`.

## Examples

```python
storage_format = x.format
```
