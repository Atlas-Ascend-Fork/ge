# Overview

<!-- md-trans-meta sourceCommit=9d821ee607484f54716ece49d9dbdbe1c4f9524d translatedAt=2026-10-15T04:20:56.333Z pushedAt=2026-10-16T10:53:07.734Z -->

Utility classes related to Graph Engine (GE) graph modification, such as shape inference, checking whether a node is supported, and some graph optimization utility methods.
The calling example is as follows:

```python
from ge.utils import GeUtils
from ge.graph import Graph, Node

# Shape inference
graph = Graph("my_graph")
# ... Build the graph ...
GeUtils.infer_shape(graph, [[1, 3, 224, 224], [1, 3, 224, 224]])

# Check whether the node is supported on AICore
node = graph.get_node("node_name")
is_supported, reason = GeUtils.check_node_support_on_aicore(node)
if is_supported:
    print("The node is supported on AICore")
else:
    print(f"The node is not supported on AICore, reason: {reason}")
```
