# DataFlow Running

<!-- md-trans-meta sourceCommit=816ef06d01755132788035613a4d64a7bafeebbb translatedAt=2026-11-16T08:54:51.059Z pushedAt=2026-11-17T03:39:11.991Z nl=2 -->

## Overview

This section describes how to construct and execute a FlowGraph, including the following two methods:

- Compose and execute via annotation. You can add the @pyflow decorator to a UDF definition, which automatically generates an fnode graph composition function. This allows you to construct a graph through the UDF definition, making it more convenient to use.
- Compose and execute via APIs. You can directly construct a graph through FlowNode-related APIs.

## Constructing and Executing a Graph Using Annotation

The process of constructing and executing using annotation is as follows.

**Figure 1** Constructing and executing a graph using annotation
![](figures/graph_construct_and_execute_use_annotation.png "Constructing and executing a graph using annotation")

The example code is as follows.

```python
import dataflow as df
import numpy as np

@df.pyflow
def add(in0, in1):
    return in0 + in1

# System initialization
options = {
    "ge.exec.deviceId":"0",
    "ge.experiment.data_flow_deploy_info_path":"./data_flow_deploy_info.json",
    "ge.socVersion": "AscendXXX"  # Modify the version based on the environment.
}
df.init(options)

# Define FlowData
data0 = df.FlowData()
data1 = df.FlowData()
# Generate FlowNode through UDF annotations
add_node = add.fnode()

# Build the edge relationships between FlowData and FlowNode
add_out = add_node(data0, data1)

# Build FlowGraph through FlowOut
dag = df.FlowGraph([add_out])

# Automatically generate the deployment policy based on the graph. All nodes are deployed on the first device in numa_config.
# To modify the policy, comment out the following line and modify the content of the ./data_flow_deploy_info.json file.
df.utils.generate_deploy_template(dag, "./data_flow_deploy_info.json")

# Call FlowGraph.feed to fill in the input
dag.feed({data0:np.array([[1, 2]], dtype=np.int32), data1:np.array([[2, 3]], dtype=np.int32)})

# Call FlowGraph.fetch to obtain the output
print("dataflow fetch result:", dag.fetch())
# Print: dataflow fetch result: ([array([[3, 5]], dtype=int32)], 0)

# Release system resources
df.finalize()
```

## Constructing and Executing a Graph Using API

The process of constructing and executing via API is as follows.

**Figure 2**  Constructing and executing a graph using FlowNode
![](figures/graph_construct_and_execute_use_flownode.png "Constructing and executing a graph using FlowNode")

The following is an example. For the complete example code, see [sample\_base](https://gitcode.com/cann/ge/blob/master/examples/dflow/python/sample_base.py).

```python
import dataflow as df
import numpy as np

# System initialization
options = {
    "ge.exec.deviceId":"0",
    "ge.experiment.data_flow_deploy_info_path":"./data_flow_deploy_info.json",
    "ge.socVersion": "AscendXXX"  # Modify the version based on the environment.
}
df.init(options)

# Define FlowData
data0 = df.FlowData()
data1 = df.FlowData()

# Define FuncProcessPoint to implement the Add function and add it to FlowNode
pp0 = df.FuncProcessPoint(compile_config_path='config/add_func.json')
add_node = df.FlowNode(input_num=2, output_num=1)
add_node.add_process_point(pp0)

# Build the edge relationships between FlowData and FlowNode
add_out = add_node(data0, data1)

# Build FlowGraph through FlowOut
dag = df.FlowGraph([add_out])

# Call FlowGraph.feed_data to fill in the input
flow_info = df.FlowInfo()
dag.feed_data({data0:np.array([[1, 2]], dtype=np.int32), data1:np.array([[2, 3]], dtype=np.int32)}, flow_info)

# Call FlowGraph.fetch_data to obtain the output
print("dataflow fetch result:", dag.fetch_data())

# Release system resources
df.finalize()
```
