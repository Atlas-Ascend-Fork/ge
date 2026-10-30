# Special Topics

<!-- md-trans-meta sourceCommit=24fb17e3715fbd5097e72e1018d9d79c241fb875 translatedAt=2026-10-28T12:57:00.863Z pushedAt=2026-10-29T06:03:37.371Z nl=2 -->

## UDF Development Process

There are two ways to develop UDFs:

- Creating UDFs through annotations, that is, developing UDFs by decorating functions with dataflow.pyflow or dataflow.method.
- Creating UDFs through projects.

The differences between the two ways are as follows:

|Comparison Item|Creating UDFs through Annotations|Creating UDFs through Projects|
|--|--|--|
|Advantages and Disadvantages|Advantages: simple and direct to use. Any code logic can be encapsulated into a UDF through annotations. No additional modification to the original code logic of user code is required, resulting in low modification cost. Streaming input and streaming output are supported. Disadvantages: usage is restricted in some scenarios.|Advantages: API calls are more flexible. UDF calling NN scenarios are supported. Disadvantages: the usage is relatively cumbersome. Intrusive modification to user source code is required, and UDF-related APIs are used for development.|
|Usage Restrictions|UDF calling NN scenarios are not supported.|None|
|Recommended Scenarios|When UDF calling NN is not used, it is recommended to use this way to create UDFs first.|For UDF calling NN scenarios, use this way to create UDFs.|

### Environment Preparation

- Before developing UDFs, install the driver firmware and the development suite package Ascend-cann-toolkit. For details, see [CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum).
- Before developing Python UDFs, install the Python dependency packages pybind11 and jinja2. pybind11 is used for object conversion between C++ and Python. jinja2 is used to quickly generate project templates.
- Configure the environment variable for the CANN software package installation path ASCEND\_INSTALL\_PATH. During UDF compilation, the dependent header files and .so files are located based on this environment variable. If it is not set, the default value is "/usr/local/Ascend". A configuration example is as follows.

    ```shell
    export ASCEND_INSTALL_PATH=CANN package installation path
    ```

### Creating UDFs through Annotations

**Development Process**

The process of developing UDFs through annotations is shown in the following figure:

![](figures/260109170200356.png)

**Creating a UDF**

Using the function that implements the add operation as an example, the following describes how to create a UDF through the dataflow.pyflow and dataflow.method annotations. Refer to the following example:

```python
import dataflow as df

# Decorate a regular function with the pyflow annotation to build the function into a UDF
@df.pyflow()
def add(a, b):
    return a + b

# Decorate a class with the pyflow annotation and use method to decorate a class method to build the class method into a UDF
@df.pyflow
class Add():
    @df.method()
    def add(self, a, b):
        return a + b
```

**Building a FlowGraph**

After the UDF is constructed through annotations, use the fnode method to construct a FlowNode and participate in the construction of the FlowGraph. The example is as follows:

```python
# Define the input
data0 = df.FlowData()
data1 = df.FlowData()

# Define the FlowNode
flow_node0 = add.fnode()
flow_node1 = Add.fnode()

# Build the edge relationships
flow_node0_out = flow_node0(data0, data1)
flow_node1_out = flow_node1.add(flow_node0_out, data1)

# Build the FlowGraph
dag = df.FlowGraph([flow_node1_out])
```

### Creating UDFs through Projects

**Development Process**

The process of developing UDFs through projects is shown in the following figure:

![](figures/260109170411587.png)

**UDF Project Creation**

UDF projects are classified into the following types. Create a project based on the usage scenario.

- Implementing user-defined functions through UDFs: In this scenario, users do not call third-party dependency libraries or existing NN functions, and implement custom functions entirely by writing UDF-related files.
- Implementing custom functions by calling NN: In this scenario, the functions required by users can be carried by existing NN functions, and users only need to call the NN functions through UDFs.

To create a project through a tool, see [UDF Python Project Creation Tool](appendices.md#udf-python-project-creation-tool).

**Custom Function Development**

During UDF Python development, custom processing functions are written. This section describes the development process of UDFs. This part of the code can be automatically generated from a template.

A UDF implementation consists of two parts:

- UDF implementation file: the code implementation of the user-defined function, with the file extension .py. Depending on the scenario, it is divided into a UDF implementation file (implementing custom functions through UDFs) and implementing custom functions by calling NN.
- UDF encapsulation and registration file: a custom cpp file generated by users based on tools or written according to specifications. It is used to obtain the custom Python UDF, encapsulate it into a C++ function, and call FLOW\_FUNC\_REGISTRAR to complete function registration.

> [!NOTE]
>
>- In the single-P scenario, one input corresponding to multiple outputs, one input corresponding to one output, or multiple inputs corresponding to one output is supported.
>- In the 2P scenario, only one input corresponding to one output is supported.
>- The following uses a function that implements the add function as an example to describe how to develop, compile, construct a graph for, and verify a UDF.

**UDF implementation file (implementing custom functions through UDFs)**

Users need to develop user-defined functions in the “workspace/src\_python/xxx.py” file of the project. The following uses “func\_add.py” as an example.

**Function analysis:**

This function implements the Add function, converts the two inputs into numpy arrays, performs an addition operation, and outputs the operation result.

**Specify the inputs and outputs:**

The Add function has two inputs and one output.

**Notes:**

The function name defined by users in the Python file must be consistent with the name used to obtain the Python module in the cpp file.

For example, if users define the initialization function as init\_flow\_func in the Python file, when looking up the Python module attribute in the Init function of cpp, they should look up init\_flow\_func. For a specific example, see the following code.

**Function implementation:**

It includes the initialization function and implementation functions (there can be multiple implementation functions).

When writing UDF code, users usually need to import the dataflow.data\_type and dataflow.flow\_func modules.

flow\_func provides common APIs for UDF development, and data\_type encapsulates common data types. Users can import the modules and view their specific content.

```python
import dataflow.flow_func as ff
import dataflow.data_type as dt
# Define the class
class Add():
# The class name is related to the UDF function to be developed
```

- Initialization function, which performs initialization. An example is as follows.

    ```python
        @ff.init_wrapper() # This description is mandatory. The wrapper converts C++ objects and Python-exposed objects.
        def init_flow_func(self, meta_params):  # The input parameter must be meta_params only, of the MetaParams type.
            name = meta_params.get_name()
            ff.logger.info("func name is %s", name)
            input_num = meta_params.get_input_num()
            ff.logger.info("input num %d", input_num)
            device_id = meta_params.get_running_device_id()
            ff.logger.info("device id %d", device_id)
            out_type = meta_params.get_attr_tensor_dtype("out_type")
            if out_type[0] != ff.FLOW_FUNC_SUCCESS:
                 ff.logger.error("get dtype failed")
                 return ff.FLOW_FUNC_FAILED
            self.count = 0
            return ff.FLOW_FUNC_SUCCESS

    ```

- Implementation function.

    ```python
    @ff.proc_wrapper() # This description is mandatory. The wrapper converts C++ objects and Python-exposed objects.
    def Add1(self, run_context, input_flow_msgs): # The input parameters must be a MetaRunContext object and a FlowMsg object only.
        for msg in input_flow_msgs:
            ff.logger.error("msg code %d", msg.get_ret_code())
            if msg.get_ret_code() != ff.FLOW_FUNC_SUCCESS:
                ff.logger.error("invalid input")
                return ff.FLOW_FUNC_FAILED
        # The add method should have only two inputs.
        if input_flow_msgs.__len__() != 2:
            ff.logger.error("invalid input")
            # Return the error code defined in the flow_func module.
            return ff.FLOW_FUNC_ERR_PARAM_INVALID
        tensor1 = input_flow_msgs[0].get_tensor()
        tensor2 = input_flow_msgs[1].get_tensor()

        dtype1 = tensor1.get_data_type()
        dtype2 = tensor2.get_data_type()
        # The datatypes of the two inputs are expected to be the same.
        if dtype1 != dtype2:
            ff.logger.error("input data type is not same")
            return ff.FLOW_FUNC_FAILED
        ff.logger.info("element size %d", tensor1.get_data_size())
        ff.logger.event("data type is same")
        shape1 = tensor1.get_shape()
        shape2 = tensor2.get_shape()
        if shape1 != shape2:
            ff.logger.error("input data shape is not same")
            return ff.FLOW_FUNC_FAILED
        ff.logger.info("shape is same")
        # Apply for the output msg object.
        out = run_context.alloc_tensor_msg(shape1, dt.DT_INT32)
        data_size = out.get_tensor().get_data_size()
        ff.logger.error("data_size %d", data_size)
        ele_cnt = out.get_tensor().get_element_cnt()
        ff.logger.error("ele_cnt %d", ele_cnt)
        np1 = tensor1.numpy()
        np2 = tensor2.numpy()
        a = out.get_tensor().numpy()
        # Obtain the tensor, and then obtain the array data in the tensor to perform the addition operation.
        a[...] = np1 + np2
        ff.logger.event("prepare to set output in add1")
        if run_context.set_output(0, out) != ff.FLOW_FUNC_SUCCESS:
            ff.logger.error("set output failed")
            return ff.FLOW_FUNC_FAILED
        self.count += 1
        return ff.FLOW_FUNC_SUCCESS

    ```

- If users need to define multiple functions, they can define multiple implementation functions in the same class.

    The following example shows a class that includes two implementation functions: Add1 and Add2.

    ```python
    @ff.proc_wrapper()
    def Add1(self, run_context, input_flow_msgs):
        xxxx

    @ff.proc_wrapper()
    def Add2(self, run_context, input_flow_msgs):
        xxxx
    ```

**UDF implementation file (implementing custom functions by calling NN)**

Users need to develop user-defined functions in the “workspace/src\_python/xxx.py” file of the project.

**Function analysis:**

This function implements the Add function.

**Specifying inputs and outputs:**

It includes two inputs and one output.

**Function implementation:**

It includes an initialization function and implementation functions (there can be multiple implementation functions).

When writing UDF code, users usually need to import the dataflow.data\_type and dataflow.flow\_func modules.

flow\_func exposes the common APIs for UDF development, and data\_type encapsulates the common data types. Users can import the modules and view their specific content.

```python
import dataflow.flow_func as ff
import dataflow.data_type as dt
# Define the class
class Add():
# The class name is related to the actual UDF function being developed
```

- Initialization function, which performs initialization actions. The example is as follows.

    ```python
        @ff.init_wrapper() # This decorator is mandatory. The wrapper converts C++ objects to Python-exposed objects
        def init_flow_func(self, meta_params):  # The input parameter must be meta_params only, of the MetaParams type
            name = meta_params.get_name()
            ff.logger.info("func name is %s", name)
            input_num= meta_params.get_input_num()
            ff.logger.info("input num %d", input_num)
            device_id = meta_params.get_running_device_id()
            ff.logger.info("device id %d", device_id)
            out_type = meta_params.get_attr_tensor_dtype("out_type")
            if out_type[0] != ff.FLOW_FUNC_SUCCESS:
                 ff.logger.error("get dtype failed")
                 return ff.FLOW_FUNC_FAILED
            self.count = 0
            return ff.FLOW_FUNC_SUCCESS

    ```

- Add\(\): a user-defined computation processing function. After the UDF framework receives the input tensor data, it calls this method.

    The example is as follows:

    ```python
    @ff.proc_wrapper() # This decorator is mandatory. The wrapper converts C++ objects to Python-exposed objects
    def AddNN(self, run_context, input_flow_msgs): # The input parameters must be exactly one MetaRunContext object and one FlowMsg object
        ret = run_context.run_flow_model("invoke_graph", input_flow_msgs, 1000)
        if ret[0] != ff.FLOW_FUNC_SUCCESS:
            ff.logger.error("run nn failed")
            return ff.FLOW_FUNC_FAILED
        ff.logger.event("run nn success")
        i = 0
        for out in ret[1]:
            if run_context.set_output(i, out) != ff.FLOW_FUNC_SUCCESS:
                ff.logger.error("set output failed")
                return ff.FLOW_FUNC_FAILED
            i = i + 1
        return ff.FLOW_FUNC_SUCCESS
    ```

- If users need to define multiple functions, they can define multiple implementation functions in the same class.

    The following example shows a class that includes two implementation functions: Add1 and Add2.

    ```python
    @ff.proc_wrapper()
    def Add1(self, run_context, input_flow_msgs):
        xxxx

    @ff.proc_wrapper()
    def Add2(self, run_context, input_flow_msgs):
        xxxx
    ```

**UDF Encapsulation and Registration**

In addition to the Python file, developing a UDF in Python also requires a C++ interface invocation file. It is recommended to generate it directly through the [tool](appendices.md#udf-python-project-creation-tool), or you can customize it. An example is as follows.

The C++ class must inherit from MetaMultiFunc. The core content of the C++ file includes three parts.

**Init Function**

```cpp
    int32_t Init(const std::shared_ptr<MetaParams> &params) override
    {
        FLOW_FUNC_LOG_DEBUG("Init enter");
        PyAcquire();
        ScopeGuard gilGuard([this]() { PyRelease(); });
        int32_t result = FLOW_FUNC_SUCCESS;
        try {
            PyRun_SimpleString("import sys");
            std::string append = std::string("sys.path.append('") + params->GetWorkPath() + "')";
            PyRun_SimpleString(append.c_str());

            constexpr const char *pyModuleName = "func_add"; // Keep it consistent with the file name of the Python UDF, such as func_add in the preceding example
            constexpr const char *pyClzName = "Add";
            FLOW_FUNC_LOG_INFO("Load py module name: %s", pyModuleName);
            auto module = py::module_::import(pyModuleName);
            FLOW_FUNC_LOG_INFO("%s.%s import success", pyModuleName, pyClzName);
            pyModule_ = module.attr(pyClzName)();
            if (CheckProcExists() != FLOW_FUNC_SUCCESS) {
                FLOW_FUNC_LOG_ERROR("%s.%s check proc exists failed", pyModuleName, pyClzName);
                return FLOW_FUNC_FAILED;
            }
            if (py::hasattr(pyModule_, "init_flow_func")) { // Keep it consistent with the initialization function name defined in the Python UDF, such as init_flow_func in the preceding example
                result = pyModule_.attr("init_flow_func")(params).cast<int32_t>();
                if (result != FLOW_FUNC_SUCCESS) {
                    FLOW_FUNC_LOG_ERROR("%s.%s init_flow_func result=%d", pyModuleName, pyClzName, result);
                    return result;
                }
                FLOW_FUNC_LOG_INFO("%s.%s init_flow_func success", pyModuleName, pyClzName);
            } else {
                FLOW_FUNC_LOG_INFO("%s.%s has no init_flow_func method, no need init", pyModuleName, pyClzName);
            }
        } catch (std::exception &e) {
            FLOW_FUNC_LOG_ERROR("init failed: %s", e.what());
            result = FLOW_FUNC_FAILED;
        }
        FLOW_FUNC_LOG_DEBUG("FlowFunc Init end.");
        return result;
    }

```

**proc() Function:**

```cpp
    int32_t AddProc1(
        const std::shared_ptr<MetaRunContext> &runContext, const std::vector<std::shared_ptr<FlowMsg>> &inputFlowMsgs)
    {
        FLOW_FUNC_LOG_INFO("enter");
        PyAcquire();
        ScopeGuard gilGuard([this]() { PyRelease(); });
        int32_t result = FLOW_FUNC_SUCCESS;
        try {
            result = pyModule_.attr("Add1")(runContext, inputFlowMsgs).cast<int32_t>(); // The name of attr here is the same as the processing function defined in the Python UDF, such as Add1, Add2, or Sub1, AddNN in the preceding example.
            if (result != FLOW_FUNC_SUCCESS) {
                FLOW_FUNC_LOG_ERROR(".Add Add return %d", result);
                return result;
            }
            FLOW_FUNC_LOG_INFO("call Add result: %d", result);
        } catch (std::exception &e) {
            FLOW_FUNC_LOG_ERROR("proc failed: %s", e.what());
            result = FLOW_FUNC_FAILED;
        }
        return result;
    }
// If users write multiple processing functions in the same UDF Python class, they can implement multiple FUNCs here by calling different Python attrs through multiple C++ functions.
    int32_t AddProc2(
        const std::shared_ptr<MetaRunContext> &runContext, const std::vector<std::shared_ptr<FlowMsg>> &inputFlowMsgs)
```

Declare the implementation class as the func name through the FLOW\_FUNC\_REGISTRAR macro and register it with the UDF framework.

```cpp
FLOW_FUNC_REGISTRAR(Add)
    .RegProcFunc("Add1", &Add::AddProc1)
    .RegProcFunc("Add2", &Add::AddProc2);
```

**UDF Project Compilation**

Create the build and release directories under the udf\_py\_ws\_sample directory:

After compilation is complete, the successfully compiled so is generated in the workspace/release directory.

1. Create the build directory.

    ```shell
    mkdir -p build
    ```

2. Create the release directory.

    ```shell
    mkdir -p release
    ```

3. Enter the build directory.

    ```shell
    cd build
    ```

4. Compile the project.
    - In the non-cross-compilation scenario (where the development environment and runtime environment have the same architecture), the example is as follows: compile the x86\_64 type so, specify the target lib as libadd.so, and use g++ as the compilation tool.

        ```shell
        cmake .. -DTOOLCHAIN=g++ -DRELEASE_DIR=../release -DRESOURCE_TYPE=X86 -DUDF_TARGET_LIB=add && make
        ```

        - -DTOOLCHAIN: name of the compilation tool, for example, g++.
        - -DRELEASE\_DIR: path for storing the target file after compilation is complete.
        - -DRESOURCE\_TYPE: resource type to compile. The value can be X86, Aarch, or Ascend.
            - X86: set to X86 when the target so needs to be deployed on a Host with the X86 architecture.
            - Aarch: set to Aarch when the target so needs to be deployed on a Host with the Aarch architecture.
            - Ascend: set to Ascend when the target so needs to be deployed on the Device.

        - -DUDF\_TARGET\_LIB: name of the target so after compilation is complete. For example, if set to add, libadd.so is actually generated.

    - In the cross-compilation scenario (where the development environment is x86\_64 and the runtime environment is aarch64), compile the Ascend type so, specify the target lib as libadd.so, and use \$\{INSTALL\_DIR\}/toolkit/toolchain/hcc/bin/aarch64-target-linux-gnu-g++ as the compilation tool. Replace \$\{INSTALL\_DIR\} with the file storage path after CANN software installation. Taking installation by the root user as an example, the default storage path after installation is /usr/local/Ascend/cann.

        The example is as follows:

        ```shell
        cmake .. -DTOOLCHAIN=${INSTALL_DIR}/toolkit/toolchain/hcc/bin/aarch64-target-linux-gnu-g++ -DRELEASE_DIR=../release -DRESOURCE_TYPE=Ascend -DUDF_TARGET_LIB=add && make
        ```

**Build the FlowGraph**

After constructing the UDF, build the FlowGraph through FlowNode and FuncProcessPoint. The example is as follows:

```python
# Define the input
data0 = df.FlowData()
data1 = df.FlowData()

# Define the FlowNode whose FuncProcessPoint implements the Add function
pp0 = df.FuncProcessPoint(compile_config_path="./add_func.json")
flow_node0 = df.FlowNode(input_num=2, output_num=1)
flow_node0.add_process_point(pp0)

# Build the edge relationships
flow_node0_out = flow_node0(data0, data1)

# Build the FlowGraph
dag = df.FlowGraph([flow_node0_out])
```

## Specifying the Deployment Location of the DataFlow Node

**Feature Overview**

A configuration file can be used to specify the deployment location of DataFlow graph nodes. This feature enables multi-card and multi-instance deployment, distributes the computation workload, and improves performance.

**Usage Constraints**

- When specifying deployment, the deployment location of all nodes must be specified.
- Specify the deployment configuration file based on ge.experiment.data\_flow\_deploy\_info\_path in the options parameter of AddGraph. The file must exist and be in the correct format. For details about the format requirements, see **"Configuration File Format"**.
- The deployment location must be consistent with the location where the node can actually be deployed. For example, if a UDF supports only compiling Host release packages, it cannot be specified to be deployed on the Device. Similarly, if a node can be deployed only on the Device, it cannot be specified to be deployed on the Host.
- The same node cannot be configured repeatedly.
- The same node cannot be deployed on both the Host and the Device at the same time.

**Usage Method**

The following shows usage examples.

```python
flow_node0 = df.FlowNode(input_num=1, output_num=2)
flow_node0_out = flow_node0(data0)

# Build the FlowGraph. This section focuses on the usage of options. For details about how to construct a dataflow graph, see other sections.
options = {
    "ge.experiment.data_flow_deploy_info_path":"./data_flow_deploy_info.json"
}
dag = df.FlowGraph([out for out in flow_node0_out], options)
```

**Configuration File Format**

The format requirements of a typical configuration file data\_flow\_deploy\_info.json are shown in the following example.

```json
{

   "keep_logic_device_order": false,
   "batch_deploy_info": [
        {
            "flow_node_list": ["flowNode1", "flowNode2"],
            "logic_device_list": "0:0:-1:0"
        },
        {
            "flow_node_list": ["flowNode3"],
            "logic_device_list": "0:0:1:1"

        },
        {
            "flow_node_list": ["flowNode4"],
            "logic_device_list": "0:0:0:0,0:0:1:0",
            "invoke_list":[
            {
               "invoke_name":"invoked_flow_graph_name",
               "deploy_info_file":"./data_flow_invoke_flow_graph_deploy_info.json"
             },
            {
               "invoke_name":"invoked_flow_graph_name1",
               "logic_device_list": "0:0:0:0"
             }]
        },
        {
            "flow_node_list": ["flowNode5", "flowNode6"],
            "logic_device_list": "0:0:2~3:0~1"
        },
        {
            "flow_node_list": ["flowNode7", "flowNode8"],
            "logic_device_list": "0:0:0~1:0,0:0:2:1"
        }
    ]
}
```

**Table 1** Configuration items

|Configuration Item|Description|
|--|--|
|keep_logic_device_order|Whether to deploy instances in the order of the device_list configured by users.<br>Values are as follows:<br>- true: Yes. In the multi-instance scenario, devices are sorted and deployed according to the device_list configured by users.<br>- false: No. In the multi-instance scenario, devices are sorted and deployed according to the internal implementation logic of the framework.<br>Default value: false.|
|**batch_deploy_info**|-|
|flow_node_list|List of FlowNode node names. One or more names are supported. Separate multiple names with half-width commas.|
|logic_device_list|Deployment location of DataFlow graph nodes.<br>**Format**: clusterid:serverid:deviceid:numaid (pgid/dieid). The meaning of each field is as follows:<br>- clusterid: ID of the cluster. It is fixed to 0.<br>- serverid: ID of the server node. Set it to node_id in numa_config.json.<br>- deviceid: Logical number of the device. It corresponds to the item sequence in the "node>item_list" field of the numa_config file specified by the environment variable RESOURCE_CONFIG_PATH, starting from 0.<br>- numaid (pgid/dieid): Number of multiple compute units on a single device.<br> **Typical scenarios are as follows**.<br>When heavy_load of FunctionPp is true, the node is deployed to the Host node corresponding to the specified node. When deploying to multiple devices, configure as follows:<br>- Configure one by one: For example, "0:0:0:0,0:0:0:1" indicates that the node is deployed to the two devices "0:0:0:0" and "0:0:0:1" in multi-instance mode.<br>- Configure by device range: For example, "0:0:0~1:0~1", where "~" indicates a range and the value before "~" must be less than or equal to the value after it. "0:0:0~1:0~1" indicates that the node is deployed to two compute units of two devices: "0:0:0:0", "0:0:0:1", "0:0:1:0", and "0:0:1:1".<br>When a FlowNode is a parent node that invokes a sub-FlowGraph, multi-instance deployment cannot be configured.|
|invoke_list (optional; if not configured, the subgraph deployment node is the same as the parent node deployment node)|-|
|invoke_name|Name of the InvokedClosure nested in the FlowNode node, corresponding to the name parameter of the AddInvokedClosure API.|
|deploy_info_file|Specifies the deployment location of the DataFlow subgraph node through this configuration item. When the InvokedClosure corresponding to invoke_name is a DataFlow subgraph, you can configure the deployment location file corresponding to the DataFlow subgraph, for example, ./data_flow_invoke_flow_graph_deploy_info.json. For the fields contained in this file and the format requirements, see data_flow_deploy_info.json.|
|logic_device_list|Specifies the deployment location of the DataFlow subgraph or AscendGraph subgraph node through this configuration item. The configuration method is the same as that of logic_device_list in batch_deploy_info.<br>For the same subgraph node, logic_device_list and deploy_info_file cannot be configured at the same time. Otherwise, an error is reported.<br>For an AscendGraph subgraph node, the number of instances of logic_device_list must be the same as that of the parent node's logic_device_list. Otherwise, an error is reported.|

## DataFlow Subgraph Compilation Cache

**Feature Overview**

When the DataFlow compilation cache feature is enabled, if the subgraph of a single DataFlow node is modified, only the modified subgraph is incrementally compiled, and the remaining subgraphs use the cache, thereby reducing the compilation time.

**Usage constraints:**

- This feature supports FlowGraph but not AscendGraph.
- Models with resource-type operators are not supported.
- The cache does not guarantee cross-version compatibility. If the version is upgraded, you need to clear the cache directory and recompile to generate the cache.
- After a DataFlow subgraph changes, the original cache files become unavailable. Users need to manually delete the cache files in the cache directory and recompile to generate new cache files.

**Usage Method**

1. Enable cache configuration.

    The configuration example is as follows.

    ```python
        # Initialize
        SOC_VERSION = os.getenv('SOC_VERSION', 'AscendXXX')
        options = {
            "ge.exec.deviceId":"0",
            "ge.exec.logicalDeviceClusterDeployMode":"SINGLE",
            "ge.exec.logicalDeviceId":"[0:0]",
           "ge.graph_compiler_cache_dir":"./build_cache_dir"
        }
        df.init(options)

        flow_node0 = df.FlowNode(input_num=1, output_num=2)
        flow_node0_out = flow_node0(data0)

        # Build a FlowGraph. This section focuses on the use of options. For details about how to construct a dataflow graph, see other sections.
        options = {
            "ge.graph_key":"test_graph_00"
        }
        dag = df.FlowGraph([out for out in flow_node0_out], options)
    ```

    **Table 1** Parameters

|Parameter|Description|
|--|--|
|ge.graph_compiler_cache_dir|Graph compilation disk cache directory. It is used together with ge.graph_key. The graph compilation disk cache function takes effect only when both ge.graph_compiler_cache_dir and ge.graph_key are configured as non-empty.<br>The configured cache directory must exist; otherwise, compilation fails.<br>After the graph changes, the original cache files become unavailable. Users need to manually delete the cache files in the cache directory, including the model cache file, index file, and variable format file, and then recompile to generate cache files.|
|ge.graph_key|Unique graph identifier. It is recommended that the value contain only uppercase and lowercase letters (A-Z, a-z), digits (0-9), underscores (_), and hyphens (-), and be no longer than 128 characters. If the condition is not met, the system reports an error.|

    After compilation is complete, cache files are generated in the specified directory. For details, see "Cache File Generation Rules".

2.  If the graph changes and recompilation is required, perform the following steps.

    > [!NOTE]
    >Graph changes include: modifying the graph or config.json corresponding to GraphProcessPoint, modifying the UDF implementation file, and modifying the deployment information.
    >1.  Manually delete the files cached for the entire graph (including the model cache file, index file, and variable format file; for details, see "Cache File Generation Rules").
    >2.  Recompile.
    >    After compilation is complete, cache files are generated in the specified directory. For details, see "Cache File Generation Rules".

**Cache File Generation Rules**

The generated files include:

- Model cache file
- Index file, which helps you quickly locate the corresponding cache file through graph\_key. The following is an example of the index file content:

    ```json
    {
        "cache_file_list":[
            {
                "cache_file_name":"./cache_dir/graph_$key1_20230117202307.om",
                "graph_key":"graph_$key1",
                "var_desc_file_name":"./cache_dir/graph_$key1_20230117202307.rdcpkt"
            },
            {
                "cache_file_name":"./cache_dir/graph_$key1_20230117203007.om",
                "graph_key":"graph_$key1",
                "var_desc_file_name":"./cache_dir/graph_$key1_20230117203007.rdcpkt"
            }
        ]
    }
    ```

- Variable format file, which is generated only when variables exist in the graph. It is used by the framework to match the model cache file. If the variable format in the graph corresponding to graph\_key changes, the previously cached files cannot be directly restored and used. In this scenario, the compilation process is triggered again to regenerate the cache files.
- If the external weight feature is enabled, that is, the ge.externalWeight parameter is configured in the options and set to 1, a weight directory is also generated in the path specified by **ge.graph\_compiler\_cache\_dir** to store the weight information of the Const/Constant nodes in the original network.
- The DataFlow subgraph cache folder is generated only in the FlowGraph scenario. It is named graph\_key and stores the cache files generated during DataFlow subgraph compilation.

File naming rules:

- When the ge.graph\_key configuration value contains only uppercase and lowercase letters (A-Z, a-z), digits (0-9), underscores (\_), and hyphens (-), and its length does not exceed 128 characters:
  - The index file is named ge.graph\_key + ".idx".
  - The model cache file is named ge.graph\_key + current time + ".om".
  - The variable format file is named ge.graph\_key + current time + ".rdcpkt".

- If the preceding conditions are not met, the system reports an error.

## DataFlow Offline Compilation

**Feature Overview**

DataFlow offline compilation refers to compiling in the development environment and loading and deploying in the runtime environment, thereby decoupling compilation from graph construction.

**Usage constraints:**

The numa_config.json of the execution environment must be consistent with that at compilation time, except for the ipaddr field.

**Usage method**

1. Enable graph compilation cache. An example is as follows.

    ```python
        # Initialize
        SOC_VERSION = os.getenv('SOC_VERSION', 'AscendXXX')
        options = {
            "ge.exec.deviceId":"0",
            "ge.exec.logicalDeviceClusterDeployMode":"SINGLE",
            "ge.exec.logicalDeviceId":"[0:0]",
            "ge.graph_compiler_cache_dir":"./build_cache_dir",
            "ge.runFlag":"0"
        }
        df.init(options)

        flow_node0 = df.FlowNode(input_num=1, output_num=2)
        flow_node0_out = flow_node0(data0)

        # Build the FlowGraph. This section focuses on the use of options. For the actual construction of a dataflow graph, see other sections.
        options = {
            "ge.graph_key":"test_graph_00"
        }
        dag = df.FlowGraph([out for out in flow_node0_out], options)
    ```

2. Configure the development environment.

    Configure the RESOURCE\_CONFIG\_PATH file of the target execution environment through the environment variable numa\_config.json. The example is as follows:

    ```shell
    export RESOURCE_CONFIG_PATH=numa_config.json //Used to set the storage path of the heterogeneous resource description information file.
    ```

    > [!NOTE]
    >**numa\_config.json** For details, see [numa\_config.json Configuration](appendices.md#numa_configjson-configuration). The ipaddr field can use any IP address.

3. Graph generation.
    1. Call the feed\_data API to compile the graph and generate cache files.

        The example is as follows.

        ```python
        graph_options = {"ge.graph_key" : "key"}
        # Build the FlowGraph
        dag = df.FlowGraph([flow_node2_out], graph_options)

        # feed data
        feed_data0 = np.ones([1,2], dtype=np.int32)
        feed_data1 = np.array([[1,2]], dtype=np.int32)

        flow_info = df.FlowInfo()
        flow_info.start_time = 0
        flow_info.end_time = 5

        dag.feed_data({data0:feed_data0, data1:feed_data1}, flow_info) # Feed data asynchronously
        # The feed_data API involves graph compilation and loading. If ge.runFlag is set to 0, only graph compilation is performed:
        graph_options = {"ge.graph_key" : "key",
                         "ge.runFlag": "0"}
        ```

    2. Offline deployment.

        Copy the model cache file, index file, and variable format file from the graph\_compiler\_cache\_dir path in the development environment to the graph\_compiler\_cache\_dir path in the runtime environment. For details about the specific files, see "Cache File Generation Rule".

        The constraints for Atlas A2 products are as follows:

        The numa\_config.json of the execution environment must be consistent with that at compilation time, except the ipaddr field.

## Data Alignment

**Function Introduction**

**Before learning about data alignment, you need to first understand what a data pair is.**

**Figure 1**  Data pair diagram
![](figures/data_for_schematic.png "Data pair diagram")

- Define Model X as the earliest source of data. It has two output queues, and the multiple groups of output data \[A1',B1'\], \[A2',B2'\], \[A3',B3'\] are associated with each other within each group.
- Define Model A and Model B as data processing nodes, deployed as multiple instances on multiple cards. Model A is responsible for processing data flow A', and Model B is responsible for processing data flow B'. Because of multi-instance deployment, the processed data becomes out of order and cannot be output in the original order of Model X.
- Define Model C as the data aggregation node. It takes data from data flow A and data flow B group by group for processing, and must have the pairing relationship of \[A1, B1\],\[A2, B2\],\[A3, B3\] to process correctly.

**Data alignment means that a UDF node requires the input data to be data pairs.**

According to the data flow in the diagram, if data pairing is not performed, following the framework's default receiving order, the order of data received by Model C is \[A3, B2\],\[A2, B1\],\[A1, B3\]. Because the data is disordered and cannot be processed directly, after the UDF node receives \[A3, B2\], it needs to cache them first, and then wait until it receives \[A2, B1\] before it can process \[A2, B2\] normally. Only after receiving \[A1, B3\] can it process \[A1, B1\] and \[A3, B3\].

**Usage Constraints**

The framework data alignment relies on transId and dataLabel as the basis for determining alignment. If a piece of data is distributed to different instances, the data flows arriving at the next node are not ordered due to differences in processing timing, making it impossible to guarantee pairing.

**Usage Method**

```python
dag = df.FlowGraph([flow_node_out])
dag.set_inputs_align_attrs(align_max_cache_num=256, align_timeout=60 * 1000, dropout_when_not_align=True) # Cache up to 256 groups of data. If the timeout is 1 minute, discard the data that is not aligned within the timeout.
```

## Configuring Dynamic Shape

**Feature Overview**

Configuring dynamic shape provides more shape range information to the subgraph compilation phase, allowing the compiler to compile efficient operators and manage memory better. Setting maxOutputSize configures a more accurate maximum output value, and the graph executor pre-allocates output memory before graph computation based on this configuration, improving performance.

**Usage constraints**

None

**Usage method**

Suppose there is a resize subgraph with a dynamic shape. The specific implementation is as follows. This subgraph has two inputs: one is the image data to be resized, and the other is the resize target size information:

```python
def resize_bilinear(x: torch.Tensor, hw: torch.Tensor):
      return F.interpolate(x, size=[hw[0], hw[1]], mode="bilinear", align_corners=True)
```

For this dynamic subgraph, the specific configuration is as follows.

- **ge.inputShape**: shape range of the dynamic input.
  - Setting by name is supported: "input\_name1:n1,c1,h1,w1;input\_name2:n2,c2,h2,w2", for example: "input\_name1:8\~20,3,5,-1;input\_name2:5,3\~9,10,-1". The specified node must be enclosed in double quotation marks, and nodes are separated by English semicolons. input\_name must be the node name in the network model before conversion. If users know the name of the data node, setting by name is recommended.

  - Setting by index is supported: "n1,c1,h1,w1;n2,c2,h2,w2", for example: "8\~20,3,5,-1;5,3\~9,10,-1". The node name can be omitted, and nodes are arranged in index order and separated by English semicolons. When setting the shape range by index, the data node must have the index attribute set to indicate which input it is, with the index starting from 0.

  - For dynamic dimensions, a shape range is represented by a tilde “\~”; a fixed dimension is represented by a fixed number; and an unbounded range is represented by -1.

- **ge.outputMaxSize (optional)**: specifies the maximum memory for graph output.
  - When this parameter is set, the dataflow framework uses it to pre-allocate memory for the graph output, which reduces one data copy and improves execution performance.

        If the maximum output shape is \[1,3,2567,3440\] and the dtype is float32 (fp32 storage requires 4 bytes), outputMaxSize is calculated as 1x3x2567x3440x4=105965760.

        If it is set incorrectly, an error is reported when outputMaxSize is smaller than the actual output.

  - When this parameter is not set, output memory is allocated based on the actual situation when model execution is complete.

- **inputs\_tensor\_desc**
  - **data\_type**: data type of the tensor.
  - **shape**: shape information of the tensor. Set dynamic dimensions to -1.
  - **format**: the format information of each input must be set based on the actual situation. NCHW, NHWC, and ND are supported.

```python
{
    "build_options": {
        "ge.inputShape": "1,3,1~1728,1~1728;2",
        "ge.outputMaxSize": "107495424"
    },
    "inputs_tensor_desc": [
      {
        "data_type": "DT_FLOAT",
        "shape": [1, 3, -1, -1],
        "format": "NCHW"
      },
      {
        "data_type": "DT_INT32",
        "shape": [2],
        "format": "ND"
      }
    ]
}
```

## Multi-Instance Deployment

**Feature Overview**

When a node requires a large amount of computing resources, consider deploying multiple copies to accelerate computation.

**Usage constraints**

After multi-instance deployment, because the computation timing between instances is inconsistent, you may need to enable the data alignment feature to ensure data pairing.

**Usage method**

To enable multi-instance deployment, see [Specifying the Deployment Location of the DataFlow Node](#specifying-the-deployment-location-of-the-dataflow-node).

Specifically, the following two modes are available in usage scenarios:

**Table 1** Parallel modes

|Parallel Mode|Usage Scenario|Advantage|Disadvantage|
|--|--|--|--|
|Inter-request multi-instance parallel|Multiple concurrent requests exist and the throughput needs to be improved|Simple to implement, and no special code modification is required|The single-request latency cannot be reduced.|
|Split-request multi-instance acceleration|The single-request latency needs to be reduced|Low single-request latency|The code needs to be adapted. The node before the multi-instance node needs to split data and specify a distribution policy, the multi-instance node needs to specify a data aggregation policy, and the data needs to be merged after multi-instance processing.|

- Parallel mode 1: inter-request multi-instance parallel:

    **Figure 1**  Inter-request multi-instance parallel
    ![](figures/multi_instance_run_in_parallel_req.png "Inter-request multi-instance parallel")

    The figure shows a DataFlow model connected in series by Model X-\>Model A-\>Model B. Because Model A takes a long time to compute, Model A is split into two instances for acceleration. After the data flows (A, B, C, and D) pass through Model X, they are polled and distributed to the two instances of Model A based on the transaction ID (data A and C are routed to instance 1, and data B and D are routed to instance 2), and then aggregated by Model B.

- Parallel mode 2: split-request multi-instance acceleration:

    In this scenario, data splitting and data merging logic needs to be added. Balanced distribution is implemented by specifying BalanceConfig at output to control output distribution and aggregation (this feature currently does not support the py\_flow annotation execution mode).

    **Figure 2**  Split-request multi-instance acceleration
    ![](figures/multi_instance_acceleration_split_req.png "Split-request multi-instance acceleration")

The original model is a DataFlow model connected in series by Model X-\>Model A-\>Model B. Because Model A takes a long time to compute, multi-instance acceleration is applied to Model A to reduce the end-to-end latency. The detailed steps are as follows.

1. Request data A passes through Model X and enters the Model data splitting module, where it is split into four parts (A1, A2, A3, and A4). This operation needs to be implemented by users based on their own business logic. The split data needs to carry relevant data identifiers for subsequent nodes to identify and process (for example, adding information such as DataId/DataName to the data).
2. A1 and A3 are sent to instance 1 of Model A, and A2 and A4 are sent to instance 2 of Model A.
3. After processing, A1, A2, A3, and A4 are sent to the Model data merging module and merged into data A.
4. Data A is sent to Model B.

**The data distribution example is as follows.**

Based on the data A1, A2, A3, and A4 split by users, the following shows how to perform data distribution.

```python
# Graph construction phase
flow_node.set_balance_scatter()

# UDF output mode 1: batch output, applicable to the scenario where users split all data at once.
cfg = ff.BalanceConfig(1, 4, ff.AffinityPolicy.NO_AFFINITY) # The meaning of each parameter is: row_num=1, col_num=4 (4 data copies in total), affinity_policy=ff.AffinityPolicy.NO_AFFINITY (no affinity)

cfg.set_data_pos([(0, 0), (0, 1), (0, 2), (0, 3)]) # The pos corresponding to data A1 is {0, 0}, A2 is {0, 1}, A3 is {0, 2}, and A4 is {0, 3}
context.set_multi_outputs(0, [msg0, msg1, msg2, msg3], cfg)
# UDF output mode 2: single output, applicable to the scenario where users split data one by one, and the data split first can be processed first.
cfg = ff.BalanceConfig(1, 4, ff.AffinityPolicy.NO_AFFINITY)
# Distribute based on the split msg0
cfg.set_data_pos([(0, 0)])
context.set_output(0, msg0, cfg)
# Distribute based on the split msg1
cfg.set_data_pos([(0, 1)])
context.set_output(0, msg1, cfg)
# Distribute based on the split msg2
cfg.set_data_pos([(0, 2)])
context.set_output(0, msg2, cfg)
# Distribute based on the split msg3
cfg.set_data_pos([(0, 3)])
context.set_output(0, msg3, cfg)
```

**An example of data merging is as follows.**

```python
# Graph construction phase
flow_node.set_balance_gather()
# Set the UDF output BalanceConfig
cfg = ff.BalanceConfig(1, 4, ff.AffinityPolicy.ROW_AFFINITY)# The parameters mean: row_num=1, col_num=4 (four data copies in total), affinity_policy=ff.AffinityPolicy.ROW_AFFINITY (row affinity)
# Each instance distributes data with the same row to the same subsequent target node (if the data merging node is a single node, you do not need to send data by specifying BalanceConfig in this way). The data merging module merges the received data.
cfg.set_data_pos([(0, 0)])
context.set_output(0, msg0, cfg)
```

## Streaming Input and Output

**Function introduction**

In DataFlow, streaming input means that a UDF can obtain multiple inputs in a single execution. Similarly, streaming output means that a UDF can produce multiple outputs in a single execution. For detailed introduction and examples, see "usage method".

When users construct a UDF using dataflow.pyflow or dataflow.method, if multiple input data need to be grouped into a batch, streaming input is recommended; if the output data need to be split into multiple parts for distribution, streaming output is recommended.

Streaming input is expressed through the parameter stream\_input='Queue', in which case the input parameter type is FlowMsgQueue, and users can retrieve data from the queue by themselves; streaming output is expressed through the yield generator function in Python, and the number of function outputs is consistent with the number of yield statements.

**Usage constraints**

In the streaming input scenario, the DataFlow framework does not support data alignment and exception transaction processing.

**Usage method**

**Streaming input usage example:**

```python
import dataflow as df

# Use the stream_input parameter to indicate that this function uses streaming input. The input parameter type is Queue. Each time func is executed, it retrieves data twice from input queue a and once from input queue b.
@df.pyflow(stream_input='Queue')
def func(a, b):
    data1 = a.get()
    data2 = a.get()
    data3 = b.get()
    return data1 + data2 + data3

@df.pyflow
class Foo():
    # Use the stream_input parameter to indicate that the function uses streaming input. The input parameter type is Queue. Each time func is executed, it retrieves data twice from input queue a and once from input queue b.
    @df.method(stream_input='Queue')
    def func(self, a, b):
        data1 = a.get()
        data2 = a.get()
        data3 = b.get()
        return data1 + data2 + data3
```

**Streaming output usage example:**

```python
import dataflow as df

# Use yield to indicate that the function uses streaming output. In this example, func is scheduled and executed once, and outputs results five times.
@df.pyflow
def func(a):
    for i in range(5):
        yield a + i


@df.pyflow
class Foo():
    # Use yield to indicate that the function uses streaming output. In this example, func is scheduled and executed once, and outputs results five times.
    @df.method()
    def func(self, a):
        for i in range(5):
            yield a + i
```
