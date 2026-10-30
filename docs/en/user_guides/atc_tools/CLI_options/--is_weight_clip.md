# --is\_weight\_clip

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:14:59.991Z pushedAt=2026-10-26T02:50:39.702Z -->

## Product Support

Supported on all chips.

## Feature Description

Whether to clip data when converting floating-point weight data from a high-bit type to a low-bit type.

When the original model weights are of a high-bit data type (such as Float32) and a Cast operator is inserted during conversion to convert them to a low-bit data type (such as Float16), data overflow may occur due to the numeric range limit. By configuring the --is\_weight\_clip parameter, the high-bit data can be clipped before the Cast operator, effectively preventing overflow and ensuring data integrity.

## Related Parameters

None.

## Parameter Values

- 0: No clipping.
- 1: (Default) Clipping.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --is_weight_clip=1 ...
```

## Dependencies and Constraints

None.
