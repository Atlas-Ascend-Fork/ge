# --oo\_constant\_folding

<!-- md-trans-meta sourceCommit=3b4c8f106b74c3a75512dd245c2b55b143c90db0 translatedAt=2026-11-16T14:17:51.199Z pushedAt=2026-11-17T11:19:28.765Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

**This is an extended debugging parameter. It is not supported in production environments and will be released as a formal feature in a later version.**

Whether to enable constant folding optimization.

Constant folding replaces nodes in the computation graph whose output values can be determined in advance with constants, and performs some structural simplification on the computation graph.

## Related Parameters

None.

## Parameter Values

**Parameter values:**

- true: (default) Enables constant folding optimization.
- false: Disables constant folding optimization.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --oo_constant_folding=true ...
```

## Constraints

None
