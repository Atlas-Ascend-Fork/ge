# --oo\_dead\_code\_elimination

<!-- md-trans-meta sourceCommit=3b4c8f106b74c3a75512dd245c2b55b143c90db0 translatedAt=2026-11-16T14:18:11.415Z pushedAt=2026-11-17T11:19:35.973Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

**This is an extended debugging parameter. It is not supported in production environments and will be released as a formal feature in a later version.**

Whether to enable dead-edge elimination optimization.

Dead-edge elimination optimization: In a Switch node, if its first input (pred input) is a constant node (const), redundant branches are eliminated based on the value of the constant. If the constant is true, the false branch is removed; if the constant is false, the true branch is removed.

## Related Parameters

None.

## Parameter Values

- true: (default) enables dead-edge elimination optimization.
- false: disables dead-edge elimination optimization.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --oo_dead_code_elimination=true ...
```

## Constraints

None.
