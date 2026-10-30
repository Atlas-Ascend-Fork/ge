# check\_node\_support\_on\_aicore

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T04:20:11.052Z pushedAt=2026-10-16T10:53:07.732Z -->

## Applicable Products

Supported on all chips.

## Description

Validates the passed-in **Node** to check whether it supports execution on the **AI Core**.

## Prototype

```python
check_node_support_on_aicore(node: Node) -> Tuple[bool, str]
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| node | Input | Node to be checked, of the **Node** type. |

## Return Value

- \(Tuple\[bool, str\]\) Returns \(is\_supported, unsupported\_reason\) a tuple.
- **is\_supported** is **True** if the node is supported on the AI Core, and **False** otherwise.
- **unsupported\_reason** is the specific reason why the node is not supported.

## Constraints

- If the check operation fails, **RuntimeError** is thrown.
- Before calling this API, ensure that the shape of the node has been inferred, because the validation process requires shape information for judgment.
