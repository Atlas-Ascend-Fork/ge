# Description

<!-- md-trans-meta sourceCommit=9f2ea985eb7ee6374346cc894560d663152ee14c translatedAt=2026-10-15T03:04:29.977Z pushedAt=2026-10-16T10:53:07.431Z -->

The **Graph** class is used to represent and manipulate computation graphs.

Examples of basic graph operations:

```python
from ge.graph import Graph, DataType, Format

# Create a graph.
graph = Graph("my_graph")

# Retrieve all nodes.
nodes = graph.get_all_nodes()

# Set and get attributes.
graph.set_attr("my_attr", "value")
attr_value = graph.get_attr("my_attr")

# Export the graph.
graph.dump_to_file(format=DumpFormat.kReadable, suffix="debug")
```
