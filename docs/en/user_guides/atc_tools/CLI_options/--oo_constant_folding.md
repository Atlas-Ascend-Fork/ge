# --oo\_constant\_folding

<!-- md-trans-meta sourceCommit=3b4c8f106b74c3a75512dd245c2b55b143c90db0 translatedAt=2026-10-22T03:22:49.661Z pushedAt=2026-10-26T11:21:14.254Z -->

## Product Support

Full chip support

## Function Description

**This is a debugging extension parameter and is not supported in production environments. It will be released as a formal feature in a later version.**

Whether to enable constant folding optimization.

Constant folding replaces nodes in the computation graph whose output values can be determined in advance with constants, and performs some structural simplification on the computation graph.

## Related Parameters

None.

## Parameter Values

**Parameter value:**

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
