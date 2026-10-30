# Overview

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-15T02:50:19.429Z pushedAt=2026-10-16T10:53:07.378Z -->

The **GraphBuilder** class is used to build a computational graph in Eager Style.

Example of building a graph in Eager Style:

```python
from ge.es import GraphBuilder

# Create the builder.
builder = GraphBuilder("my_graph")

# Create the input.
input1 = builder.create_input(0, shape=[2, 2])
input2 = builder.create_input(1, shape=[2, 2])

# Create the constant.
const1 = builder.create_const_float(1.0)

# Perform tensor operations.
result = input1 + input2

# Set the output.
builder.set_graph_output(result, 0)

# Build the graph.
graph = builder.build_and_reset()
```
