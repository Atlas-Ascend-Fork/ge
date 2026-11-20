# --atomic\_clean\_policy

<!-- md-trans-meta sourceCommit=0e375ee43f53b583431f339c4dc1d9fa79286a1d translatedAt=2026-11-16T13:37:01.086Z pushedAt=2026-11-17T03:29:34.581Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

Whether to centrally clean up the memory occupied by all memset operators in the network (all operators with the memset attribute are memset operators).

## Related Parameters

None.

## Parameter Values

- 0: (Default) Centralized cleanup.
- 1: Individual cleanup, which cleans up each memset operator in the network separately. This cleanup method is recommended when the memory occupied by memset operators in the network is too large, as it significantly reduces memory usage, but it may cause a certain performance loss.

## Recommended Configuration and Benefits

None.

## Example

Set the cleanup policy of the memset operator to separate cleanup. The following is an example:

```bash
atc --atomic_clean_policy=1 ...
```

## Constraints

None.
