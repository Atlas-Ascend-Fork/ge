# Special Topics

<!-- md-trans-meta sourceCommit=0e5c73f3b4489260ae9f6461e1178f02c751dbca translatedAt=2026-11-16T08:54:38.523Z pushedAt=2026-11-18T06:33:00.949Z nl=2 -->

## UDF Development

### Development Process

The process of UDF development is shown in the following figure.

**Figure 1** UDF development process
![](figures/udf_develop_process.png "UDF development process")

The development steps are described in Table 1.

**Table 1** UDF development steps

|Step|Description|References|
|--|--|--|
|Environment Preparation|Prepare the development environment and operating environment on which UDF development and running verification depend.|Preparations|
|Project creation|Create a UDF project.|Creating a Project|
|Development process|The user inherits the MetaFlowFunc base class in the meta_flow_func.h file and overrides the Init and Proc functions to implement the UDF calculation logic.|Development Process (Calling the MetaFlowFunc Class)|
|Project compilation|Compile the user-defined function cpp file into an so file.|Project Compilation|
|Build a graph using DataFlow APIs|Use DataFlow APIs to build and execute a graph.|Building a Graph Using DataFlow APIs|
|ST test|System Test (ST) verifies the correctness of the UDF in a real hardware environment.|Testing|

### Preparations

#### Environment Preparation

- Before UDF development, install the driver firmware and the development suite package Ascend-cann-toolkit. For details, see [CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum).
- Configure the environment variable for the CANN software package installation path ASCEND\_INSTALL\_PATH. During UDF compilation, the dependent header files and so files are located according to this environment variable. If it is not set, the default value of this environment variable is "/usr/local/Ascend". The configuration example is as follows.

    ```shell
    export ASCEND_INSTALL_PATH=CANN package installation path
    ```

#### Creating a Project

UDF projects are classified into the following types. Create a project according to the usage scenario.

- Implement user-defined functionality through UDF: In this scenario, the user does not call third-party dependency libraries or implemented NN functionality, and implements custom functionality entirely by writing UDF-related files.
- Implement custom functionality by calling NN: In this scenario, the functionality required by the user can be carried by existing NN functionality, and the user calls the NN functionality through UDF.
- Implement custom functionality by calling third-party dependency libraries: In this scenario, the functionality required by the user already exists in third-party dependency libraries, and the user calls the relevant functionality in the third-party dependency libraries through UDF.

You can directly download the [project example](https://gitcode.com/cann/ge/tree/master/examples/dflow/udf_workspace) and develop based on it. The following UDF project directory example includes three scenarios. Download the corresponding example according to the usage scenario, and develop the UDF deliverables in the corresponding directory according to the following rules.

```cpp
├── 01_udf_add
│ ├── CMakeLists.txt
│ └── add_flow_func.cpp // add function defined by a single-func API
├── 02_udf_call_add_nn
│ ├── CMakeLists.txt
│ └── call_nn_flow_func.cpp // UDF calls a TensorFlow model
├── 03_udf_add_multi_func
│ ├── CMakeLists.txt
│ └── add_flow_func.cpp // add function defined by a multi-func API, can be used with the 04 example
├── 04_control_func
│ ├── CMakeLists.txt
│ └── control_func.cpp // activate one of the multi-funcs defined in 03 according to the input, can be used with the 03 example
└── README.md
```

### Development Process (Calling the MetaFlowFunc Class)

**Introduction**

During UDF development, users can call the MetaFlowFunc class to write custom single-func processing functions. They can also call the MetaMultiFunc class to write custom multi-func processing functions. This section introduces the development process of UDF by calling the MetaFlowFunc class.

The implementation of UDF includes two parts:

- UDF implementation file: the code implementation of user-defined function functionality, with the file extension .cpp. According to the scenario, it is divided into implementing user-defined functionality through UDF and implementing custom functionality by calling NN.
- UDF registration: the implementation class is declared as func name through the REGISTER\_FLOW\_FUNC macro and registered to the UDF framework.

> [!NOTE]Note
>
>- In the single-P scenario, one input corresponding to multiple outputs, one input corresponding to one output, or multiple inputs corresponding to one output is supported.
>- In the 2P scenario, only one input corresponding to one output is supported.
>- The following uses the function that implements the add functionality as an example to introduce how to perform UDF development, compilation, graph construction, and verification.

**UDF Implementation File (Implementing User-Defined Functionality Through UDF)**

Users need to develop user-defined functions in the "workspace/xx.cpp" file of the project. The following uses "add\_flow\_func.cpp" as an example for introduction.

**Function analysis:**

This function implements the Add functionality and supports the Add functionality of the float type and the int type.

**Clarify input and output:**

It contains two inputs and one output.

**Function implementation:**

The user inherits the MetaFlowFunc base class of the meta\_flow\_func.h file and overrides the Init and Proc functions.

If there are resources to be released, they need to be handled in the destructor.

```cpp
namespace FlowFunc {
class AddFlowFunc: public MetaFlowFunc{};
}
```

> [!NOTE]Note
>If users need to use the TPRT (Task Parallel Runtime) concurrency functionality, for the implementation in the following Init\(\) and Proc\(\) functions, refer to [Development Process (Calling the MetaMultiFunc Class)](#development-process-calling-the-metamultifunc-class).

- Init\(\): performs initialization actions, such as variable initialization and obtaining attributes. In this example, it needs to obtain the out\_type attribute set in FunctionPp during DataFlow graph construction. The example is as follows.

    ```cpp
    int32_t Init() override
        {
            // Obtain the out_type attribute set in operator graph construction through the GetAttr method of the MetaContext class. The attribute values are stored in the private variable outDataType_ of the class and are used to convert the source type to the outDataType_ type.
            auto getRet = context_->GetAttr("out_type", outDataType_);
            if (getRet != FLOW_FUNC_SUCCESS) {
                FLOW_FUNC_RUN_LOG_ERROR("GetAttr dType not exist. ");
                return getRet;
            }
            FLOW_FUNC_RUN_LOG_INFO("Add flow func init success");
            return FLOW_FUNC_SUCCESS;
        }
    ```

- Proc\(\): user-defined calculation processing function. When the UDF framework receives the input Tensor data, it will call this method.

    ```cpp
    int32_t Proc(const std::vector<std::shared_ptr<FlowMsg>> &inputMsgs) override
    {
        FLOW_FUNC_LOG_DEBUG("proc start");
        // In the Add method, there are only two inputs
        if (inputMsgs.size() != 2) {
            // Return the error code defined in flow_func_defines.h
            FLOW_FUNC_LOG_ERROR("add must have 2 inputs, but %zu", inputMsgs.size());
            return FLOW_FUNC_ERR_PARAM_INVALID;
        }

        // invalid input
        auto inputFlowMsg1 = inputFlowMsgs[0];
        auto inputFlowMsg2 = inputFlowMsgs[1];
        FLOW_FUNC_LOG_INFO("inputFlowMsg1.RetCode=%d, inputFlowMsg2.RetCode=%d", inputFlowMsg1->GetRetCode(), inputFlowMsg2->GetRetCode());
        // Determine whether the RetCode in the input msg is 0. RetCode is the processing result of the previous step. If it is 0, it indicates normal; if it is not 0, an error is reported
        if (inputFlowMsg1->GetRetCode() != 0 || inputFlowMsg2->GetRetCode() != 0) {
            FLOW_FUNC_LOG_ERROR("invalid input");
            return -1;
        }
        MsgType MsgType1 = inputFlowMsg1->GetMsgType();
        MsgType MsgType2 = inputFlowMsg2->GetMsgType();
        // Determine whether the message type in the input msg is the tensor type
        if (MsgType1 == MsgType::MSG_TYPE_TENSOR_DATA && MsgType2 == MsgType::MSG_TYPE_TENSOR_DATA) {
            // Obtain the input Tensor
            auto inputTensor1 = inputFlowMsg1->GetTensor();
            auto inputTensor2 = inputFlowMsg2->GetTensor();
            auto inputShape1 = inputTensor1->GetShape();
            // According to the input shape and the dataType information of the output data, apply for the output flow msg of the FlowFunc operator
            auto outputMsg = context_->AllocTensorMsg(inputShape1, outDataType_);
            if (outputMsg == nullptr) {
                FLOW_FUNC_LOG_ERROR("alloc Tensor msg failed");
                return -1;
            }
            // Call the AddTensor API in the third-party library to implement the Add functionality
            if (AddDepend::Instance().AddTensor(inputTensor1, inputTensor2, outputMsg, outDataType_) != 0) {
                FLOW_FUNC_LOG_ERROR("add tensor failed");
                return -1;
            }
            // Output the calculation result
            return context_->SetOutput(0, outputMsg);
        }
        // The input message is not of the Tensor type, return an error
        FLOW_FUNC_LOG_ERROR("MsgType is not Tensor.");
        return -1;
    }

    ```

**UDF Implementation File (Implementing Custom Functionality by Calling NN)**

Users need to develop user-defined functions in the "workspace/xx.cpp" file of the project. The following uses "call\_nn\_flow\_func.cpp" as an example for introduction.

**Function analysis:**

This function implements the Add functionality.

**Clarify input and output:**

It contains two inputs and one output.

**Function implementation:**

The user inherits the MetaFlowFunc base class of the meta\_flow\_func.h file and overrides the Init and Proc functions.

```cpp
class CallNnFlowFunc: public MetaFlowFunc{};
```

- Init\(\): performs initialization actions, such as variable initialization and obtaining attributes. In this example, Init does not need to perform processing and directly returns SUCCESS.

    ```cpp
    int32_t Init() override
    {
        FLOW_FUNC_RUN_LOG_INFO("call nn flow func init success");
        return FLOW_FUNC_SUCCESS;
    }
    ```

- Proc\(\): user-defined calculation processing function. When the UDF framework receives the input Tensor data, it will call this method.

    The example is as follows:

    ```cpp
    int32_t Proc(const std::vector<std::shared_ptr<FlowMsg>> &inputMsgs) override
    {
        std::vector<std::shared_ptr<FlowMsg>> outputMsgs;
        // Call RunFlowModel to execute the NN model called in the custom processing function. This NN model implements the Add functionality and is implemented by the user through IR graph construction. The model execution result is placed in outputMsgs.
        auto ret = context_->RunFlowModel(dependKey_.c_str(), inputMsgs, outputMsgs, 100000);
        if (ret != FLOW_FUNC_SUCCESS) {
            FLOW_FUNC_LOG_ERROR("Run flow model failed, ret=%d, dependKey=%s", ret, dependKey_.c_str());
            return ret;
        }
        // Output the result outputMsgs of calling the NN model through SetOutput.
        for (size_t i = 0; i < outputMsgs.size(); ++i) {
            ret = context_->SetOutput(i, outputMsgs[i]);
            if (ret != FLOW_FUNC_SUCCESS) {
                FLOW_FUNC_LOG_ERROR("Set output failed, ret=%d, index=%zu", ret, i);
                return ret;
            }
        }
        return FLOW_FUNC_SUCCESS;
    }
    // dependKey_ is a private data member defined by the class CallNnFlowFunc:
    private:
        std::string dependKey_{"invoke_graph"};

    ```

**UDF Registration**

The implementation class is declared as func name through the REGISTER\_FLOW\_FUNC macro and registered to the UDF framework.

```cpp
REGISTER_FLOW_FUNC("call_nn", AddFlowFunc);
```

- call\_nn: user-defined function name, which needs to be customized by the user according to the function functionality.
- AddFlowFunc: class name, which needs to keep consistent with the class name in "UDF Implementation File (Implementing User-Defined Functionality Through UDF)".

### Development Process (Calling the MetaMultiFunc Class)

**Overview**

During UDF development, the user can call the MetaFlowFunc class to write a custom single-func processing function, or call the MetaMultiFunc class to write custom multi-func processing functions. This section describes the development process of a UDF by calling the MetaMultiFunc class.

The UDF implementation consists of two parts:

- UDF implementation file: the code implementation of user-defined function functionality, with the .cpp file extension. By scenario, it is divided into implementing user-defined functionality through UDF and implementing custom functionality by calling NN.
- UDF registration: declare the implementation class as a func name through the FLOW\_FUNC\_REGISTRAR macro and register it to the UDF framework.

> [!NOTE]
>
> - In the single-P scenario, one input corresponding to multiple outputs, one input corresponding to one output, or multiple inputs corresponding to one output is supported.
> - In the 2P scenario, only one input corresponding to one output is supported.
> - The following uses the function that implements the add functionality as an example to describe how to perform UDF development, compilation, graph construction, and verification.

**UDF Implementation File (Implementing User-Defined Functionality Through UDF)**

The user needs to develop user-defined functions in the workspace/xx.cpp file of the project. The following uses add\_flow\_func.cpp as an example for introduction.

**Function analysis:**

This function implements the Add functionality and supports the Add functionality of the float type and the int type. The first processing function implements a+b, and the second function implements 2a+b.

**Clarify input and output:**

Each proc function contains two inputs and one output. The input queues of the two proc processing functions are not shared, while the output queue is shared.

**Note:**

In the multi-func scenario, multiple processing functions are scheduled concurrently. The user needs to consider multi-thread concurrency issues. Locking may affect deterministic scheduling.

**Function implementation:**

The user inherits the MetaMultiFunc base class in the meta\_multi\_func.h file and overrides the Init function and the custom multi-func processing functions.

If there are resources to be released, they need to be handled in the destructor.

```cpp
namespace FlowFunc {
class AddFlowFunc: public MetaMultiFunc{};
}
```

- Init\(\): performs initialization actions, such as variable initialization and obtaining attributes. If no initialization operation is required in FlowFunc, this function does not need to be added. In this example, the out\_type attribute set in FunctionPp during DataFlow graph construction needs to be obtained. The example is as follows.

    ```cpp
    int32_t Init(const std::shared_ptr<MetaParams> &params) override
        {
            // Obtain the out_type attribute set in operator graph construction through the GetAttr method of the MetaParams class. The attribute value is stored in the private variable outDataType_ of the class and is used to convert the source type to the outDataType_ type.
            auto getRet = params->GetAttr("out_type", outDataType_);
            if (getRet != FLOW_FUNC_SUCCESS) {
                FLOW_FUNC_RUN_LOG_ERROR("GetAttr dType not exist. ");
                return getRet;
            }
            // Initialize the shared variables of the multi-func processing functions.
            setOutputCount_ = 0;
            // To enable the TPRT concurrency feature, add the following line.
            //(void)UdfTprt::Init(params);
            return 0;
        }
    ```

- template: the template function implementing the Add method.

    ```cpp
        // Template function implementing the Add1 method
        template<typename srcT, typename dstT>
        void Add1(srcT *src1, srcT *src2, dstT *dst, size_t count)
        {
            for (size_t i = 0; i < count; ++i) {
                dst[i] = src1[i] + src2[i];
            }
            /* To enable TPRT concurrency for the computation of the three lines above, replace the three lines above with the five lines below (the following example uses ParallelFor; Submit+Wait can also be used).
            std::function<void(const int64_t index)> func = [src1, src2, dst]
               (const int64_t index) {
                dst[index] = src1[index] + src2[index];
            };
            UdfTprt::ParallelFor(0, count, func);
            */
        }
        // Template function implementing the Add2 method
        template<typename srcT, typename dstT>
        void Add2(srcT *src1, srcT *src2, dstT *dst, size_t count)
        {
            for (size_t i = 0; i < count; ++i) {
                dst[i] = 2 * src1[i] + src2[i];
            }
            /* To enable TPRT concurrency for the computation of the three lines above, replace the three lines above with the seven lines below (the following example uses Submit+Wait; ParallelFor can also be used).
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

- Multi-func processing functions: user-defined calculation processing functions. When the UDF framework receives the input Tensor data, it will call this method.

    The code example is as follows.

    ```cpp
    // User-defined multi-func processing function name: "Proc1". The function name is defined by the user. The input parameters and returns must remain consistent with the prototype.
    int32_t Proc1(const std::shared_ptr<MetaRunContext> &runContext, const std::vector<std::shared_ptr<FlowMsg>> &inputFlowMsgs)
        {
            // Validate whether the add operation has two inputs
            if (inputFlowMsgs.size() != 2) {
                FLOW_FUNC_LOG_ERROR("add must have 2 inputs, but %zu", inputMsgs.size());
                return -1;
            }
            auto inputFlowMsg1 = inputFlowMsgs[0];
            auto inputFlowMsg2 = inputFlowMsgs[1];
            // Invalid input validation: validate whether the error code of the input message is non-zero
            if (inputFlowMsg1->GetRetCode() != 0 || inputFlowMsg2->GetRetCode() != 0) {
                FLOW_FUNC_LOG_ERROR("invalid input");
                return -1;
            }
            // Obtain the message type of the input data
            MsgType MsgType1 = inputFlowMsg1->GetMsgType();
            MsgType MsgType2 = inputFlowMsg2->GetMsgType();
            // Validate whether the input data is of the Tensor type
            if (MsgType1 == MsgType::MSG_TYPE_TENSOR_DATA && MsgType2 == MsgType::MSG_TYPE_TENSOR_DATA) {
                // Obtain the Tensor from msg
                auto inputTensor1 = inputFlowMsg1->GetTensor();
                auto inputTensor2 = inputFlowMsg2->GetTensor();
                // Obtain the data type from the Tensor
                auto inputDataType1 = inputTensor1->GetDataType();
                auto inputDataType2 = inputTensor2->GetDataType();
                // Validate whether the data types of the two inputs are consistent
                if (inputDataType1 != inputDataType2) {
                    FLOW_FUNC_LOG_ERROR("input type is not the same");
                    return -1;
                }
                // Obtain the shape information from the Tensor
                auto &inputShape1 = inputTensor1->GetShape();
                auto &inputShape2 = inputTensor2->GetShape();

                // Validate whether the shapes of the two input tensors are consistent
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
                // Perform different add operations according to different data types.
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
                // Multiple processing functions are scheduled concurrently. Users need to lock shared data themselves.
                std::unique_lock<std::mutex> lock(countMutex_);
                setOutputCount_++;
                return runContext->SetOutput(0, outputMsg);
            }
            FLOW_FUNC_LOG_ERROR("MsgType is not Tensor.");
            return -1;
        }
        // User-defined multi-func processing function name: "Proc2". The function name is defined by the user. Input parameters and returns must remain consistent with the prototype.
        int32_t Proc2(const std::shared_ptr<MetaRunContext> &runContext, const std::vector<std::shared_ptr<FlowMsg>> &inputFlowMsgs)
        {
            // Validate whether the add operation has two inputs.
            if (inputFlowMsgs.size() != 2) {
                FLOW_FUNC_LOG_ERROR("add must have 2 inputs, but %zu", inputMsgs.size());
                return -1;
            }
            auto inputFlowMsg1 = inputFlowMsgs[0];
            auto inputFlowMsg2 = inputFlowMsgs[1];
            // Invalid input validation: validate whether the error code of the input message is non-zero.
            if (inputFlowMsg1->GetRetCode() != 0 || inputFlowMsg2->GetRetCode() != 0) {
                FLOW_FUNC_LOG_ERROR("invalid input");
                return -1;
            }
            // Obtain the message type of the input data.
            MsgType MsgType1 = inputFlowMsg1->GetMsgType();
            MsgType MsgType2 = inputFlowMsg2->GetMsgType();
            // Validate whether the input data is of the Tensor type.
            if (MsgType1 == MsgType::MSG_TYPE_TENSOR_DATA && MsgType2 == MsgType::MSG_TYPE_TENSOR_DATA) {
                // Obtain the Tensor from the input msg.
                auto inputTensor1 = inputFlowMsg1->GetTensor();
                auto inputTensor2 = inputFlowMsg2->GetTensor();
                // Obtain the data type from the Tensor.
                auto inputDataType1 = inputTensor1->GetDataType();
                auto inputDataType2 = inputTensor2->GetDataType();

                // Validate whether the data types of the two inputs are consistent.
                if (inputDataType1 != inputDataType2) {
                    FLOW_FUNC_LOG_ERROR("alloc Tensor msg failed");
                    return -1;
                }
                // Obtain the shape information from the tensor.
                auto &inputShape1 = inputTensor1->GetShape();
                auto &inputShape2 = inputTensor2->GetShape();

                // Validate whether the shapes of the two input tensors are consistent.
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
                // Perform specific add operations according to different data types.
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
                // Multiple processing functions are scheduled concurrently. Users need to lock shared data themselves.
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

**UDF implementation file (implement custom functionality by calling NN)**

Users need to develop user-defined functions in the workspace/xx.cpp file of the project. The following uses call\_nn\_flow\_func.cpp as an example for introduction.

**Function analysis:**

This function implements the Add functionality.

**Clarify input and output:**

It contains two inputs and one output.

**Function implementation:**

The user inherits the MetaFlowFunc base class in the meta\_flow\_func.h file and overrides the Init function and the custom processing function.

```cpp
class CallNnFlowFunc: public MetaMultiFunc{};
```

- Init\(\): performs initialization actions, such as variable initialization and obtaining attributes. In this example, Init does not need to perform any processing, so this function is not required.
- Add\(\): user-defined calculation processing function. After the UDF framework receives the input tensor data, it calls this method.

    The example is as follows:

    ```cpp
    // User-defined multi-func processing function name: "Add". The function name is defined by the user. Input parameters and returns must remain consistent with the prototype.
    int32_t Add(const std::shared_ptr<MetaRunContext> &runContext, const std::vector<std::shared_ptr<FlowMsg>> &inputFlowMsgs)
    {
        std::vector<std::shared_ptr<FlowMsg>> outputMsgs;
        // Use the RunFlowModel method of runContext to execute the NN model called in the custom processing function. The NN model implements the Add functionality and is implemented by the user through IR composition. The model execution result is placed in outputMsgs.
        auto ret = runContext->RunFlowModel("invoke_graph", inputMsgs, outputMsgs, 100000);
        if (ret != FLOW_FUNC_SUCCESS) {
            return ret;
        }
        // Output the result outputMsgs of calling the NN model through SetOutput.
        for (size_t i = 0; i < outputMsgs.size(); ++i) {
            ret = runContext->SetOutput(i, outputMsgs[i]);
            if (ret != FLOW_FUNC_SUCCESS) {
                return ret;
            }
        }
        return FLOW_FUNC_SUCCESS;
    }
    ```

**UDF registration**

Use the FLOW\_FUNC\_REGISTRAR macro to declare the implementation class as the func name and register it to the UDF framework.

```cpp
FLOW_FUNC_REGISTRAR(AddFlowFunc)
    .RegProcFunc("Proc1", &AddFlowFunc::Proc1)
    .RegProcFunc("Proc2", &AddFlowFunc::Proc2);
FLOW_FUNC_REGISTRAR(CallNnFlowFunc)
    .RegProcFunc("CallNn_Add", &CallNnFlowFunc::Add);
```

- Proc1: user-defined multi-func processing function name. It needs to remain consistent with the processing function name defined in AddFlowFunc.
- Proc2: user-defined multi-func processing function name. It needs to remain consistent with the processing function name defined in AddFlowFunc.
- Add: user-defined function name. It needs to remain consistent with the processing function name defined in CallNnFlowFunc.
- AddFlowFunc: class name. It needs to remain consistent with the class name in "UDF implementation file (implement custom functionality through UDF)".
- : class name. It needs to remain consistent with the class name in "UDF implementation file (implement custom functionality by calling NN)".

### Project Compilation

Create the build and release directories under the udf\_workspace/01\_udf\_add directory:

After compilation, the successfully compiled .so file is generated under the workspace/udf\_add/release directory.

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

4. Configure the environment variables. The following example uses the default path after a non-root user installs the Toolkit. Run the following command according to the actual path of set\_env.sh.

    ```shell
    source ${HOME}/Ascend/cann/set_env.sh
    ```

    The preceding environment variable configuration takes effect only in the current window. Users can write the preceding commands to an environment variable configuration file (for example, the .bashrc file) as needed.

5. Compile the project.
    - In a non-cross-compilation scenario (where the development environment and the operating environment have the same architecture), the example is as follows: compile the .so file of the x86\_64 type, specify the target lib as libadd.so, and use g++ as the compilation tool.

        ```shell
        cmake .. -DTOOLCHAIN=g++ -DRELEASE_DIR=../release -DRESOURCE_TYPE=X86 -DUDF_TARGET_LIB=add && make
        ```

        - -DTOOLCHAIN: name of the compilation tool, for example, g++.
        - -DRELEASE\_DIR: storage path of the target file after compilation.
        - -DRESOURCE\_TYPE: resource type to be compiled. The value can be X86, Aarch, or Ascend.
            - X86: set to X86 when the target .so file needs to be deployed on an X86-architecture Host.
            - Aarch: set to Aarch when the target .so file needs to be deployed on an Aarch Host.
            - Ascend: set to Ascend when the target .so file needs to be deployed on a Device.

        - -DUDF\_TARGET\_LIB: name of the target .so file after compilation. For example, if it is set to add, libadd.so is actually generated.

    - In a cross-compilation scenario (where the development environment is x86\_64 and the operating environment is aarch64), compile the .so file of the Ascend type, specify the target lib as libadd.so, and use \$\{INSTALL\_DIR\}/toolkit/toolchain/hcc/bin/aarch64-target-linux-gnu-g++ as the compilation tool. Replace $\{INSTALL\_DIR\} with the file storage path after CANN software installation. Taking the root user installation as an example, the default file storage path after installation is /usr/local/Ascend/cann.

        The example is as follows:

        ```shell
        cmake .. -DTOOLCHAIN=${INSTALL_DIR}/toolkit/toolchain/hcc/bin/aarch64-target-linux-gnu-g++ -DRELEASE_DIR=../release -DRESOURCE_TYPE=Ascend -DUDF_TARGET_LIB=add && make
        ```

### Building a Graph Using the DataFlow API

1. Build a FlowGraph.

    Build a graph through FlowNode and FunctionPp. The example is as follows:

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

    The following two methods are available.

    - Session run method. The example is as follows:

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

    - DataFlow method. The example is as follows:

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

Use the developed UDF to build a single-UDF FlowGraph, compile it, and run it. For details, see [Building a FlowGraph with FlowOperator](constructing_flowgraph_using_flowoperator.md) and [Building and Running FlowGraph](compiling_and_executing_flowgraph.md#building-and-running-flowgraph).

## Graph Splitting Based on Flow Tags

**Feature Overview**

The frontend expression supports only IR expression. Between homogeneous engines, to perform Flow expression, you can set the flow attribute of an Op or SubGraphOp through "**\_flow\_attr**" to split them into different Flow graphs, and then perform graph compilation, deployment, and execution separately. For a subgraph, subgraph nesting can be implemented through the PartitionedCall operator.

"**\_flow\_attr**" supports the following two setting methods:

- Use the SetAttr API to set the flow attribute for an Op, indicating that all inputs and outputs of the Op are flow, and all Ops connected to it are split into different FlowGraphs.
- Use the SetInputAttr/SetOutputAttr API to set the flow attribute for the input or output of an Op, indicating that the specified input/output of the Op is flow, and the Ops connected to it are split into different FlowGraphs.

In the queue-driven scenario, queues are inserted between asynchronous graphs. The configuration attributes of a queue are as follows:

- "\_flow\_attr":: a BOOL-type attribute that can be configured as true or false. true indicates that the flow attribute exists, and false indicates that the flow attribute does not exist.
- "\_flow\_attr\_depth":: an INT-type attribute that specifies the queue depth. The value range is greater than or equal to 2. If it is not configured, the default depth is 128.
- "\_flow\_attr\_enqueue\_policy":: a STRING-type attribute that specifies the enqueue policy. Only "FIFO" and "OVERWRITE" are supported. "FIFO" indicates that queue data is enqueued in order and the queue waits for dequeue when it is full. "OVERWRITE" indicates that enqueue does not wait and data is overwritten cyclically. If it is not configured, the default policy is FIFO.
- FlowGraph splitting is not supported for the V1 control operator structure of while/loop.

**Usage**

IR composition example:

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
    // Set the flow attribute for op

    const std::string ATTR_NAME_FLOW_ATTR = "_flow_attr";
    const std::string ATTR_NAME_FLOW_ATTR_DEPTH= "_flow_attr_depth";
    const std::string ATTR_NAME_FLOW_ATTR_ENQUEUE_POLICY = "_flow_attr_enqueue_policy";

    OP_Reciprocal.SetAttr(ATTR_NAME_FLOW_ATTR.c_str(), true);
    AscendString policy("FIFO");
    OP_Reciprocal.SetAttr(ATTR_NAME_FLOW_ATTR_ENQUEUE_POLICY.c_str(), policy);

    auto OP_Square = op::Square("Square").set_input_x(data2);
    // Set the flow attribute for op
    OP_Square.SetAttr(ATTR_NAME_FLOW_ATTR.c_str(), true);
    OP_Square.SetAttr(ATTR_NAME_FLOW_ATTR_DEPTH.c_str(), static_cast<int32_t>(16));
    AscendString policy1("");
    OP_Square.SetAttr(ATTR_NAME_FLOW_ATTR_ENQUEUE_POLICY.c_str(), policy1);

    auto OP_Add = op::Add("Add1").set_input_x1(OP_Reciprocal).set_input_x2(OP_Square);
    // Set the flow attribute for op
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
    // Set the flow attribute for PartitionedCall
    partition1.SetAttr(ATTR_NAME_FLOW_ATTR.c_str(), true);
    partition1.SetAttr(ATTR_NAME_FLOW_ATTR_DEPTH.c_str(), static_cast<int32_t>(16));
    AscendString policy3("FIFO");
    partition1.SetAttr(ATTR_NAME_FLOW_ATTR_ENQUEUE_POLICY.c_str(), policy3);

    auto result_output = op::Add("Add2").set_input_x1(partition1).set_input_x2(partition1);
    // Set the flow attribute for input+output
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

Where build\_graph is defined as:

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

DataFlow composition example:

```cpp
// DataFlow composition
auto node0 = dflow::FlowNode("node0", 2, 1)         // Create a FlowOperator instance of FlowNode
  .SetInput(0, data0)                     // Set the first input of FlowNode to data0
  .SetInput(1, data1);                    // Set the second input of FlowNode to data1
// Set the flow attribute
node0.SetAttr("_flow_attr", true);
node0.SetAttr("_flow_attr_depth", static_cast<int32_t>(108));
AscendString policy1("FIFO");
node0.SetAttr("_flow_attr_enqueue_policy", policy1);
```

## Specifying DataFlow Node Deployment Locations

**Feature Overview**

The configuration file can be used to specify the deployment locations of DataFlow graph nodes. This functionality implements multi-card multi-instance deployment, distributes the calculation workload, and improves performance.

In the scenario where a UDF calls an NN, the deployment strategy implemented by the system is as follows.

- When the UDF is deployed on the Host, the NN is deployed on the first Device.
- When the UDF is deployed on a Device, the NN is also deployed on the Device.

**Usage Constraints**

- When specifying deployment, the deployment locations of all nodes must be specified.
- Specify the deployment configuration file through ge.experiment.data\_flow\_deploy\_info\_path in the options parameter of AddGraph. The file must exist and be in the correct format. For the specific format requirements, see "Configuration File Format".
- The deployment location must be consistent with the actual deployable location of the node. For example, if a UDF supports only X86, it cannot be specified to be deployed on a Device. Similarly, if a node can be deployed only on a Device, it cannot be specified to be deployed on the Host.
- The same node cannot be configured repeatedly.
- The same node does not support being deployed on both the Host and a Device at the same time.

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

The following shows an example of the format requirements for a typical configuration file data\_flow\_deploy\_info.json.

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

|Configuration Item|Configuration Description|
|--|--|
|keep_logic_device_order|Whether to deploy instances in the order of the device_list configured by the user.<br>Values are as follows:<br>true: Yes. In the multi-instance scenario, Devices are sorted and deployed according to the device_list configured by the user.<br>false: No. In the multi-instance scenario, Devices are sorted and deployed according to the internal implementation logic of the framework.<br>Default value: false.|
|**batch_deploy_info**|-|
|flow_node_list|List of FlowNode node names. One or more names are supported. When multiple names are used, separate them with half-width commas.|
|logic_device_list|Deployment location of the DataFlow graph node.<br>**Format**: clusterid:serverid:deviceid:numaid (pgid/dieid). The meaning of each field is as follows:<br>- clusterid: ID of the cluster. Currently fixed to 0.<br>- serverid: ID of the server node. Configure it as node_id in numa_config.json.<br>- deviceid: Logical number of the device. It corresponds to the item order of the "node>item_list" field in the numa_config file specified by the environment variable RESOURCE_CONFIG_PATH, starting from 0.<br>- numaid (pgid/dieid): Number of the calculation unit in a single device.<br>**Typical scenarios are as follows**.<br>When heavy_load of FunctionPp is true, it is deployed to the Host node corresponding to the specified node. When deployed to multiple Devices, configure it as follows:<br>- Configure one by one: For example, "0:0:0:0,0:0:0:1" indicates that the node is deployed to the two devices "0:0:0:0" and "0:0:0:1" in multiple instances.<br>- Configure by device range: For example, "0:0:0~1:0~1", where "~" indicates a range, and the value before "~" must be less than or equal to the value after it. "0:0:0~1:0~1" indicates that the node is deployed to the two calculation units of the two devices "0:0:0:0", "0:0:0:1", "0:0:1:0", and "0:0:1:1" in multiple instances.<br>When a FlowNode is the parent node that calls a sub-FlowGraph, multiple instances cannot be configured.|
|invoke_list (optional. If not configured, the deployment node of the subgraph is consistent with that of the parent node)|-|
|invoke_name|Name of the InvokedClosure nested in the FlowNode node, corresponding to the name parameter of the AddInvokedClosure API.|
|deploy_info_file|Specifies the deployment location of the DataFlow subgraph node through this configuration item. When the InvokedClosure corresponding to invoke_name is a DataFlow subgraph, the deployment location file corresponding to the DataFlow subgraph can be configured, for example, ./data_flow_invoke_flow_graph_deploy_info.json. For the fields and format requirements contained in this file, see data_flow_deploy_info.json.|
|logic_device_list|Specifies the deployment location of the DataFlow subgraph or AscendGraph subgraph node through this configuration item. The configuration method is the same as logic_device_list in batch_deploy_info.<br>For the same subgraph node, logic_device_list and deploy_info_file cannot be configured at the same time. Otherwise, an error is reported.<br>For an AscendGraph subgraph node, the number of instances of logic_device_list and that of the parent node's logic_device_list must be the same. Otherwise, an error is reported.|

## DataFlow Subgraph Compilation Cache

**Feature Overview**

Enable the DataFlow compilation cache functionality. When the subgraph of a single DataFlow node is modified, only the modified subgraph is incrementally compiled, and the remaining subgraphs use the cache, thereby reducing the compilation time.

**Usage Constraints**

- This functionality supports FlowGraph but does not support AscendGraph.
- Models with resource-type operators are not supported currently.
- The cache does not guarantee cross-version compatibility. If the version is upgraded, the cache directory needs to be cleared and recompiled to generate the cache.
- After the DataFlow subgraph changes, the original cache files become unavailable. Users need to manually delete the cache files in the cache directory and recompile to generate cache files.

**Usage**

1. Enable the cache configuration.

    The following shows a configuration example.

    ```cpp
    std::map<ge::AscendString, ge::AscendString> session_options = {{"ge.graph_compiler_cache_dir", "./build_cache_dir"}};
    std::shared_ptr<ge::Session> session = std::make_shared<ge::Session>(session_options);
    const auto graph = CreateFlowGraph();
    std::map<ge::AscendString, ge::AscendString> graph_options = {{"ge.graph_key", "test_graph_001"}};
    auto = session->AddGraph(0, graph, graph_options);
    ...

    ```

     **Table 1** Parameter description

     |Parameter|Meaning|
     |--|--|
     |ge.graph_compiler_cache_dir|Disk cache directory for graph compilation. It is used together with ge.graph_key. When both ge.graph_compiler_cache_dir and ge.graph_key are configured as non-empty, the graph compilation disk cache functionality takes effect.<br>The configured cache directory must exist. Otherwise, compilation fails.<br>After the graph changes, the original cache files become unavailable. Users need to manually delete the cache files in the cache directory, including model cache files, index files, and variable format files, and recompile to generate cache files.|
     |ge.graph_key|Unique identifier of the graph. It is recommended that the value contain only uppercase and lowercase letters (A-Z, a-z), digits (0-9), underscores (_), and hyphens (-), and be no longer than 128 characters. If the condition is not met, the system reports an error.|

    Note: For ge.graph_compiler_cache_dir, if users need to reduce the cache recovery time, they can add the configuration file "cache.conf" under the directory. The following shows an example.

    ```sh
    {
     "cache_manual_check":true,
     "cache_debug_mode":true
    }
    ```

    - cache_manual_check: Whether to enable the manual cache check mode. When configured as true, it indicates that when the graph changes and is compiled again, the corresponding subgraph cache files under the DataFlow subgraph cache folder need to be deleted manually.

    - cache_debug_mode: Whether to enable the cache debug mode. When configured as true, the files of the whole-graph cache are not generated (including model cache files, index files, and variable format files. For details, see "Cache File Generation Rules").

    After compilation, cache files are generated in the specified directory. For details, see "Cache File Generation Rules".

2. After the graph changes, if recompilation is needed, refer to the following steps.

   **Note:**
   <br>Graph changes include: modifying the graph or config.json corresponding to graphpp, modifying the UDF implementation file, and modifying the deployment information.
   <br>If "cache.conf" is configured in ge.graph\_compiler\_cache\_dir and cache\_debug\_mode=true, the graph cache files do not need to be deleted manually.

   a. (Optional) Manually delete the whole-graph cache files (including model cache files, index files, and variable format files. For details, see "Cache File Generation Rules".

   b. Recompile.

      After compilation, cache files are generated in the specified directory. For details, see "Cache File Generation Rules".

**Cache File Generation Rules**

The generated files include:

- Model cache file.
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

- Variable format file, generated only when variables exist in the graph. It is used by the framework to match the model cache file. If the variable format in the graph corresponding to graph\_key changes, the previously cached cache file cannot be directly restored for use. In this scenario, the compilation process is triggered again to regenerate the cache file.
- If the external weight feature is enabled, that is, the ge.externalWeight parameter is configured in the options and set to 1, a weight directory is also generated under the path specified by the **ge.graph\_compiler\_cache\_dir** parameter to save the weight information of the Const/Constant nodes in the original network.
- DataFlow subgraph cache folder, generated only in the FlowGraph scenario. It is named graph\_key and stores the cache files generated during DataFlow subgraph compilation.

File naming rules:

- When the value configured in ge.graph\_key contains only uppercase and lowercase letters (A-Z, a-z), digits (0-9), underscores (\_), and hyphens (-), and is no longer than 128 characters
  - The index file is named: ge.graph\_key+".idx".
  - The model cache file is named: ge.graph\_key+ current time+".om".
  - The variable format file is named: ge.graph\_key+ current time+".rdcpkt".

- If the above conditions are not met, the system reports an error.

## DataFlow Offline Compilation

**Feature Overview**

DataFlow offline compilation refers to compiling in the development environment and loading and deploying in the operating environment, thereby decoupling compilation from link establishment.

**Usage**

1. Enable graph compilation cache. An example is as follows.

    ```cpp
    std::map<ge::AscendString, ge::AscendString> session_options = {{"ge.graph_compiler_cache_dir", "./build_cache_dir"}};
    std::shared_ptr<ge::Session> session = std::make_shared<ge::Session>(session_options);
    const auto graph = CreateFlowGraph();
    std::map<ge::AscendString, ge::AscendString> graph_options = {{"ge.graph_key", "test_graph_001"}};
    auto = session->AddGraph(0, graph, graph_options);
    ...
    ```

2. Configure the development environment.

    Use the environment variable RESOURCE\_CONFIG\_PATH to configure the numa\_config.json file of the target execution environment. An example is as follows:

    ```shell
     export RESOURCE_CONFIG_PATH=numa_config.json//Used to set the storage path of the heterogeneous resource description information file.
    ```

    > [!NOTE]Note
    >**numa\_config.json** For details, see [Appendix](appendices.md). The ipaddr field can use any IP address.

   **Table 1** Parameters

   |Parameter|Meaning|
   |--|--|
   |resourceType|Optional. Resource type. The value can be Ascend. It must remain consistent with the resourceType in the operating environment where the deployment is actually performed.|
   |chip_count|Mandatory. Number of accelerator cards. It must remain consistent with the number of operating environments where the deployment is actually performed. If they are inconsistent, the system performs the following processing.<br>- If chip_count is greater than the number of operating environments where the deployment is actually performed, an error is reported during the deployment phase.<br>- If chip_count is less than the number of operating environments where the deployment is actually performed and no deployment location is specified for load balancing, the model is deployed according to the load balancing scheme at the compile phase, and the extra devices in the operating environment are not deployed. If this parameter is not configured, the offline functionality does not take effect.|

3. Generate the graph.
    1. When calling the AddGraph API, set the option "ge.runFlag"="0".
    2. Call the BuildGraph API to perform graph compilation and generate cache files.

        An example is as follows.

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

        Copy the model cache file, index file, and variable format file from the graph\_compiler\_cache\_dir path in the development environment to the graph\_compiler\_cache\_dir path in the operating environment. For details about the files, see "Cache File Generation Rules". The constraints are as follows:

        The numa\_config of the execution environment must remain consistent with that at compile time, except for the ipaddr field.
