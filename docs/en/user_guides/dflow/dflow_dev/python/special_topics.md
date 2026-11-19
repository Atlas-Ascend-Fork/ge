# Special Topics

<!-- md-trans-meta sourceCommit=f2e33566b0420150e0470a13f9852d17f1cfd42b translatedAt=2026-11-16T09:03:13.902Z pushedAt=2026-11-17T03:39:12.017Z nl=2 -->

## UDF Development Process

Currently, there are two methods for UDF development:

- Create a UDF via annotation, that is, develop a UDF by decorating a function with dataflow.pyflow or dataflow.method.
- Create a UDF via a UDF project.

The differences between the two methods are as follows:

|Comparison Item|Create a UDF via Annotation|Create a UDF via Project|
|--|--|--|
|Advantages and Disadvantages|Advantages: The usage is simple and direct. Any code logic can be encapsulated as a UDF through annotation, without modifying the original code logic of the user code, resulting in low modification cost. Streaming input and streaming output are supported. Disadvantages: Usage restrictions exist in some scenarios.|Advantages: API calls are more flexible. UDF calls in NN scenarios are supported. Disadvantages: The usage is more cumbersome. Invasive modification of the user source code is required, and UDF-related APIs are used for development.|
|Usage Restrictions|UDF calls in NN scenarios are not supported.|None|
|Recommended Scenarios|When UDF calls in NN scenarios are not used, this method is recommended for creating a UDF.|For UDF calls in NN scenarios, use this method to create a UDF.|

### Environment Preparation

- Before UDF development, install the driver, firmware, and the development suite package Ascend-cann-toolkit. For details, see [CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum).
- Before Python UDF development, install the Python dependency packages pybind11 and jinja2. pybind11 is used to convert objects between C++ and Python. jinja2 is used to quickly generate project templates.
- Configure the environment variable for the CANN software package installation path ASCEND\_INSTALL\_PATH. During UDF compilation, the dependent header files and .so files are located according to this environment variable. If it is not set, the default value of this environment variable is "/usr/local/Ascend". The configuration example is as follows.

    ```shell
    export ASCEND_INSTALL_PATH=CANN package installation path
    ```

### Creating a UDF via Annotation

**Development Process**

The process of UDF development through annotation is shown in the following figure:

![](figures/260109170200356.png)

**Creating a UDF**

Using the function that implements the add function as an example, the following describes how to create a UDF through the dataflow.pyflow and dataflow.method annotations. Refer to the following example:

```python
import dataflow as df

# Decorate a common function with the pyflow annotation to build the common function as a UDF
@df.pyflow()
def add(a, b):
    return a + b

# Decorate a class with the pyflow annotation and decorate a class method with method to build the class method as a UDF
@df.pyflow
class Add():
    @df.method()
    def add(self, a, b):
        return a + b
```

**Building a FlowGraph**

After the UDF is constructed using annotation, construct a FlowNode using the fnode method and use it in the construction of the FlowGraph. The example is as follows:

```python
# Define the input
data0 = df.FlowData()
data1 = df.FlowData()

# Define a FlowNode
flow_node0 = add.fnode()
flow_node1 = Add.fnode()

# Build the edge relationship
flow_node0_out = flow_node0(data0, data1)
flow_node1_out = flow_node1.add(flow_node0_out, data1)

# Build the FlowGraph
dag = df.FlowGraph([flow_node1_out])
```

### Create a UDF via Project

**Development Process**

The following figure shows the process of UDF development via a project:

![](figures/260109170411587.png)

**UDF Project Creation**

UDF projects are classified into the following types. Create a project according to the usage scenario.

- Implement custom functions through a UDF: In this scenario, the user does not call third-party dependency libraries or implemented NN functions, and implements custom functions entirely by writing UDF-related files.
- Implement custom functions by calling NN: In this scenario, the functions required by the user can be carried by existing NN functions, and the user only needs to call the NN functions through the UDF.

Create a project using the tool. For details, see [UDF Python Project Creation Tool](appendices.md#udf-python-project-creation-tool).

**Custom Function Development**

During UDF Python development, custom processing functions are written. This section describes the UDF development process. The code in this part can be automatically generated through a template.

The UDF implementation includes two parts:

- UDF implementation file: the code implementation of user-defined function functions, with the file extension .py. According to the scenario, it is divided into the UDF implementation file (implementing custom functions through a UDF) and implementing custom functions by calling NN.
- UDF encapsulation and registration file: a cpp file generated by the user through the tool or customized according to the specification. It is used to obtain the custom Python UDF, encapsulate it into a C++ function, and call FLOW\_FUNC\_REGISTRAR to complete the function registration.

> [!NOTE]Note
>
>- In the single-P scenario, one input corresponding to multiple outputs, one input corresponding to one output, or multiple inputs corresponding to one output is supported.
>- In the 2P scenario, only one input corresponding to one output is supported.
>- The following uses the function that implements the add function as an example to describe how to perform UDF development, compilation, graph construction, and verification.

**UDF Implementation File (Implementing Custom Functions Through a UDF)**

The user needs to develop user-defined functions in the **workspace/src\_python/xxx.py** file of the project. The **func\_add.py** is used as an example.

**Function Analysis:**

This function implements the Add function. It converts the two inputs into numpy arrays, performs the addition operation, and outputs the operation result.

**Determine the Inputs and Outputs:**

The Add function has two inputs and one output.

**Note:**

The function name defined by the user in the Python file must be consistent with the name used to obtain the Python module in the cpp file.

For example, if the user defines the initialization function as init\_flow\_func in the Python file, when searching for the Python module attribute in the Init function of cpp, init\_flow\_func should be searched. For a specific example, see the following code.

**Function Implementation:**

It includes initialization functions and implementation functions (there can be multiple implementation functions).

When writing UDF code, the user usually needs to import the dataflow.data\_type and dataflow.flow\_func modules.

flow\_func exposes the common APIs for UDF development, and data\_type encapsulates the common data types. Users can import and view the specific module content.

```python
import dataflow.flow_func as ff
import dataflow.data_type as dt
# Define the class
class Add():
# The class name is related to the actual UDF function to be developed
```

- Initialization function, which performs the initialization action. The example is as follows.

    ```python
        @ff.init_wrapper() # This description is a required item. The wrapper converts the C++ object and the Python open object.
        def init_flow_func(self, meta_params):  # The input parameters must have exactly meta_params, and the type is MetaParams.
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
    @ff.proc_wrapper() # This description is a required item. The wrapper converts the C++ object and the Python open object.
    def Add1(self, run_context, input_flow_msgs): # The input parameters must have exactly a MetaRunContext type object and a FlowMsg type object.
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
        # Obtain the tensor, further obtain the array data in the tensor, and perform the addition operation.
        a[...] = np1 + np2
        ff.logger.event("prepare to set output in add1")
        if run_context.set_output(0, out) != ff.FLOW_FUNC_SUCCESS:
            ff.logger.error("set output failed")
            return ff.FLOW_FUNC_FAILED
        self.count += 1
        return ff.FLOW_FUNC_SUCCESS

    ```

- If the user needs to define multiple functions, multiple implementation functions can be defined in the same class.

    The following example shows a class that includes two implementation functions: Add1 and Add2.

    ```python
    @ff.proc_wrapper()
    def Add1(self, run_context, input_flow_msgs):
        xxxx

    @ff.proc_wrapper()
    def Add2(self, run_context, input_flow_msgs):
        xxxx
    ```

**UDF Implementation File (Implementing Custom Functions by Calling NN)**

The user needs to develop user-defined functions in the "workspace/src\_python/xxx.py" file of the project.

**Function Analysis:**

This function implements the Add function.

**Determine the Inputs and Outputs:**

It includes two inputs and one output.

**Function implementation:**

Implements initialization functions and implementation functions (there can be multiple implementation functions).

When writing UDF code, the user usually needs to import the dataflow.data\_type and dataflow.flow\_func modules

flow\_func provides common APIs for UDF development, and data\_type encapsulates common data types. Users can import and view the specific module content.

```python
import dataflow.flow_func as ff
import dataflow.data_type as dt
# Define the class
class Add():
# The class name is related to the actual UDF function being developed
```

- Initialization function, which performs initialization actions. Example:

    ```python
        @ff.init_wrapper() # This description is a required item. The wrapper converts C++ objects and Python open objects
        def init_flow_func(self, meta_params):  # The input parameters must have exactly meta_params, whose type is MetaParams
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

    Example:

    ```python
    @ff.proc_wrapper() # This description is a required item. The wrapper converts C++ objects and Python open objects
    def AddNN(self, run_context, input_flow_msgs): # The input parameters must have exactly a MetaRunContext type object and a FlowMsg type object
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

- If the user needs to define multiple functions, multiple implementation functions can be defined in the same class.

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

To develop a UDF in Python, in addition to the Python file, a C++ API call file is also required. It is recommended to generate it directly using the [tool](appendices.md#udf-python-project-creation-tool), or it can be customized. Example:

The C++ class must inherit MetaMultiFunc. The core content of the C++ file includes three parts

**Init function**

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

            constexpr const char *pyModuleName = "func_add"; // Keep consistent with the Python UDF file name, such as func_add in the above example
            constexpr const char *pyClzName = "Add";
            FLOW_FUNC_LOG_INFO("Load py module name: %s", pyModuleName);
            auto module = py::module_::import(pyModuleName);
            FLOW_FUNC_LOG_INFO("%s.%s import success", pyModuleName, pyClzName);
            pyModule_ = module.attr(pyClzName)();
            if (CheckProcExists() != FLOW_FUNC_SUCCESS) {
                FLOW_FUNC_LOG_ERROR("%s.%s check proc exists failed", pyModuleName, pyClzName);
                return FLOW_FUNC_FAILED;
            }
            if (py::hasattr(pyModule_, "init_flow_func")) { // Keep consistent with the initialization function name defined in the Python UDF, such as init_flow_func in the above example
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

**proc() function:**

```cpp
    int32_t AddProc1(
        const std::shared_ptr<MetaRunContext> &runContext, const std::vector<std::shared_ptr<FlowMsg>> &inputFlowMsgs)
    {
        FLOW_FUNC_LOG_INFO("enter");
        PyAcquire();
        ScopeGuard gilGuard([this]() { PyRelease(); });
        int32_t result = FLOW_FUNC_SUCCESS;
        try {
            result = pyModule_.attr("Add1")(runContext, inputFlowMsgs).cast<int32_t>(); // The attr name here is consistent with the processing function defined in the Python UDF, such as Add1, Add2, or Sub1, AddNN in the above example
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
// If the user writes multiple processing functions in the same UDF Python class, multiple FUNCs can be implemented here by calling different Python attrs through multiple C++ functions
    int32_t AddProc2(
        const std::shared_ptr<MetaRunContext> &runContext, const std::vector<std::shared_ptr<FlowMsg>> &inputFlowMsgs)
```

Use the FLOW\_FUNC\_REGISTRAR macro to declare the implementation class as the func name and register it with the UDF framework.

```cpp
FLOW_FUNC_REGISTRAR(Add)
    .RegProcFunc("Add1", &Add::AddProc1)
    .RegProcFunc("Add2", &Add::AddProc2);
```

**UDF Project Compilation**

Create the build and release directories under the udf\_py\_ws\_sample directory:

After compilation, the successfully compiled .so file is generated in the workspace/release directory.

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

4. Compile the project
    - In the non-cross-compilation scenario (where the development environment and operating environment have the same architecture), the example is as follows: compile an x86\_64 type .so file, specify the target lib as libadd.so, and use g++ as the compilation tool.

        ```shell
        cmake .. -DTOOLCHAIN=g++ -DRELEASE_DIR=../release -DRESOURCE_TYPE=X86 -DUDF_TARGET_LIB=add && make
        ```

        - -DTOOLCHAIN: the name of the compilation tool, for example, g++.
        - -DRELEASE\_DIR: the storage path of the target file after compilation.
        - -DRESOURCE\_TYPE: the resource type to compile. The value can be X86, Aarch, or Ascend.
            - X86: when the target .so file needs to be deployed on an X86-architecture Host, configure it as X86.
            - Aarch: when the target .so file needs to be deployed on an Aarch Host, configure it as Aarch.
            - Ascend: when the target .so file needs to be deployed on the Device, configure it as Ascend.

        - -DUDF\_TARGET\_LIB: the name of the target .so file after compilation. For example, if it is set to add, libadd.so is actually generated.

    - In the cross-compilation scenario (where the development environment is x86\_64 and the operating environment is aarch64), compile an Ascend type .so file, specify the target lib as libadd.so, and use \$\{INSTALL\_DIR\}/toolkit/toolchain/hcc/bin/aarch64-target-linux-gnu-g++ as the compilation tool. Replace \$\{INSTALL\_DIR\} with the file storage path after CANN software installation. Taking installation by the root user as an example, the default storage path after installation is /usr/local/Ascend/cann.

        Example:

        ```shell
        cmake .. -DTOOLCHAIN=${INSTALL_DIR}/toolkit/toolchain/hcc/bin/aarch64-target-linux-gnu-g++ -DRELEASE_DIR=../release -DRESOURCE_TYPE=Ascend -DUDF_TARGET_LIB=add && make
        ```

**Build FlowGraph**

After constructing the UDF, build the FlowGraph through FlowNode and FuncProcessPoint. Example:

```python
# Define the input
data0 = df.FlowData()
data1 = df.FlowData()

# Define the FlowNode that implements the Add function through FuncProcessPoint
pp0 = df.FuncProcessPoint(compile_config_path="./add_func.json")
flow_node0 = df.FlowNode(input_num=2, output_num=1)
flow_node0.add_process_point(pp0)

# Build the edge relationship
flow_node0_out = flow_node0(data0, data1)

# Build the FlowGraph
dag = df.FlowGraph([flow_node0_out])
```

## Specifying the Deployment Location of the DataFlow Node

**Feature Overview**

The configuration file can be used to specify the deployment location of DataFlow graph nodes. This function implements multi-card multi-instance deployment, distributes the computation workload, and improves performance.

**Constraints**

- When specifying deployment, the deployment location of all nodes must be specified.
- Specify the deployment configuration file according to the ge.experiment.data\_flow\_deploy\_info\_path in the options parameter of AddGraph. The file must exist and be in the correct format. For the specific format requirements, see **"Configuration File Format"**.
- The deployment location must be consistent with the actual deployable location of the node. For example, if a UDF supports only compiling Host release packages, it cannot be specified to be deployed on the Device. Similarly, if a node can only be deployed on the Device, it cannot be specified to be deployed on the Host.
- The same node cannot be configured repeatedly.
- The same node does not support being deployed on both the Host and the Device at the same time.

**Method**

The following shows a usage example.

```python
flow_node0 = df.FlowNode(input_num=1, output_num=2)
flow_node0_out = flow_node0(data0)

# Build a FlowGraph. This section focuses on the use of options. For the actual construction of a dataflow graph, see other sections.
options = {
    "ge.experiment.data_flow_deploy_info_path":"./data_flow_deploy_info.json"
}
dag = df.FlowGraph([out for out in flow_node0_out], options)
```

**Configuration file format**

The following shows the format requirements of a typical configuration file data\_flow\_deploy\_info.json.

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

**Table 1** Configuration item description

|Configuration Item|Description|
|--|--|
|keep_logic_device_order|Whether to deploy instances in the order of the device_list configured by the user.<br>Values are as follows:<br>- true: Yes. In the multi-instance scenario, Devices are sorted and deployed according to the device_list configured by the user.<br>- false: No. In the multi-instance scenario, Devices are sorted and deployed according to the internal implementation logic of the framework.<br>Default value: false.|
|**batch_deploy_info**|-|
|flow_node_list|List of FlowNode node names. One or more names are supported. When multiple names are used, separate them with half-width commas.|
|logic_device_list|Deployment location of DataFlow graph nodes.<br>**Format**: clusterid:serverid:deviceid:numaid (pgid/dieid). The meaning of each field is as follows:<br>- clusterid: ID of the cluster, which is fixed to 0.<br>- serverid: ID of the server node. Configure it as node_id in numa_config.json.<br>- deviceid: logical number of the device, corresponding to the item order of the "node>item_list" field in the numa_config file specified by the environment variable RESOURCE_CONFIG_PATH, starting from 0.<br>- numaid (pgid/dieid): number of the compute unit in a single device.<br> **Typical scenarios are as follows**.<br>When heavy_load of FunctionPp is true, it is deployed on the Host node corresponding to the specified node. When deploying to multiple Devices, configure as follows:<br>- Configure one by one: for example, "0:0:0:0,0:0:0:1" indicates that the node is deployed as multiple instances on the two devices "0:0:0:0" and "0:0:0:1".<br>- Configure by device range: for example, "0:0:0~1:0~1", where "~" indicates a range, and the value before "~" must be less than or equal to the value after it. "0:0:0~1:0~1" indicates that the node is deployed as multiple instances on the two compute units of the two devices "0:0:0:0", "0:0:0:1", "0:0:1:0", and "0:0:1:1".<br>When a FlowNode is the parent node that calls a sub-FlowGraph, multiple instances cannot be configured.|
|invoke_list (optional. If not configured, the deployment node of the subgraph is the same as that of the parent node)|-|
|invoke_name|Name of the InvokedClosure nested in the FlowNode node, corresponding to the parameter name of the AddInvokedClosure API.|
|deploy_info_file|Specifies the deployment location of the DataFlow subgraph node through this configuration item. When the InvokedClosure corresponding to invoke_name is a DataFlow subgraph, the deployment location file corresponding to the DataFlow subgraph can be configured, for example, ./data_flow_invoke_flow_graph_deploy_info.json. For the fields and format requirements contained in this file, see data_flow_deploy_info.json.|
|logic_device_list|Specifies the deployment location of the DataFlow subgraph or AscendGraph subgraph node through this configuration item. The configuration method is the same as logic_device_list in batch_deploy_info.<br>For the same subgraph node, logic_device_list and deploy_info_file cannot be configured at the same time. Otherwise, an error is reported.<br>For an AscendGraph subgraph node, the number of instances of logic_device_list and that of the parent node's logic_device_list must be the same. Otherwise, an error is reported.|

## DataFlow Subgraph Compilation Cache

**Feature Overview**

When the DataFlow compilation cache function is enabled and the subgraph of a single DataFlow node is modified, only the modified subgraph is incrementally compiled, and the remaining subgraphs use the cache, thereby reducing compilation time consumption.

**Constraints**

- This function supports FlowGraph but does not support AscendGraph.
- Models with resource-type operators are not supported currently.
- The cache does not guarantee cross-version compatibility. If the version is upgraded, the cache directory must be cleared and recompiled to generate the cache.
- After the DataFlow subgraph changes, the original cache file becomes unavailable. The user needs to manually delete the cache files in the cache directory and recompile to generate the cache file.

**Method**

1. Enable the cache configuration.

    The following shows a configuration example.

    ```python
        # Initialize.
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

        # Build a FlowGraph. This section focuses on the use of options. For the actual construction of a dataflow graph, see other sections.
        options = {
            "ge.graph_key":"test_graph_00"
        }
        dag = df.FlowGraph([out for out in flow_node0_out], options)
    ```

    **Table 1** Parameters

|Parameter|Meaning|
|--|--|
|ge.graph_compiler_cache_dir|Disk cache directory for graph compilation. It is used together with ge.graph_key. The graph compilation disk cache function takes effect only when both ge.graph_compiler_cache_dir and ge.graph_key are configured as non-empty.<br>The configured cache directory must exist. Otherwise, compilation fails.<br>After the graph changes, the original cache file becomes unavailable. The user needs to manually delete the cache files in the cache directory, including the model cache file, index file, and variable format file, and recompile to generate the cache file.|
|ge.graph_key|Unique identifier of the graph. The recommended value contains only uppercase and lowercase letters (A-Z, a-z), digits (0-9), underscores (_), and hyphens (-), and is no longer than 128 characters. If the condition is not met, the system reports an error.|

    After compilation is complete, a cache file is generated in the specified directory. For details, see the cache file generation rules.

2.  After the graph changes, if recompilation is required, perform the following steps.

    > [!NOTE]Note
    >Graph changes include: modifying the graph or config.json corresponding to GraphProcessPoint, modifying the implementation file of the UDF, and modifying the deployment information.
    >1.  Manually delete the files cached for the entire graph (including the model cache file, index file, and variable format file. For details, see the cache file generation rules).
    >2.  Recompile.
    >    After compilation is complete, a cache file is generated in the specified directory. For details, see the cache file generation rules.

**Cache file generation rule**

The generated files include:

- Model cache file
- Index file, which helps users quickly find the corresponding cache file through graph\_key. The following is an example of the index file content:

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

- Variable format file, generated only when variables exist in the graph. It is used by the framework to match the model cache file. If the variable format in the graph corresponding to graph\_key changes, the previously cached file cannot be directly restored for use. In this scenario, the compilation process is triggered again to regenerate the cache file.
- If the external weight feature is enabled, that is, the ge.externalWeight parameter is configured in the options and set to 1, a weight directory is also generated under the path specified by the **ge.graph\_compiler\_cache\_dir** parameter to save the weight information of the Const/Constant nodes in the original network.
- DataFlow subgraph cache folder, generated only in the FlowGraph scenario. It is named graph\_key and stores the cache files generated during DataFlow subgraph compilation.

File naming rules:

- When the value of ge.graph\_key contains only uppercase and lowercase letters (A-Z, a-z), digits (0-9), underscores (\_), and hyphens (-), and is no longer than 128 characters
  - The index file is named: ge.graph\_key + ".idx".
  - The model cache file is named ge.graph\_key + current time + ".om".
  - The variable format file is named ge.graph\_key + current time + ".rdcpkt".

- If the preceding conditions are not met, the system reports an error.

## DataFlow Offline Compilation

**Feature Overview**

DataFlow offline compilation refers to compiling in the development environment and loading and deploying in the operating environment, thereby decoupling compilation from graph building.

**Constraints**

The numa_config.json of the execution environment must be consistent with that at compile time, except for the ipaddr field.

**Method**

1. Enable graph compilation cache. The following is an example.

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

        # Build the FlowGraph. This section focuses on the use of options. For the actual construction of the dataflow graph, see other sections.
        options = {
            "ge.graph_key":"test_graph_00"
        }
        dag = df.FlowGraph([out for out in flow_node0_out], options)
    ```

2. Configure the development environment.

    Configure the numa\_config.json file of the target execution environment through the environment variable RESOURCE\_CONFIG\_PATH. The following is an example:

    ```shell
    export RESOURCE_CONFIG_PATH=numa_config.json //Used to set the storage path of the heterogeneous resource description information file.
    ```

    > [!NOTE]Note
    >**numa\_config.json** For details, see [numa\_config.json configuration](appendices.md#numa_configjson-configuration). The ipaddr field can use any IP.

3. Generate the graph.
    1. Call the feed\_data API to compile the graph and generate the cache file.

        The following is an example.

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

    2. Deploy offline.

        Copy the model cache file, index file, and variable format file from the graph\_compiler\_cache\_dir path in the development environment to the graph\_compiler\_cache\_dir path in the operating environment. For specific files, see the cache file generation rules.

        The constraints for Atlas A2 series products are as follows:

        The numa\_config.json of the execution environment must be consistent with that at compile time, except for the ipaddr field.

## Data Alignment

**Feature Overview**

**Before learning about data alignment, you need to understand what an data pair is.**

**Figure 1** Data pair schematic diagram
![](figures/data_for_schematic.png "Data pair schematic diagram")

- Model X is defined as the earliest source of the data. It has two output queues, and the output data groups \[A1',B1'\], \[A2',B2'\], and \[A3',B3'\] are related to each other.
- Model A and Model B are defined as data processing nodes deployed as multiple instances on multiple cards. Model A processes data stream A', and Model B processes data stream B'. Because of multi-instance deployment, the output order becomes disordered after processing and cannot follow the original order output by Model X.
- Model C is defined as the data aggregation node, which takes data from data stream A and data stream B group by group for processing. The pairing relationship \[A1, B1\],\[A2, B2\],\[A3, B3\] is required for normal processing.

**Data alignment means that a UDF node requires the input data to be an data pair.**

According to the data stream in the schematic diagram, if data pairing is not performed, the data order received by Model C follows the default receiving order of the framework, which is \[A3, B2\],\[A2, B1\],\[A1, B3\]. Because the data is disordered and cannot be processed directly, after the UDF node receives \[A3, B2\], it must first cache them and then wait for \[A2, B1\] before it can process \[A2, B2\]. Only after receiving \[A1, B3\] can it process \[A1, B1\] and \[A3, B3\].

**Constraints**

The framework's data alignment relies on transId and dataLabel as the basis for determining alignment. If one data is distributed to different instances, the order in which the data stream reaches the next node is not preserved due to differences in processing timing, which makes pairing impossible to guarantee.

**Method**

```python
dag = df.FlowGraph([flow_node_out])
dag.set_inputs_align_attrs(align_max_cache_num=256, align_timeout=60 * 1000, dropout_when_not_align=True) # Cache up to 256 groups of data. If the timeout is 1 minute, discard the data that is not aligned within the timeout.
```

## Configuring Dynamic Shape

**Feature Overview**

Configuring dynamic shape provides more shape range information to the graph compilation phase, which the compiler uses to compile efficient operators and better manage buffers. Setting maxOutputSize configures a more accurate maximum output value. The graph executor applies for the output buffer in advance before graph computation based on this configuration, improving performance.

**Constraints**

None

**Method**

Suppose there is a resize sunk graph with dynamic shape. The specific implementation is as follows. The sunk graph has two inputs: one is the image data to be resized, and the other is the target size information for resizing:

```python
def resize_bilinear(x: torch.Tensor, hw: torch.Tensor):
      return F.interpolate(x, size=[hw[0], hw[1]], mode="bilinear", align_corners=True)
```

For this dynamic sunk graph, the specific configuration is as follows.

- **ge.inputShape**: shape range of the dynamic input.
  - Setting by name is supported: "input\_name1:n1,c1,h1,w1;input\_name2:n2,c2,h2,w2", for example: "input\_name1:8\~20,3,5,-1;input\_name2:5,3\~9,10,-1". The specified node must be enclosed in double quotation marks, and nodes are separated by English semicolons. input\_name must be the node name in the network model before conversion. If the user knows the name of the data node, setting by name is recommended.

  - Supports setting by index: "n1,c1,h1,w1;n2,c2,h2,w2", for example: "8\~20,3,5,-1;5,3\~9,10,-1". The node name can be omitted, in which case the nodes are arranged in index order and separated by English semicolons. When setting the shape range by index, the data node must have the index attribute set to indicate which input it is, with the index starting from 0.

  - For dynamic dimensions, a shape range is represented by a tilde "\~"; a fixed dimension is represented by a fixed number; and an unbounded range is represented by -1.

- **ge.outputMaxSize (optional)**: specifies the maximum buffer of the graph output.
  - When this parameter is set, the dataflow framework uses it to allocate the buffer for the sunk graph output in advance, which reduces one data copy and improves execution performance.

        If the maximum output shape is \[1,3,2567,3440\] and the dtype is float32 (fp32 storage requires 4 bytes), outputMaxSize is calculated as 1x3x2567x3440x4=105965760.

        If the setting is incorrect and outputMaxSize is smaller than the actual output, an error is reported.

  - When this parameter is not set, the output buffer is applied for based on the actual situation when model execution is complete.

- **inputs\_tensor\_desc**
  - **data\_type**: data type of the tensor.
  - **shape**: shape information of the tensor. Set dynamic dimensions to -1.
  - **format**: the format information of each input must be set according to the actual situation. NCHW, NHWC, and ND formats are supported.

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

When a node requires more computing resources, deploying multiple copies can be considered to accelerate computation.

**Constraints**

After multi-instance deployment, because the computation timing between instances is inconsistent, the data alignment feature may need to be enabled to ensure data pairing.

**Method**

To enable multiple instances, see [Specifying the Deployment Location of the DataFlow Node](#specifying-the-deployment-location-of-the-dataflow-node).

For specific usage scenarios, the following two methods are available:

**Table 1** Parallel methods

|Parallel method|Scenario|Advantage|Disadvantage|
|--|--|--|--|
|Inter-request multi-instance parallelism|Multiple concurrent requests exist and the throughput needs to be improved|Simple to implement, no special code modification required|Cannot reduce the latency of a single request.|
|Split-request multi-instance acceleration|The latency of a single request needs to be reduced|Low latency for a single request|The code needs adaptation. The nodes before the multi-instance node need to split the data and specify the distribution policy. The multi-instance node needs to specify the data aggregation policy. After multi-instance processing, the data needs to be merged.|

- Parallel method 1: inter-request multi-instance parallelism:

    **Figure 1** Inter-request multi-instance parallelism
    ![](figures/multi_instance_run_in_parallel_req.png "Inter-request multi-instance parallelism")

    The figure shows a DataFlow model connected by Model X-\>Model A-\>Model B. Because Model A takes a long time to compute, Model A is split into two instances for acceleration. After the data stream (A, B, C, D) passes through Model X, it is polled and distributed to the two instances of Model A according to the transaction ID (data A and C are routed to instance 1, and data B and D are routed to instance 2). Then Model B performs aggregation.

- Parallel method 2: split-request multi-instance acceleration:

    In this scenario, the logic for data split and data merge needs to be added. Balanced distribution is performed by specifying BalanceConfig at output, controlling output distribution and aggregation (this function currently does not support the py\_flow annotation execution method).

    **Figure 2** Split-request multi-instance acceleration
    ![](figures/multi_instance_acceleration_split_req.png "Split-request multi-instance acceleration")

The original model is a DataFlow model connected by Model X-\>Model A-\>Model B. Because Model A takes a long time to compute, multi-instance acceleration is applied to Model A to reduce the end-to-end latency. The detailed steps are as follows.

1. Request data A passes through Model X and enters the Model data split module, where it is split into four parts (A1, A2, A3, A4). This operation needs to be implemented by the user according to their own business logic. The split data must carry the relevant data identifiers for subsequent nodes to identify and process (for example, adding information such as DataId/DataName to the data).
2. A1 and A3 are sent to instance 1 of Model A, and A2 and A4 are sent to instance 2 of Model A.
3. After processing, A1, A2, A3, and A4 are sent to the Model data merge module and merged into data A.
4. Data A is sent to Model B.

**The following is an example of data distribution.**

Based on the data A1, A2, A3, and A4 split by the user, the following describes how to distribute the data.

```python
# Graph construction phase
flow_node.set_balance_scatter()

# UDF output method 1: batch output, applicable to the scenario where the user completes the data split at once.
cfg = ff.BalanceConfig(1, 4, ff.AffinityPolicy.NO_AFFINITY) # The meaning of each parameter: row_num=1, col_num=4 (four parts of data in total), affinity_policy=ff.AffinityPolicy.NO_AFFINITY (no affinity)

cfg.set_data_pos([(0, 0), (0, 1), (0, 2), (0, 3)]) # The pos corresponding to data A1 is {0, 0}, A2 is {0, 1}, A3 is {0, 2}, and A4 is {0, 3}
context.set_multi_outputs(0, [msg0, msg1, msg2, msg3], cfg)
# UDF output method 2: single output, applicable to the scenario where the user splits the data one by one, and the data split first can be processed first.
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

**The following is an example of data merge.**

```python
# Graph construction phase
flow_node.set_balance_gather()
# UDF output BalanceConfig setting
cfg = ff.BalanceConfig(1, 4, ff.AffinityPolicy.ROW_AFFINITY)# The meaning of each parameter: row_num=1, col_num=4 (four parts of data in total), affinity_policy=ff.AffinityPolicy.ROW_AFFINITY (row affinity)
# Each instance distributes the data with the same row to the same subsequent target node (if the data merge node is a single node, it is not necessary to send in this way of specifying BalanceConfig). The data merge module merges the received data.
cfg.set_data_pos([(0, 0)])
context.set_output(0, msg0, cfg)
```

## Streaming Input and Output

**Feature Overview**

In DataFlow, streaming input means that a UDF can obtain multiple inputs in one execution. Streaming output means that a UDF can output multiple times in one execution. For detailed introduction and examples, see **Method**.

When the user constructs a UDF using dataflow.pyflow or dataflow.method, if multiple input datas need to be grouped into a batch, streaming input is recommended. If the output data needs to be split into multiple parts for distribution, streaming output is recommended.

Streaming input is represented by the parameter stream\_input='Queue'. In this case, the input parameter type is FlowMsgQueue, and the user can obtain data from the queue by themselves. Streaming output is expressed through the yield generator function in Python, and the number of function outputs is consistent with the number of yields.

**Constraints**

In streaming input scenarios, the DataFlow framework does not support data alignment or exception transaction handling.

**Method**

**Streaming input usage example:**

```python
import dataflow as df

# Use the stream_input parameter to indicate that this function uses streaming input. The input parameter type is Queue. In one execution, func obtains data twice from input queue a and once from input queue b.
@df.pyflow(stream_input='Queue')
def func(a, b):
    data1 = a.get()
    data2 = a.get()
    data3 = b.get()
    return data1 + data2 + data3

@df.pyflow
class Foo():
    # Use the stream_input parameter to indicate that this function uses streaming input. The input parameter type is Queue. In one execution, func obtains data twice from input queue a and once from input queue b.
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
