# Example Code for Executing Fixed-Shape Operators

<!-- md-trans-meta sourceCommit=52549352caa2479eb7a7a57b727e9512eacd6df3 translatedAt=2026-11-17T15:29:56.048Z pushedAt=2026-11-18T06:39:42.377Z nl=2 -->

This section describes the key APIs and example code for invoking fixed-shape operators based on single-operator model execution.

## Prerequisites

Before calling the ACL API to execute a fixed-shape operator, compile the operator in advance. The following uses the ATC tool to compile the model file of the Add operator as an example:

1. First, create the description file of the operator (\*.json file, which describes the input/output tensor description, operator attributes, and so on).

    The following is an example of the description file of the Add operator:

    ```bash
    [
      {
        "op": "Add",
        "input_desc": [
          {
            "format": "ND",
            "shape": [8, 16],
            "type": "int32"
          },
          {
            "format": "ND",
            "shape": [8, 16],
            "type": "int32"
          }
        ],
        "output_desc": [
          {
            "format": "ND",
            "shape": [8, 16],
            "type": "int32"
          }
        ]
      }
    ]
    ```

2. Use the ATC tool to compile the operator description file into a single-operator model file (\*.om file), and then call the ACL API to load the om model file and execute the operator.

    The following is an example of the ATC tool command:

    ```c++
    atc --singleop=$HOME/singleop/add.json --output=$HOME/singleop/out/op_model --soc_version=<soc_version>
    ```

    The key parameters are described as follows. For details about parameter values and constraints, see *[ATC Offline Model Compilation Tool](../../../../user_guides/atc_tools/README.md)*:

    - --singleop: Path of the single-operator description file (in JSON format).
    - --output: Directory for storing the single-operator model file.
    - --soc\_version: Version of the AI processor. _<soc\_version\>_ Replace it based on the actual situation.

## Example Code

The following code example shows the key steps for loading and executing a single operator. It is for reference only and cannot be directly copied, compiled, and run. After calling the APIs, add branches for exception handling and record error logs and prompt logs, which are not listed here one by one.

You can click [aclop\_invocation](https://gitcode.com/cann/asc-devkit/blob/master/examples/01_simd_cpp_api/02_features/99_acl_based/01_acl_invocation/aclop_invocation/README_en.md) to obtain the sample.

```c++
// 1. Initialize.
aclRet = aclInit(nullptr);

// 2. Apply for runtime resources (use the default Context and default Stream; when the default Stream is passed to other APIs, a null pointer can be passed).
aclRet = aclrtSetDevice(0);
// Obtain the running mode of the software stack. Different running modes affect the subsequent API call flow (for example, whether data transfer is performed).
aclrtRunMode runMode;
bool g_isDevice = false;
aclError aclRet = aclrtGetRunMode(&runMode);
g_isDevice = (runMode == ACL_DEVICE);

// 3. Load the single operator model file (*.om file).
// This directory is relative to the directory where the executable file is located. For example, if the compiled executable file is stored in the out directory, this represents the out/op_models directory.
aclRet = aclopSetModelDir("op_models");

// 4. Execute the operator.
// opType represents the operator type name, for example, Add.
// numInputs represents the number of operator inputs, for example, the Add operator has two inputs.
// inputDesc represents the array of operator input tensor descriptions, describing the format, shape, and data type of each input.
// inputs represents the operator input tensor data.
// numOutputs represents the number of operator outputs, for example, the Add operator has one output.
// outputDesc represents the array of operator output tensor descriptions, describing the format, shape, and data type of each output.
// outputs represents the operator output tensor data.
// attr represents the operator attributes. If the operator has no attributes, the aclopCreateAttr API must still be called to create data of the aclopAttr type.
// stream is used to maintain the execution order of some asynchronous operations.

aclopExecuteV2(opType, numInputs, inputDesc, inputs,
               numOutputs, outputDesc, outputs, attr, nullptr);

// Block the application from running until all tasks in the specified Stream are complete.
aclrtSynchronizeStream(nullptr);

// Process the output data after the operator is executed, for example, display it on the screen or write it to a file. This is implemented by the user according to actual conditions.
// ......

// 5. Release runtime resources (the default Context and Stream do not need to be released by the user; they are automatically released after the aclrtResetDevice API is called).
aclRet = aclrtResetDevice(0);

// 6. Deinitialize.
aclRet = aclFinalize();

// ....
```
