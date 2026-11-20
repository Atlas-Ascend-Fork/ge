# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T03:32:21.924Z pushedAt=2026-10-16T10:53:07.550Z -->

**offline\_compile** module is used for offline graph compilation.

Example of calling offline graph compilation:

```python
from ge.graph import Graph
from ge.offline_compile import build_initialize, build_finalize, build_model, save_model

# Create a graph.
graph = Graph("test_graph")
# Initialize model building.
build_initialize({"ge.socVersion": "Ascend910B1"})
# Compile the model.
model = build_model(graph, {"input_format": "ND"})
# Save the model.
save_model("sample", model)
# Release model building resources.
build_finalize()
```
