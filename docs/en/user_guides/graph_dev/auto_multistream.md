# Automatic Multi-Stream Parallelism

<!-- md-trans-meta sourceCommit=6e5f525dd9e972a6824fe2d1ae18cafa0d5e5675 translatedAt=2026-11-18T02:05:00.420Z pushedAt=2026-11-18T02:56:14.497Z nl=2 -->

## Overview

### Feature Overview

Multistream is a pure-Python multi-stream planning module. The caller describes computation tasks and their dependencies as a directed acyclic graph (DAG), and the module assigns a stream ID to each node participating in the planning and returns the required number of streams. Based on this, the executor can schedule mutually independent tasks to different streams, leveraging computation overlap to improve device resource utilization.

This technique is suitable for computation graphs with independent branches. For tasks that consist mainly of serial dependencies, increasing the number of streams does not necessarily bring benefits; the final effect also depends on computation resources, memory access pressure, and synchronization overhead.

### Implementation Principle

Multi-stream planning consists of three steps:

1. Describe dependencies: use nodes to represent computation tasks and edges to represent data or control dependencies.
2. Generate a plan: analyze the execution paths in the graph, merge them into a limited number of streams according to the specified policy, and preserve serial group constraints.
3. Apply the plan: the caller reads the node stream IDs and the total number of streams, creates execution resources, schedules tasks, and sets up synchronization for cross-stream dependencies.

For example, when two branches both depend on the same input and have no dependency on each other, they are eligible for parallel execution; their convergence node still needs to wait for both branches to complete. The planning preserves these dependencies and does not change the computation semantics of the tasks.

### Constraints

- The input must be a directed acyclic graph that fully contains data and control dependencies.
- The module only generates the plan; stream creation, task dispatch, and cross-stream synchronization are the caller's responsibility. A stream ID is not a Runtime stream handle.
- The upper limit of normal streams does not include the additional serial group. The caller should reserve resources based on the total number of streams returned.
- The currently available public entry points provide two policies: load balancing and main stream priority. The core count recommendation API returns 0, indicating that the original configuration is retained.

## Usage

### Preparing the Environment

Set up a complete AI processor environment. Install the driver, firmware, and CANN software package as described in [Environment Preparation](overview/environment_setup.md), and configure the environment variables.

When DAG automatic multi-stream is enabled via GE (setting ge.autoMultistreamParallelMode to LoadBalance:N or MainStream:N), the Python native bridge components must be loaded. If no precompiled artifacts match the current Python version, platform, and component ABI, GE compiles these components locally. In this case, a working C++ compiler, the development headers and libpython of the current Python, and pybind11 with its header files are required. A verified version can be installed in the same Python environment where GE runs:

```bash
python -m pip install 'pybind11==2.13.6'
python -c "import sys, pybind11; print(sys.version); print(sys.prefix); print(pybind11.get_include())"
```

### Graph Construction and Planning

The following uses a computation graph that forks and then joins to demonstrate the call process. Complete the environment preparation described above before running the example.

The example uses [DAGGraph](../../api/graph_engine_api/python/ge/multistream/DAGGraph.md) to create a graph and add nodes and dependency edges, calls [plan](../../api/graph_engine_api/python/ge/multistream/plan.md) to generate a plan, and then queries the stream assignment result through the returned [StreamPlan](../../api/graph_engine_api/python/ge/multistream/StreamPlan.md).

```python
import multistream as ms

# Create a task dependency graph. Node names must be unique within the graph.
graph = ms.DAGGraph("branches")
source = graph.add_node("source", "Op")
left = graph.add_node("left", "Op")
right = graph.add_node("right", "Op")
join = graph.add_node("join", "Op")

# source forks to left and right, which then join at join. There is no dependency between left and right.
# The add_edge parameters are, in order, the source node, source port, target node, and target port.
graph.add_edge(source, 0, left, 0)
graph.add_edge(source, 1, right, 0)
graph.add_edge(left, 0, join, 0)
graph.add_edge(right, 0, join, 1)

# Use the load balancing policy, with the maximum number of normal streams set to 2.
result = ms.plan(
    graph,
    max_stream_num=2,
    merge_strategy=ms.MergeStrategy.LOAD_BALANCE,
)

# Query the total number of streams required and the stream ID of each node for the caller to create streams and schedule tasks.
print("required streams:", result.get_required_stream_count())
for node in graph.get_nodes():
    print(node.get_name(), result.get_stream_id(node))
```

The number of streams is an upper limit, and the example does not guarantee that two streams are actually allocated. The caller can set the execution engine type, cost, and serial tag of each node before planning, and then generate a plan through the same entry point. For API parameters and constraints, see [Multi-Stream Interface](../../api/graph_engine_api/python/ge/multistream/multistream_interface.md).

### Selecting a Policy and Using the Result

| Policy | Selection Method |
| --- | --- |
| Load balancing | [MergeStrategy.LOAD_BALANCE](../../api/graph_engine_api/python/ge/multistream/MergeStrategy.md) |
| Main stream priority | [MergeStrategy.MAIN_STREAM](../../api/graph_engine_api/python/ge/multistream/MergeStrategy.md) |

The upper limit of the normal stream count can be set to a value from 1 to 64. You can specify the starting stream ID for this planning through `base_stream_id`.

After planning succeeds, first call [get_required_stream_count()](../../api/graph_engine_api/python/ge/multistream/StreamPlan.md) to reserve resources, and then call [get_stream_id(node)](../../api/graph_engine_api/python/ge/multistream/StreamPlan.md) to establish the mapping from nodes to actual streams. Excluded normal nodes return -1, and the caller must handle them according to its own execution semantics and must not create a stream for -1. When dependencies exist between different streams, the consumer must wait for the producer to complete.

To make a group of tasks execute serially, you can set the same non-empty `stream_label` for the nodes before planning. This group uses one additional stream. The label does not replace dependency edges, and the caller must still fully describe the dependencies between tasks.

## Effect Verification

Keep the tasks, inputs, and runtime environment consistent, and compare the result correctness and end-to-end latency before and after using the solution. Start with a smaller upper limit on the number of streams, and then compare the effects of different policies and stream counts. The planning time and the device execution benefit should be measured separately; performance improvement cannot be judged solely by an increase in the number of streams.
