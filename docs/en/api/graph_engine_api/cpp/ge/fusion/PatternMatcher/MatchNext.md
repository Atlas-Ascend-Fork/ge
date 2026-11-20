# MatchNext

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-16T13:34:35.774Z pushedAt=2026-10-28T07:40:22.584Z nl=2 -->

## Applicable Products

Supported on all chips.

## Header File/Library File

- Header file: \#include <ge/fusion/pattern\_matcher.h\>
- Library file: libge\_compiler.so

## Description

Returns the next match result that conforms to the **Pattern** definition, following the topological order of the current **Target Graph**.

Providing a one-by-one matching API resolves the selection problem in overlap scenarios. Users can call this API in a loop.

Note:

- The match results returned by the current API may overlap, and the caller must handle them.

    Consider the following structure. When matching ABC, C appears in two match results in succession, and the caller must decide which result to select.

    ```plaintext
             B  C   B
             \ / \ /
              A   A
    ```

- This function matches only the current graph. If the **Target Graph** contains subgraphs, the caller must match the subgraphs separately.

## Prototype

```c++
std::unique_ptr<MatchResult> MatchNext()
```

## Parameters

None

## Return Value

| Parameter | Type | Description |
| --- | --- | --- |
| - | std::unique_ptr\<MatchResult> | Match result. |

## Constraints

None
