# Special Topics

<!-- md-trans-meta sourceCommit=73cc22fa193c049e7877714b651cc02a774ef134 translatedAt=2026-10-28T13:06:57.542Z pushedAt=2026-10-29T04:52:20.522Z nl=2 -->

## UDF Development

### Development Process

The workflow for UDF development is shown in the following figure.

**Figure 1** UDF development process
![](figures/udf_develop_process.png "UDF development process")

The development steps are described in Table 1.

**Table 1** UDF development steps

|Step|Description|References|
|--|--|--|
|Environment Preparation|Prepare the development environment and operating environment required for UDF development and runtime verification.|Development Preparation|
|Project Creation|Create a UDF project.|Project Creation|
|Development Process|The user inherits the MetaFlowFunc base class from the meta_flow_func.h file and overrides the Init and Proc functions to implement the UDF computation logic.|Development Process (Calling the MetaFlowFunc Class)|
|Project Compilation|Compile the user-defined function cpp file into an so file.|Project Compilation|
|Build a graph using the DataFlow API|Use the DataFlow API to build and execute a graph.|Build a graph using the DataFlow API|
|ST Test|System Test (ST) verifies the correctness of the UDF in a real hardware environment.|Test|

### Development Preparation

#### Environment Preparation

- Before UDF development, install the driver firmware and the development suite package Ascend-cann-toolkit. For details, see *[CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum)*.
- Configure the environment variable for the CANN software package installation path ASCEND\_INSTALL\_PATH. During UDF compilation, the dependent header files and .so files are located based on this environment variable. If it is not set, the default value is "/usr/local/Ascend". The following is a configuration example.

    ```shell
    export ASCEND_INSTALL_PATH=CANN package installation path
    ```

#### Project Creation

UDF projects are divided into the following categories. Create a project based on your usage scenario.

- Implement custom functions through UDF: In this scenario, users do not call third-party dependency libraries or existing NN functions, but fully implement custom functions by writing UDF-related files.
- Implement custom functions by calling NN: In this scenario, the functions required by users can be carried by existing NN functions, and users only need to call the NN functions through UDF.
- Implement custom functions by calling third-party dependency libraries: In this scenario, the functions required by users already exist in third-party dependency libraries, and users only need to call the relevant functions in the third-party dependency libraries through UDF.

You can directly download the [project sample](https://gitcode.com/cann/ge/tree/master/examples/dflow/udf_workspace) and develop based on it. The following UDF project directory example includes the three scenarios. Download the corresponding sample based on your usage scenario, and develop the UDF deliverables in the corresponding directory according to the following rules.

```cpp
├── 01_udf_add
│ ├── CMakeLists.txt
│ └── add_flow_func.cpp // add function defined by a single-func interface
├── 02_udf_call_add_nn
│ ├── CMakeLists.txt
│ └── call_nn_flow_func.cpp // UDF calls the TensorFlow model
├── 03_udf_add_multi_func
│ ├── CMakeLists.txt
│ └── add_flow_func.cpp // add function defined by a multi-func interface, can be used with sample 04
├── 04_control_func
│ ├── CMakeLists.txt
│ └── control_func.cpp // activates one of the multi-funcs defined in 03 based on input, can be used with sample 03
└── README.md
```

### Development Process (Calling the MetaFlowFunc Class)

**Overview**

During UDF development, you can call the MetaFlowFunc class to write a custom single-func handler function, or call the MetaMultiFunc class to write a custom multi-func handler function. This section describes the development process of a UDF by calling the MetaFlowFunc class.

A UDF implementation consists of two parts:

- UDF implementation file: the code implementation of the user-defined function, with the .cpp file extension. Depending on the scenario, it implements a custom function through a UDF or through NN invocation.
- UDF registration: the implementation class is declared as a func name through the REGISTER\_FLOW\_FUNC macro and registered with the UDF framework.

> [!NOTE]
>
>- In the single-P scenario, one input can correspond to multiple outputs, one input can correspond to one output, or multiple inputs can correspond to one output.
>- In the 2P scenario, only one input corresponding to one output is supported.
>- The following uses a function that implements the Add operation as an example to describe how to develop, compile, construct a graph for, and verify a UDF.

**UDF Implementation File (Implementing a Custom Function Through a UDF)**

You need to develop the user-defined function in the "workspace/xx.cpp" file of the project. The following uses “add\_flow\_func.cpp” as an example.

**Function Analysis:**

This function implements the Add operation, supporting Add for both float and int types.

**Clear input and output:**

Contains two inputs and one output.

**Function implementation:**

The user inherits the MetaFlowFunc base class in the meta\_flow\_func.h file and overrides the Init and Proc functions.

If resources need to be released, handle them in the destructor.

```cpp
namespace FlowFunc {
class AddFlowFunc: public MetaFlowFunc{};
}
```

> [!NOTE]
> If the user needs to use the TPRT (Task Parallel Runtime) concurrency feature, refer to [Development Process (Calling the MetaMultiFunc Class)](#development-process-calling-the-metamultifunc-class) for the implementation of the following Init\(\) and Proc\(\) functions.

- Init\(\): Execute initialization actions, such as variable initialization and property retrieval. In this example, the out\_type property set in FunctionPp during DataFlow graph construction needs to be obtained. An example is shown as follows:

    ```cpp
    int32_t Init() override
        {
            // Obtain the out_type property set during operator graph construction through the GetAttr method of the MetaContext class. The property value is stored in the private variable outDataType_ of the class and is used to convert the source type to the outDataType_ type.
            auto getRet = context_->GetAttr("out_type", outDataType_);
            if (getRet != FLOW_FUNC_SUCCESS) {
                FLOW_FUNC_RUN_LOG_ERROR("GetAttr dType not exist. ");
                return getRet;
            }
            FLOW_FUNC_RUN_LOG_INFO("Add flow func init success");
            return FLOW_FUNC_SUCCESS;
        }
    ```

- Proc\(\): User-defined computation handler function. The UDF framework calls this method after receiving the input Tensor data.

    ```cpp
    int32_t Proc(const std::vector<std::shared_ptr<FlowMsg>> &inputMsgs) override
    {
        FLOW_FUNC_LOG_DEBUG("proc start");
        // In the Add method, there are only two inputs.
        if (inputMsgs.size() != 2) {
            // Return the error code defined in flow_func_defines.h.
            FLOW_FUNC_LOG_ERROR("add must have 2 inputs, but %zu", inputMsgs.size());
            return FLOW_FUNC_ERR_PARAM_INVALID;
        }

        // invalid input
        auto inputFlowMsg1 = inputFlowMsgs[0];
        auto inputFlowMsg2 = inputFlowMsgs[1];
        FLOW_FUNC_LOG_INFO("inputFlowMsg1.RetCode=%d, inputFlowMsg2.RetCode=%d", inputFlowMsg1->GetRetCode(), inputFlowMsg2->GetRetCode());
        // Check whether RetCode in the input msg is 0. RetCode is the processing result of the previous step. 0 indicates normal, and a non-zero value indicates an error.
        if (inputFlowMsg1->GetRetCode() != 0 || inputFlowMsg2->GetRetCode() != 0) {
            FLOW_FUNC_LOG_ERROR("invalid input");
            return -1;
        }
        MsgType MsgType1 = inputFlowMsg1->GetMsgType();
        MsgType MsgType2 = inputFlowMsg2->GetMsgType();
        // Check whether the message type in the input msg is tensor.
        if (MsgType1 == MsgType::MSG_TYPE_TENSOR_DATA && MsgType2 == MsgType::MSG_TYPE_TENSOR_DATA) {
            // Obtain the input Tensor.
            auto inputTensor1 = inputFlowMsg1->GetTensor();
            auto inputTensor2 = inputFlowMsg2->GetTensor();
            auto inputShape1 = inputTensor1->GetShape();
            // Apply for the output flow msg of the FlowFunc operator based on the input shape and the dataType of the output data.
            auto outputMsg = context_->AllocTensorMsg(inputShape1, outDataType_);
            if (outputMsg == nullptr) {
                FLOW_FUNC_LOG_ERROR("alloc Tensor msg failed");
                return -1;
            }
            // Call the AddTensor API in the third-party library to implement the Add function.
            if (AddDepend::Instance().AddTensor(inputTensor1, inputTensor2, outputMsg, outDataType_) != 0) {
                FLOW_FUNC_LOG_ERROR("add tensor failed");
                return -1;
            }
            // Output the calculation result.
            return context_->SetOutput(0, outputMsg);
        }
        // If the input message is not of the Tensor type, return an error.
        FLOW_FUNC_LOG_ERROR("MsgType is not Tensor.");
        return -1;
    }

    ```

**UDF Implementation File (Implementing Custom Functions by Calling NN)**

User needs to develop user-defined functions in the "workspace/xx.cpp" file of the project. The following uses “call\_nn\_flow\_func.cpp” as an example for introduction.

**Function Analysis:**

This function implements the Add functionality.

**Clear Input and Output:**

Contains two inputs and one output.

**Function Implementation:**

The user inherits the MetaFlowFunc base class from the meta\_flow\_func.h file and overrides the Init and Proc functions.

```cpp
class CallNnFlowFunc: public MetaFlowFunc{};
```

- Init\(\): Executes initialization actions, such as variable initialization and property retrieval. In this example, Init requires no processing and directly returns SUCCESS.

    ```cpp
    int32_t Init() override
    {
        FLOW_FUNC_RUN_LOG_INFO("call nn flow func init success");
        return FLOW_FUNC_SUCCESS;
    }
    ```

- Proc\(\): The user-defined calculation handler function. The UDF framework will call this method after receiving the input Tensor data.

    An example is shown as follows:

    ```cpp
    int32_t Proc(const std::vector<std::shared_ptr<FlowMsg>> &inputMsgs) override
    {
        std::vector<std::shared_ptr<FlowMsg>> outputMsgs;
        // Call RunFlowModel to execute the NN model invoked in the custom handler function. This NN model implements the Add function and is constructed by the user through IR graph construction. The execution result of this model is stored in outputMsgs.
        auto ret = context_->RunFlowModel(dependKey_.c_str(), inputMsgs, outputMsgs, 100000);
        if (ret != FLOW_FUNC_SUCCESS) {
            FLOW_FUNC_LOG_ERROR("Run flow model failed, ret=%d, dependKey=%s", ret, dependKey_.c_str());
            return ret;
        }
        // Output the result outputMsgs of the NN model through SetOutput.
        for (size_t i = 0; i < outputMsgs.size(); ++i) {
            ret = context_->SetOutput(i, outputMsgs[i]);
            if (ret != FLOW_FUNC_SUCCESS) {
                FLOW_FUNC_LOG_ERROR("Set output failed, ret=%d, index=%zu", ret, i);
                return ret;
            }
        }
        return FLOW_FUNC_SUCCESS;
    }
    // dependKey_ is a private data member defined in the class CallNnFlowFunc:
    private:
        std::string dependKey_{"invoke_graph"};

    ```

**UDF Registration**

Register the implementation class as the func name into the UDF framework through the REGISTER\_FLOW\_FUNC macro.

```cpp
REGISTER_FLOW_FUNC("call_nn", AddFlowFunc);
```

- call\_nn: user-defined function name, which needs to be customized by the user according to the function.
- AddFlowFunc: class name, which needs to be consistent with the class name defined in "UDF Implementation File (Implementing Custom Functions Through UDF)".

### Development Process (Calling the MetaMultiFunc Class)

**Overview**

During UDF development, you can call the MetaFlowFunc class to write a custom single-func handler function, or call the MetaMultiFunc class to write custom multi-func handler functions. This section describes the UDF development process by calling the MetaMultiFunc class.

A UDF implementation consists of two parts:

- UDF implementation file: the code implementation of the user-defined function, with the .cpp file extension. Depending on the scenario, it is divided into implementing user-defined functions through UDF and implementing custom functions by calling NN.
- UDF registration: the implementation class is declared as a func name through the FLOW\_FUNC\_REGISTRAR macro and registered with the UDF framework.

> [!NOTE]
>
> - In the single-P scenario, one input corresponding to multiple outputs, one input corresponding to one output, or multiple inputs corresponding to one output is supported.
> - In the 2P scenario, only one input corresponding to one output is supported.
> - The following uses a function that implements the add function as an example to describe how to perform UDF development, compilation, graph construction, and verification.

**UDF Implementation File (Implementing User-Defined Functions Through UDF)**

You need to develop the user-defined function in the "workspace/xx.cpp" file of the project. The following uses “add\_flow\_func.cpp” as an example.

**Function analysis:**

This function implements the Add functionality, supporting the Add operation for both float and int types. The first handler function implements a+b, and the second function implements 2a+b.

**Clear Input and Output:**

Each proc function contains two inputs and one output. The input queues of the two proc handler functions are not shared, while the output queue is shared.

**Notes:**

In the multi-func scenario, multiple handler functions are scheduled concurrently. Users need to consider multi-thread concurrency issues; adding locks may affect deterministic scheduling.

**Function Implementation:**

The user inherits the MetaMultiFunc base class from the meta\_multi\_func.h file and overrides the Init function and the custom multi-func handler functions.

If there are resources to be released, they need to be handled in the destructor.

```cpp
namespace FlowFunc {
class AddFlowFunc: public MetaMultiFunc{};
}
```

- Init\(\): Execute initialization actions, such as variable initialization and property retrieval. If no initialization operation is required in FlowFunc, this function does not need to be added. In this example, the out\_type property set in FunctionPp during DataFlow graph construction needs to be obtained. An example is shown as follows:

    ```cpp
    int32_t Init(const std::shared_ptr<MetaParams> &params) override
        {
            // Obtain the out_type property set during operator graph construction through the GetAttr method of the MetaParams class. The property value is stored in the private variable outDataType_ of the class and is used to convert the source type to the outDataType_ type.
            auto getRet = params->GetAttr("out_type", outDataType_);
            if (getRet != FLOW_FUNC_SUCCESS) {
                FLOW_FUNC_RUN_LOG_ERROR("GetAttr dType not exist. ");
                return getRet;
            }
            // Initialize the shared variables of the multi-func handler functions.
            setOutputCount_ = 0;
            // If TPRT concurrency needs to be enabled, add the following line.
            //(void)UdfTprt::Init(params);
            return 0;
        }
    ```

- template: template function of the Add implementation method.

    ```cpp
        // Template function of the Add1 implementation method
        template<typename srcT, typename dstT>
        void Add1(srcT *src1, srcT *src2, dstT *dst, size_t count)
        {
            for (size_t i = 0; i < count; ++i) {
                dst[i] = src1[i] + src2[i];
            }
            /* To enable TPRT concurrency for the operations in the three lines above, replace the three lines above with the five lines below (the following example uses ParallelFor; Submit+Wait can also be used).
            std::function<void(const int64_t index)> func = [src1, src2, dst]
               (const int64_t index) {
                dst[index] = src1[index] + src2[index];
            };
            UdfTprt::ParallelFor(0, count, func);
            */
        }
        // Template function of the Add2 implementation method
        template<typename srcT, typename dstT>
        void Add2(srcT *src1, srcT *src2, dstT *dst, size_t count)
        {
            for (size_t i = 0; i < count; ++i) {
                dst[i] = 2 * src1[i] + src2[i];
            }
            /* To enable TPRT concurrency for the operations in the three lines above, replace the three lines above with the seven lines below (the following example uses Submit+Wait; ParallelFor can also be used).
            UdfTprtTaskAttr attr;
            for (size_t i = 0; i < count; ++i) {
                UdfTprt::Submit([src1, src2, dst, i]() {
                    dst[i] = 2 * src1[i] + src2[i];
                }, {}, {}, attr);
            }
            UdfTprt::Wait();
            */
        }

        template<typename T>
        void Add1(T *src1, T *src2, void *dst, size_t count)
        {
            if (outDataType_ == TensorDataType::DT_FLOAT) {
                Add1(src1, src2, static_cast<float *>(dst), count);
            }
            if (outDataType_ == TensorDataType::DT_UINT32) {
                Add1(src1, src2, static_cast<uint32_t *>(dst), count);
            }
            else if (outDataType_ == TensorDataType::DT_INT64) {
                Add1(src1, src2, static_cast<int64_t *>(dst), count);
            }
        }
        template<typename T>
        void Add2(T *src1, T *src2, void *dst, size_t count)
        {
            if (outDataType_ == TensorDataType::DT_FLOAT) {
                Add2(src1, src2, static_cast<float *>(dst), count);
            }
            if (outDataType_ == TensorDataType::DT_UINT32) {
                Add2(src1, src2, static_cast<uint32_t *>(dst), count);
            }
            else if (outDataType_ == TensorDataType::DT_INT64) {
                Add2(src1, src2, static_cast<int64_t *>(dst), count);
            }
        }
    ```

- Multi-func handler function: a user-defined computation handler function. The UDF framework calls this method after receiving the input Tensor data.

    The code example is as follows.

    ```cpp
    // User-defined multi-func handler function name: "Proc1". The function name is defined by the user, and the input parameters and return value must be consistent with the prototype.
    int32_t Proc1(const std::shared_ptr<MetaRunContext> &runContext, const std::vector<std::shared_ptr<FlowMsg>> &inputFlowMsgs)
        {
            // Check whether the add operation has two inputs
            if (inputFlowMsgs.size() != 2) {
                FLOW_FUNC_LOG_ERROR("add must have 2 inputs, but %zu", inputMsgs.size());
                return -1;
            }
            auto inputFlowMsg1 = inputFlowMsgs[0];
            auto inputFlowMsg2 = inputFlowMsgs[1];
            // Invalid input validation: check whether the error code of the input message is non-zero
            if (inputFlowMsg1->GetRetCode() != 0 || inputFlowMsg2->GetRetCode() != 0) {
                FLOW_FUNC_LOG_ERROR("invalid input");
                return -1;
            }
            // Obtain the message type of the input data
            MsgType MsgType1 = inputFlowMsg1->GetMsgType();
            MsgType MsgType2 = inputFlowMsg2->GetMsgType();
            // Check whether the input data is of the Tensor type
            if (MsgType1 == MsgType::MSG_TYPE_TENSOR_DATA && MsgType2 == MsgType::MSG_TYPE_TENSOR_DATA) {
                // Obtain the Tensor from msg
                auto inputTensor1 = inputFlowMsg1->GetTensor();
                auto inputTensor2 = inputFlowMsg2->GetTensor();
                // Obtain the data type from the Tensor
                auto inputDataType1 = inputTensor1->GetDataType();
                auto inputDataType2 = inputTensor2->GetDataType();
                // Check whether the data types of the two inputs are consistent
                if (inputDataType1 != inputDataType2) {
                    FLOW_FUNC_LOG_ERROR("input type is not the same");
                    return -1;
                }
                // Obtain the shape information from the Tensor
                auto &inputShape1 = inputTensor1->GetShape();
                auto &inputShape2 = inputTensor2->GetShape();

                // Check whether the shapes of the two input tensors are consistent
                if (inputShape1 != inputShape2) {
                    FLOW_FUNC_LOG_ERROR("input shape is not the same");
                    return -1;
                }
                // Apply for the output msg information
                auto outputMsg = runContext->AllocTensorMsg(inputShape1, outDataType_);
                if (outputMsg == nullptr) {
                    FLOW_FUNC_LOG_ERROR("alloc Tensor msg failed");
                    return -1;
                }
                auto outputTensor = outputMsg->GetTensor();
                // Obtain the data size of the tensor from the Tensor
                auto dataSize1 = inputTensor1->GetDataSize();
                auto dataSize2 = inputTensor2->GetDataSize();

                if (dataSize1 == 0) {
                    return runContext->SetOutput(0, outputMsg);
                }
                // Obtain the data pointer of the tensor from Tensor.
                auto inputData1 = inputTensor1->GetData();
                auto inputData2 = inputTensor2->GetData();
                auto outputData = outputTensor->GetData();
                // Execute different add operations based on the data type.
                switch (inputDataType1) {
                    case TensorDataType::DT_FLOAT:
                        Add1(static_cast<float *>(inputData1), static_cast<float *>(inputData2), outputData, dataSize1 / sizeof(float));
                        break;
                    case TensorDataType::DT_INT16:
                        Add1(static_cast<int16_t *>(inputData1), static_cast<int16_t *>(inputData2), outputData, dataSize1 / sizeof(int16_t));
                        break;
                    case TensorDataType::DT_UINT16:
                        Add1(static_cast<uint16_t *>(inputData1), static_cast<uint16_t *>(inputData2), outputData, dataSize1 / sizeof(uint16_t));
                        break;
                    case TensorDataType::DT_UINT32:
                        Add1(static_cast<uint32_t *>(inputData1), static_cast<uint32_t *>(inputData2), outputData, dataSize1 / sizeof(uint32_t));
                        break;
                    case TensorDataType::DT_INT8:
                        Add1(static_cast<int8_t *>(inputData1), static_cast<int8_t *>(inputData2), outputData, dataSize1 / sizeof(int8_t));
                        break;
                    case TensorDataType::DT_INT64:
                        Add1(static_cast<int64_t *>(inputData1), static_cast<int64_t *>(inputData2), outputData, dataSize1 / sizeof(int64_t));
                        break;
                    default:
                        // For unsupported data types, set the error code output.
                        outputMsg->SetRetCode(100);
                        break;
                }
                // Multiple handler functions are scheduled concurrently. Users need to lock shared data themselves.
                std::unique_lock<std::mutex> lock(countMutex_);
                setOutputCount_++;
                return runContext->SetOutput(0, outputMsg);
            }
            FLOW_FUNC_LOG_ERROR("MsgType is not Tensor.");
            return -1;
        }
        // The user-defined multi-func handler function name is "Proc2". The function name is user-defined, and the input parameters and return value must be consistent with the prototype.
        int32_t Proc2(const std::shared_ptr<MetaRunContext> &runContext, const std::vector<std::shared_ptr<FlowMsg>> &inputFlowMsgs)
        {
            // Verify whether the add operation has two inputs.
            if (inputFlowMsgs.size() != 2) {
                FLOW_FUNC_LOG_ERROR("add must have 2 inputs, but %zu", inputMsgs.size());
                return -1;
            }
            auto inputFlowMsg1 = inputFlowMsgs[0];
            auto inputFlowMsg2 = inputFlowMsgs[1];
            // Validate invalid input: verify whether the error code of the input message is non-zero.
            if (inputFlowMsg1->GetRetCode() != 0 || inputFlowMsg2->GetRetCode() != 0) {
                FLOW_FUNC_LOG_ERROR("invalid input");
                return -1;
            }
            // Obtain the message type of the input data.
            MsgType MsgType1 = inputFlowMsg1->GetMsgType();
            MsgType MsgType2 = inputFlowMsg2->GetMsgType();
            // Verify whether the input data is of the Tensor type.
            if (MsgType1 == MsgType::MSG_TYPE_TENSOR_DATA && MsgType2 == MsgType::MSG_TYPE_TENSOR_DATA) {
                // Obtain the Tensor from the input msg.
                auto inputTensor1 = inputFlowMsg1->GetTensor();
                auto inputTensor2 = inputFlowMsg2->GetTensor();
                // Obtain the data type from the Tensor.
                auto inputDataType1 = inputTensor1->GetDataType();
                auto inputDataType2 = inputTensor2->GetDataType();

                // Verify whether the data types of the two inputs are consistent.
                if (inputDataType1 != inputDataType2) {
                    FLOW_FUNC_LOG_ERROR("alloc Tensor msg failed");
                    return -1;
                }
                // Obtain the shape information from the tensor.
                auto &inputShape1 = inputTensor1->GetShape();
                auto &inputShape2 = inputTensor2->GetShape();

                // Verify whether the shapes of the two input tensors are consistent.
                if (inputShape1 != inputShape2) {
                    FLOW_FUNC_LOG_ERROR("input shape is not the same");
                    return -1;
                }
                // Apply for the output Tensor information.
                auto outputMsg = runContext->AllocTensorMsg(inputShape1, outDataType_);
                if (outputMsg == nullptr) {
                    FLOW_FUNC_LOG_ERROR("alloc Tensor msg failed");
                    return -1;
                }
                auto outputTensor = outputMsg->GetTensor();
                // Obtain the data size of the Tensor from the Tensor.
                auto dataSize1 = inputTensor1->GetDataSize();
                auto dataSize2 = inputTensor2->GetDataSize();
                if (dataSize1 == 0) {
                    return runContext->SetOutput(0, outputMsg);
                }
                // Obtain the data pointer of the Tensor from the tensor.
                auto inputData1 = inputTensor1->GetData();
                auto inputData2 = inputTensor2->GetData();
                auto outputData = outputTensor->GetData();
                // Execute the specific add operation based on different data types.
                switch (inputDataType1) {
                    case TensorDataType::DT_FLOAT:
                        Add2(static_cast<float *>(inputData1), static_cast<float *>(inputData2), outputData, dataSize1 / sizeof(float));
                        break;
                    case TensorDataType::DT_INT16:
                        Add2(static_cast<int16_t *>(inputData1), static_cast<int16_t *>(inputData2), outputData, dataSize1 / sizeof(int16_t));
                        break;
                    case TensorDataType::DT_UINT16:
                        Add2(static_cast<uint16_t *>(inputData1), static_cast<uint16_t *>(inputData2), outputData, dataSize1 / sizeof(uint16_t));
                        break;
                    case TensorDataType::DT_UINT32:
                        Add2(static_cast<uint32_t *>(inputData1), static_cast<uint32_t *>(inputData2), outputData, dataSize1 / sizeof(uint32_t));
                        break;
                    case TensorDataType::DT_INT8:
                        Add2(static_cast<int8_t *>(inputData1), static_cast<int8_t *>(inputData2), outputData, dataSize1 / sizeof(int8_t));
                        break;
                    case TensorDataType::DT_INT64:
                        Add2(static_cast<int64_t *>(inputData1), static_cast<int64_t *>(inputData2), outputData, dataSize1 / sizeof(int64_t));
                        break;
                    default:
                        // not support
                        outputMsg->SetRetCode(100);
                        break;
                }
                // Multiple handler functions are scheduled concurrently. Users need to add locks to handle shared data.
                std::unique_lock<std::mutex> lock(countMutex_);
                setOutputCount_++;
                return runContext->SetOutput(0, outputMsg);
            }
            FLOW_FUNC_LOG_ERROR("MsgType is not Tensor.");
            return -1;
        }

    private:
        TensorDataType outDataType_;
        std::mutex countMutex_;
        uint32_t setOutputCount_;
    ```

**UDF Implementation File (Implement Custom Function by Calling NN)**

The user needs to develop user-defined functions in the "workspace/xx.cpp" file of the project. The following uses “call\_nn\_flow\_func.cpp” as an example for introduction.

**Function Analysis:**

This function implements the Add function.

**Clear Input and Output:**

It contains two inputs and one output.

**Function Implementation:**

The user inherits the MetaFlowFunc base class in the meta\_flow\_func.h file and overrides the Init function and the custom handler function.

```cpp
class CallNnFlowFunc: public MetaMultiFunc{};
```

- Init\(\): Executes initialization actions, such as variable initialization and property retrieval. In this example, Init does not require processing, so this function is not needed.
- Add\(\): A user-defined computation handler function. The UDF framework calls this method after receiving the input tensor data.

    An example is shown as follows:

    ```cpp
    // User-defined multi-func handler function name: "Add". This function name is defined by the user, and the input parameters and return value must be consistent with the prototype.
    int32_t Add(const std::shared_ptr<MetaRunContext> &runContext, const std::vector<std::shared_ptr<FlowMsg>> &inputFlowMsgs)
    {
        std::vector<std::shared_ptr<FlowMsg>> outputMsgs;
        // Use the RunFlowModel method of runContext to execute the NN model called in the custom handler function. This NN model implements the Add function and is implemented by the user through IR graph construction. The execution result of this model is stored in outputMsgs.
        auto ret = runContext->RunFlowModel("invoke_graph", inputMsgs, outputMsgs, 100000);
        if (ret != FLOW_FUNC_SUCCESS) {
            return ret;
        }
        // Output the result outputMsgs of the called NN model through SetOutput.
        for (size_t i = 0; i < outputMsgs.size(); ++i) {
            ret = runContext->SetOutput(i, outputMsgs[i]);
            if (ret != FLOW_FUNC_SUCCESS) {
                return ret;
            }
        }
        return FLOW_FUNC_SUCCESS;
    }
    ```

**UDF Registration**

Use the FLOW\_FUNC\_REGISTRAR macro to declare the implementation class as the func name and register it with the UDF framework.

```cpp
FLOW_FUNC_REGISTRAR(AddFlowFunc)
    .RegProcFunc("Proc1", &AddFlowFunc::Proc1)
    .RegProcFunc("Proc2", &AddFlowFunc::Proc2);
FLOW_FUNC_REGISTRAR(CallNnFlowFunc)
    .RegProcFunc("CallNn_Add", &CallNnFlowFunc::Add);
```

- Proc1: User-defined multi-func handler function name, which must be consistent with the handler function name defined in AddFlowFunc.
- Proc2: User-defined multi-func handler function name, which must be consistent with the handler function name defined in AddFlowFunc.
- Add: User-defined function name, which must be consistent with the handler function name defined in CallNnFlowFunc.
- AddFlowFunc: Class name, which must be consistent with the class name in "UDF Implementation File (Implementing Custom Functions Through UDF)".
- : Class name, which must be consistent with the class name in "UDF Implementation File (Implementing Custom Functions by Calling NN)".

### Project Compilation

Create the build and release directories under the udf\_workspace/01\_udf\_add directory:

After compilation is complete, the compiled .so file is generated in the workspace/udf\_add/release directory.

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

4. Configure environment variables. The following uses the default path after the Toolkit is installed by a non-root user as an example. Run the following command based on the actual path of set\_env.sh.

    ```shell
    source ${HOME}/Ascend/cann/set_env.sh
    ```

    The preceding environment variable configuration takes effect only in the current window. You can write the preceding commands to an environment variable configuration file (such as the .bashrc file) as needed.

5. Project compilation
    - In a non-cross-compilation scenario (where the development environment and the operating environment have the same architecture), the following is an example: compile the .so file of the x86\_64 type, specify the target lib as libadd.so, and use g++ as the compilation tool.

        ```shell
        cmake .. -DTOOLCHAIN=g++ -DRELEASE_DIR=../release -DRESOURCE_TYPE=X86 -DUDF_TARGET_LIB=add && make
        ```

        - -DTOOLCHAIN: name of the compilation tool, for example, g++.
        - -DRELEASE\_DIR: path for storing the target files after compilation.
        - -DRESOURCE\_TYPE: resource type to be compiled. The value can be X86, Aarch, or Ascend.
            - X86: Set this value when the target .so file is to be deployed on an X86 Host.
            - Aarch: Set this value when the target .so file is to be deployed on an Aarch Host.
            - Ascend: Set this value when the target .so file is to be deployed on a Device.

        - -DUDF\_TARGET\_LIB: name of the target .so file after compilation. For example, if it is set to add, libadd.so is generated.

    - In the cross-compilation scenario (the development environment is x86\_64 and the operating environment is aarch64), to compile an .so file of the Ascend type, specify the target lib as libadd.so and the compilation tool as \$\{INSTALL\_DIR\}/toolkit/toolchain/hcc/bin/aarch64-target-linux-gnu-g++. Replace $\{INSTALL\_DIR\} with the path where the CANN software is stored after installation. For example, when installed by the root user, the default storage path after installation is /usr/local/Ascend/cann.

        An example is shown as follows:

        ```shell
        cmake .. -DTOOLCHAIN=${INSTALL_DIR}/toolkit/toolchain/hcc/bin/aarch64-target-linux-gnu-g++ -DRELEASE_DIR=../release -DRESOURCE_TYPE=Ascend -DUDF_TARGET_LIB=add && make
        ```

### Building a Graph Using the DataFlow API

1. Build a FlowGraph.

    Build a graph using FlowNode and FunctionPp. An example is shown as follows:

    ```cpp
    // Construct the FlowGraph of DataFlow
    dflow::FlowGraph flow_graph("flow_graph");

    // Construct the input node
    auto data0 = dflow::FlowData("Data0", 0);
    auto data1 = dflow::FlowData("Data1", 1);

    auto node0 = dflow::FlowNode("node0", 2, 1).SetInput(0, data0).SetInput(1, data1);
    // function_pp
    auto pp0 = dflow::FunctionPp("func_pp0")
                   .SetCompileConfig("./add_func.json")
                   .SetInitParam("out_type", ge::DT_UINT32);
    node0.AddPp(&pp0);
    ```

2. Run the graph.

    There are two methods as follows.

    - Session run method. An example is shown as follows:

        ```cpp
        // Create a session
        ge::Session* session1 = new Session(options);
        assert(session1 != NULL);
        ret = session1->AddGraph(graphId, graph);
        if (!ret) { return; }
        // Run the graph
        ret = session1->RunGraph(graphId, input, output);
        if (!ret) { return; }
        GEFinalize();
        return;
        ```

    - DataFlow mode, example as follows:

        ```cpp
        // Execute the graph through the DataFlow asynchronous API:
        ge::DataFlowInfo dataFlowInfo;
        dataFlowInfo.SetStartTime(0);
        dataFlowInfo.SetEndTime(5);
        std::vector<ge::Tensor> inputsData = {inputTensor, inputTensor};
        geRet = session->FeedDataFlowGraph(0, inputsData, dataFlowInfo, 3000);
        if (geRet != ge::SUCCESS)
        {
         std::cout << "TEST====Feed input data failed============." << std::endl;
         ge::GEFinalize();
         return -1;
        }
        // Obtain the output
        std::vector<ge::Tensor> outputsData;
        geRet = session->FetchDataFlowGraph(0, outputsData, dataFlowInfo, 3000);
        if (geRet != ge::SUCCESS)
        {
         std::cout << "TEST====Fetch output data failed." << std::endl;
         ge::GEFinalize();
         return -1;
        }
        ```

### Testing

Use the developed UDF to construct a single-UDF FlowGraph, compile it, and run it. For details, see [Constructing a FlowGraph Using FlowOperator](constructing_flowgraph_using_flowoperator.md) and [Building and Running FlowGraph](compiling_and_executing_flowgraph.md#building-and-running-flowgraph).

## Graph Partitioning Based on Flow Tags

**Feature Overview**

The frontend expression supports only IR expression. Between homogeneous engines, if Flow expression is desired, you can set the flow attribute of an Op or SubGraphOp through "**\_flow\_attr**" to split them into different Flow graphs, which are then compiled, deployed, and executed separately. For subgraphs, nesting can be implemented through the PartitionedCall operator.

"**\_flow\_attr**" supports two setting methods:

- Use the SetAttr API to set the flow attribute on an Op, indicating that all inputs and outputs of the Op are flow, and all Ops connected to it will be split into different FlowGraphs.
- Use the SetInputAttr/SetOutputAttr APIs to set the flow attribute on the input or output of an Op, indicating that the specified input/output of the Op is flow, and the Ops connected to it will be split into different FlowGraphs.

In queue-driven scenarios, queues are inserted between asynchronous graphs. The configuration attributes of a queue are as follows:

- "\_flow\_attr": A BOOL attribute that can be set to true or false. true indicates that the flow attribute exists, and false indicates that it does not.
- "\_flow\_attr\_depth": An INT attribute that specifies the queue depth. The value range is greater than or equal to 2. If not configured, the default depth is 128.
- "\_flow\_attr\_enqueue\_policy": A STRING attribute that specifies the enqueue policy. Only "FIFO" and "OVERWRITE" are supported. "FIFO" indicates that queue data is enqueued in order, and when the queue is full, it waits for dequeue. "OVERWRITE" indicates that enqueue does not wait and data is overwritten cyclically. If not configured, the default policy is FIFO.
- FlowGraph splitting is not supported for the V1 control operator structure of while/loop.

**Usage**

IR graph construction example:

```cpp
    auto data1 = op::Data("Data1").set_attr_index(0);
    auto data2 = op::Data("Data2").set_attr_index(1);
    auto data3 = op::Data("Data3").set_attr_index(2);
    auto data4 = op::Data("Data4").set_attr_index(3);

    float inputData1[2] = {1,1};
    Tensor input1(TensorDesc(ge::Shape({2}), FORMAT_ND, DT_FLOAT), (uint8_t *)inputData1, 2 * sizeof(float));
    input.push_back(input1);

    float inputData2[2] = {1,1};
    Tensor input2(TensorDesc(ge::Shape({2}), FORMAT_ND, DT_FLOAT), (uint8_t *)inputData2, 2 * sizeof(float));
    input.push_back(input2);

    float inputData3[2] = {1,1};
    Tensor input3(TensorDesc(ge::Shape({2}), FORMAT_ND, DT_FLOAT), (uint8_t *)inputData3, 2 * sizeof(float));
    input.push_back(input3);

    float inputData4[2] = {1,1};
    Tensor input4(TensorDesc(ge::Shape({2}), FORMAT_ND, DT_FLOAT), (uint8_t *)inputData4, 2 * sizeof(float));
    input.push_back(input4);

    auto OP_Reciprocal = op::Reciprocal("Reciprocal").set_input_x(data1);
    // Set the flow attribute, op

    const std::string ATTR_NAME_FLOW_ATTR = "_flow_attr";
    const std::string ATTR_NAME_FLOW_ATTR_DEPTH= "_flow_attr_depth";
    const std::string ATTR_NAME_FLOW_ATTR_ENQUEUE_POLICY = "_flow_attr_enqueue_policy";

    OP_Reciprocal.SetAttr(ATTR_NAME_FLOW_ATTR.c_str(), true);
    AscendString policy("FIFO");
    OP_Reciprocal.SetAttr(ATTR_NAME_FLOW_ATTR_ENQUEUE_POLICY.c_str(), policy);

    auto OP_Square = op::Square("Square").set_input_x(data2);
    // Set the flow attribute, op
    OP_Square.SetAttr(ATTR_NAME_FLOW_ATTR.c_str(), true);
    OP_Square.SetAttr(ATTR_NAME_FLOW_ATTR_DEPTH.c_str(), static_cast<int32_t>(16));
    AscendString policy1("");
    OP_Square.SetAttr(ATTR_NAME_FLOW_ATTR_ENQUEUE_POLICY.c_str(), policy1);

    auto OP_Add = op::Add("Add1").set_input_x1(OP_Reciprocal).set_input_x2(OP_Square);
    // Set the flow attribute, op
    OP_Add.SetAttr(ATTR_NAME_FLOW_ATTR.c_str(), true);
    OP_Add.SetAttr(ATTR_NAME_FLOW_ATTR_DEPTH.c_str(), static_cast<int32_t>(16));
    AscendString policy2("OVERWRITE");
    OP_Add.SetAttr(ATTR_NAME_FLOW_ATTR_ENQUEUE_POLICY.c_str(), policy2);

    auto OP_Sub = op::Sub("Sub").set_input_x1(data3).set_input_x2(data4);

    auto OP_Mul = op::Mul("Mul").set_input_x1(OP_Add).set_input_x2(OP_Sub);
    auto partition1 = op::PartitionedCall("partition_001")
 .create_dynamic_input_args(1)
 .set_dynamic_input_args(0, OP_Mul)
 .create_dynamic_output_output(1)
 .set_subgraph_builder_f(build_graph1);
    // Set the flow attribute, PartitionedCall
    partition1.SetAttr(ATTR_NAME_FLOW_ATTR.c_str(), true);
    partition1.SetAttr(ATTR_NAME_FLOW_ATTR_DEPTH.c_str(), static_cast<int32_t>(16));
    AscendString policy3("FIFO");
    partition1.SetAttr(ATTR_NAME_FLOW_ATTR_ENQUEUE_POLICY.c_str(), policy3);

    auto result_output = op::Add("Add2").set_input_x1(partition1).set_input_x2(partition1);
    // Set the flow attribute, input+output
    result_output.SetInputAttr(0,ATTR_NAME_FLOW_ATTR.c_str(), true);
    result_output.SetInputAttr(0,ATTR_NAME_FLOW_ATTR_DEPTH.c_str(), static_cast<int32_t>(32));
    AscendString policy4("OVERWRITE");
    result_output.SetInputAttr(0,ATTR_NAME_FLOW_ATTR_ENQUEUE_POLICY.c_str(), policy4);
    result_output.SetOutputAttr(0,ATTR_NAME_FLOW_ATTR.c_str(), true);
    result_output.SetOutputAttr(0,ATTR_NAME_FLOW_ATTR_DEPTH.c_str(), static_cast<int32_t>(32));
    AscendString policy5("OVERWRITE");
    result_output.SetOutputAttr(0,ATTR_NAME_FLOW_ATTR_ENQUEUE_POLICY.c_str(), policy5);

    std::vector<Operator> inputs{data1,data2,data3,data4};

    std::vector<Operator> outputs{result_output};
    graph.SetInputs(inputs).SetOutputs(outputs);
    ge::Session* session = new Session(options);
    session->AddGraph(graphId, graph);
    session->RunGraph(graphId, input, output);
    session->RemoveGraph(graphId);
    GEFinalize();
```

where build\_graph is defined as:

```cpp
Graph build_graph1(){
Graph graph("build_graph1");
std::vector<Tensor> input;
std::vector<Tensor> output;
auto data1 = op::Data("Data1").set_attr_index(0);
float inputData1[2] = {1,1};
Tensor input1(TensorDesc(ge::Shape({2}), FORMAT_ND, DT_INT32), (uint8_t *)inputData1, 2 * sizeof(float));
input.push_back(input1);
auto output_result=op::Cast("Cast_01").set_input_x(data1).set_attr_dst_type(DT_FLOAT);
std::vector<Operator> inputs{data1};
std::vector<Operator> outputs{output_result};
graph.SetInputs(inputs).SetOutputs(outputs);
return graph;
}
```

DataFlow graph construction example:

```cpp
// DataFlow graph construction
auto node0 = dflow::FlowNode("node0", 2, 1)         // Create a FlowOperator instance of FlowNode
  .SetInput(0, data0)                     // Set the first input of FlowNode to data0
  .SetInput(1, data1);                    // Set the second input of FlowNode to data1
// Set Flow properties
node0.SetAttr("_flow_attr", true);
node0.SetAttr("_flow_attr_depth", static_cast<int32_t>(108));
AscendString policy1("FIFO");
node0.SetAttr("_flow_attr_enqueue_policy", policy1);
```

## Specifying the Deployment Location of the DataFlow Node

**Feature Overview**

A configuration file can be used to specify the deployment location of DataFlow graph nodes. This feature enables multi-card multi-instance deployment, distributes the computational load, and improves performance.

In the scenario where a UDF calls an NN, the system implements the following deployment policies.

- When the UDF is deployed on the Host, the NN is deployed on the first Device.
- When the UDF is deployed on a Device, the NN is also deployed on the Device.

**Usage constraints**

- When specifying deployment, the deployment location of all nodes must be specified.
- Specify the deployment configuration file through ge.experiment.data\_flow\_deploy\_info\_path in the options parameter of AddGraph. The file must exist and be in the correct format. For details about the format requirements, see "Configuration File Format".
- The deployment location must be consistent with the location where the node can actually be deployed. For example, if a UDF supports only X86, it cannot be specified to be deployed on a Device. Similarly, if a node can be deployed only on a Device, it cannot be specified to be deployed on the Host.
- The same node cannot be configured repeatedly.
- The same node cannot be deployed on both Host and Device simultaneously.

**Usage**

The following shows usage examples.

```cpp
std::map<ge::AscendString, ge::AscendString> session_options = {};
std::shared_ptr<ge::Session> session = std::make_shared<ge::Session>(session_options);
const auto graph = CreateDataFlowGraph();
std::map<ge::AscendString, ge::AscendString> graph_options = {{"ge.experiment.data_flow_deploy_info_path", "./data_flow_deploy_info.json"}};
auto = session->AddGraph(0, graph, graph_options);
...
```

**Configuration File Format**

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

**Table 1**  Configuration items

|Configuration Item|Description|
|--|--|
|keep_logic_device_order|Whether to deploy instances in the order of the device_list configured by the user.<br>Values:<br>true: Yes. In a multi-instance scenario, Devices are sorted and deployed according to the device_list configured by the user.<br>false: No. In a multi-instance scenario, Devices are sorted and deployed according to the internal implementation logic of the framework.<br>Default value: false.|
|**batch_deploy_info**|-|
|flow_node_list|List of FlowNode node names. One or more names are supported. Separate multiple names with half-width commas (,).|
|logic_device_list|Deployment location of DataFlow graph nodes.<br>**Format**: clusterid:serverid:deviceid:numaid (pgid/dieid). The fields are described as follows:<br>- clusterid: ID of the cluster. Currently fixed to 0.<br>- serverid: ID of the server node. Set it to node_id in numa_config.json.<br>- deviceid: Logical number of the device. It corresponds to the item order of the "node>item_list" field in the numa_config file specified by the environment variable RESOURCE_CONFIG_PATH, starting from 0.<br>- numaid (pgid/dieid): Number of the compute unit in a single device with multiple compute units.<br>**Typical scenarios are as follows**.<br>When heavy_load of FunctionPp is set to true, it is deployed to the Host node corresponding to the specified node. When deploying to multiple Devices, configure as follows:<br>- Configure one by one: For example, "0:0:0:0,0:0:0:1" indicates that the node is deployed to the two devices "0:0:0:0" and "0:0:0:1" in multiple instances.<br>- Configure by device range: For example, "0:0:0~1:0~1", where "~" indicates a range, and the value before "~" must be less than or equal to the value after it. "0:0:0~1:0~1" indicates that the node is deployed in multiple instances to the two compute units of the two devices "0:0:0:0", "0:0:0:1", "0:0:1:0", and "0:0:1:1".<br>When the FlowNode is a parent node that invokes a sub-FlowGraph, multiple instances cannot be configured.|
|invoke_list (optional; if not configured, the deployment node of the subgraph is the same as that of the parent node)|-|
|invoke_name|Name of the InvokedClosure nested in the FlowNode node, corresponding to the name parameter of the AddInvokedClosure API.|
|deploy_info_file|Specifies the deployment location of the DataFlow subgraph node through this configuration item. When the InvokedClosure corresponding to invoke_name is a DataFlow subgraph, you can configure the deployment location file corresponding to the DataFlow subgraph, for example, ./data_flow_invoke_flow_graph_deploy_info.json. For the fields and format requirements contained in this file, see data_flow_deploy_info.json.|
|logic_device_list|Specifies the deployment location of the DataFlow subgraph or AscendGraph subgraph node through this configuration item. The configuration method is the same as that of logic_device_list in batch_deploy_info.<br>For the same subgraph node, logic_device_list and deploy_info_file cannot be configured at the same time. Otherwise, an error is reported.<br>For an AscendGraph subgraph node, the number of instances of logic_device_list must be the same as that of the parent node's logic_device_list. Otherwise, an error is reported.|

## DataFlow Subgraph Compilation Cache

**Feature Overview**

When the DataFlow compilation cache feature is enabled, modifying the subgraph of a single DataFlow node triggers incremental compilation of only the modified subgraph, while the remaining subgraphs use the cache, thereby reducing compilation time.

**Usage constraints**

- This feature supports FlowGraph but not AscendGraph.
- Models containing resource-type operators are not supported.
- The cache does not guarantee cross-version compatibility. If the version is upgraded, clear the cache directory and recompile to regenerate the cache.
- After a DataFlow subgraph changes, the original cache files become invalid. You need to manually delete the cache files in the cache directory and recompile to regenerate the cache files.

**Usage**

1. Enable the cache configuration.

    The configuration example is as follows.

    ```cpp
    std::map<ge::AscendString, ge::AscendString> session_options = {{"ge.graph_compiler_cache_dir", "./build_cache_dir"}};
    std::shared_ptr<ge::Session> session = std::make_shared<ge::Session>(session_options);
    const auto graph = CreateFlowGraph();
    std::map<ge::AscendString, ge::AscendString> graph_options = {{"ge.graph_key", "test_graph_001"}};
    auto = session->AddGraph(0, graph, graph_options);
    ...

    ```

     **Table 1** Parameters

     |Parameter|Description|
     |--|--|
     |ge.graph_compiler_cache_dir|Graph compilation disk cache directory. It is used together with ge.graph_key. The graph compilation disk cache takes effect only when both ge.graph_compiler_cache_dir and ge.graph_key are configured with non-empty values.<br>The configured cache directory must exist; otherwise, compilation fails.<br>After the graph changes, the original cache files become invalid. You need to manually delete the cache files in the cache directory, including the model cache file, index file, and variable format file, and then recompile to generate new cache files.|
     |ge.graph_key|Unique identifier of the graph. It is recommended that the value contain only uppercase and lowercase letters (A-Z, a-z), digits (0-9), underscores (_), and hyphens (-), and be no longer than 128 characters. Otherwise, the system reports an error.|

    Note: For ge.graph_compiler_cache_dir, if you need to reduce the cache recovery time, you can add a configuration file "cache.conf" in this directory. The example is as follows.

    ```sh
    {
     "cache_manual_check":true,
     "cache_debug_mode":true
    }
    ```

    - cache_manual_check: whether to enable the manual cache check mode. When set to true, if the graph changes and is recompiled, you need to manually delete the corresponding subgraph cache files in the DataFlow subgraph cache folder.

    - cache_debug_mode: whether to enable the cache debug mode. When set to true, the files for the whole-graph cache are not generated (including the model cache file, index file, and variable format file. For details, see "Cache File Generation Rules").

    After compilation is complete, cache files are generated in the specified directory. For details, see "Cache File Generation Rules".

2. If the graph changes and recompilation is required, perform the following steps.

   **Note:**
   <br>Graph changes include: modifying the graph or config.json corresponding to graphpp, modifying the UDF implementation file, and modifying the deployment information.
   <br>If ge.graph\_compiler\_cache\_dir "cache.conf" is configured and cache\_debug\_mode=true, you do not need to manually delete the graph cache files.

   a. (Optional) Manually delete the whole-graph cache files (including the model cache file, index file, and variable format file. For details, see "Cache File Generation Rules".

   b. Recompile.

      After compilation is complete, cache files are generated in the specified directory. For details, see "Cache File Generation Rules".

**Cache File Generation Rules**

The generated files include:

- Model cache file.
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

- Variable format file, which is generated only when variables exist in the graph. It is used by the framework to match the model cache file. If the variable format in the graph corresponding to graph\_key changes, the previously cached cache files cannot be directly restored and used. In this case, the compilation process is triggered again to regenerate the cache files.
- If the external weight feature is enabled, that is, the ge.externalWeight parameter is configured in the options and set to 1, a weight directory is also generated in the path specified by **ge.graph\_compiler\_cache\_dir** to store the weight information of the Const/Constant nodes in the original network.
- DataFlow subgraph cache folder, generated only in the FlowGraph scenario. It is named graph\_key and stores the cache files generated during DataFlow subgraph compilation.

File naming rules:

- When the ge.graph\_key configuration value contains only uppercase and lowercase letters (A-Z, a-z), digits (0-9), underscores (\_), and hyphens (-), and its length does not exceed 128:
  - The index file is named ge.graph\_key+“.idx”.
  - The model cache file is named ge.graph\_key+ current time + ".om".
  - The variable format file is named ge.graph\_key+ current time + ".rdcpkt".

- If the preceding conditions are not met, the system reports an error.

## DataFlow Offline Compilation

**Feature Overview**

DataFlow offline compilation refers to compiling in the development environment and then loading and deploying in the operating environment, thereby decoupling compilation from graph linking.

**Usage**

1. Enable graph compilation caching. The following is an example.

    ```cpp
    std::map<ge::AscendString, ge::AscendString> session_options = {{"ge.graph_compiler_cache_dir", "./build_cache_dir"}};
    std::shared_ptr<ge::Session> session = std::make_shared<ge::Session>(session_options);
    const auto graph = CreateFlowGraph();
    std::map<ge::AscendString, ge::AscendString> graph_options = {{"ge.graph_key", "test_graph_001"}};
    auto = session->AddGraph(0, graph, graph_options);
    ...
    ```

2. Configure the development environment.

    Configure the numa\_config.json file of the target execution environment through the environment variable RESOURCE\_CONFIG\_PATH. The following is an example:

    ```shell
     export RESOURCE_CONFIG_PATH=numa_config.json//Set the storage path of the heterogeneous resource description information file.
    ```

    > [!NOTE]
    >**numa\_config.json** For details, see [Appendix](appendices.md). The ipaddr field can use any IP address.

   **Table 1** Parameters

   |Parameter|Description|
   |--|--|
   |resourceType|Optional. Resource type, which can be set to Ascend. It must be consistent with the resourceType in the operating environment where the model is actually deployed.|
   |chip_count|Mandatory. Number of accelerator cards. It must be consistent with the number of accelerator cards in the operating environment where the model is actually deployed. If they are inconsistent, the system performs the following processing.<br>- If chip_count is greater than the number of accelerator cards in the operating environment, an error is reported during deployment.<br>- If chip_count is less than the number of accelerator cards in the operating environment and no deployment location is specified for load balancing, the model is deployed according to the load balancing scheme at the compilation stage, and the extra devices in the operating environment are not deployed. If this parameter is not configured, the offline function does not take effect.|

3. Graph generation.
    1. When calling the AddGraph API, set the option "ge.runFlag"="0".
    2. Call the BuildGraph API to compile the graph and generate cache files.

        An example is shown as follows:

        ```cpp
        map<AscendString, AscendString> options = {{"ge.graph_compiler_cache_dir", "./build_cache_dir"}, {"ge.runFlag", "0"}};
        Session session(options);
        map<AscendString, AscendString> graph_options = {{"ge.graph_key", "graph_key1"}}
        session.AddGraph(1, g1, graph_options);
        std::vector<InputTensorInfo> inputs;
        auto ret = session.BuildGraph(1, inputs);
        // The build API can be replaced with the FeedDataFlowGraph API.
        auto ret = session.FeedDataFlowGraph(1, inputs, dataInfo);
        ```

    3. Offline deployment.

        Copy the model cache files, index files, and variable format files from the graph\_compiler\_cache\_dir path in the development environment to the graph\_compiler\_cache\_dir path in the operating environment. For details about the files, see "Cache File Generation Rules". The constraints are as follows:

        The numa\_config of the execution environment must be consistent with that at compilation time, except for the ipaddr field.
