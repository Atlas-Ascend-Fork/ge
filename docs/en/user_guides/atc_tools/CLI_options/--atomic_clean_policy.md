# --atomic\_clean\_policy

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T02:45:34.644Z pushedAt=2026-10-23T01:41:49.741Z -->

## Product Support

Supported on all chips

## Function Description

Whether to centrally clean up the memory occupied by all memset operators in the network (all operators with the memset attribute are memset operators).

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

## Usage Constraints

None.
