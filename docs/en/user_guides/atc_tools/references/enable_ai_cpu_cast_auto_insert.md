# Enabling the AI CPU Cast Operator Auto-Insertion Feature

<!-- md-trans-meta sourceCommit=8ed641509c09b657d721fdefadfbf6dee35dac94 translatedAt=2026-10-27T14:30:10.014Z pushedAt=2026-10-28T10:29:54.761Z nl=2 -->

## Overview

During model compilation, if the AI CPU operator does not support a certain data type and causes compilation failure, you can enable the Cast Operator Auto-Insertion Feature to quickly convert the input to a data type supported by the operator, thereby quickly connecting the network.

As shown in [Figure 1](#fig1), the input x of the MatrixInverse operator does not support the float16 data type.

**Figure 1**  Error example<a id="fig1"></a>
![](../figures/error_example.png "Error example")

In this scenario, you can enable the Cast Operator Auto-Insertion Feature. For details, see [Operation Steps](#operation-steps).

## Operation Steps

1. Enable AutoCast.

    Modify the `${INSTALL_DIR}/lib64/plugin/opskernel/config/init.conf` file and change the value of the "AutoCastMode" parameter to 1, as shown below:

    ```textproto
    ...
    AutoCastMode = 1
    ```

2. Modify the corresponding operator information library (the built-in AI CPU operator information library is stored in the “built-in/op\_impl/aicpu/aicpu\_kernel/config” directory under the opp installation directory), and insert the Cast conversion rule into the operator to be modified.

    As shown below, the input x of the MatrixInverse operator does not support float16, and the operator information library is configured as follows:

    ```json
    "MatrixInverse":{
            "input0":{
                "name":"x",
                "type":"DT_FLOAT,DT_DOUBLE,DT_COMPLEX128,DT_COMPLEX64"
            },
            "opInfo":{
                "computeCost":"100",
                "engine":"DNN_VM_AICPU",
                "flagAsync":"False",
                "flagPartial":"False",
                "formatAgnostic":"False",
                "opKernelLib":"TFKernel",
                "opsFlag":"OPS_FLAG_OPEN",
                "subTypeOfInferShape":"1"
            },
            "output0":{
                "name":"y",
                "type":"DT_FLOAT,DT_DOUBLE,DT_COMPLEX128,DT_COMPLEX64"
            }
        },
    ```

    To make it support float16, make the following modifications:

    1. Modify the input information, add the supported data type, and add the data type conversion rule.

        For example, for the MatrixInverse operator, add support for the float16 type to the input, and add a Cast rule to convert float16 to float32, indicating that a Cast operator that converts float16 to float32 will be inserted before this input.

        ```json
        "input0":{
            "name":"x",
            "type":"DT_FLOAT,DT_DOUBLE,DT_COMPLEX128,DT_COMPLEX64,DT_FLOAT16",
            "srcAutoCastType":"DT_FLOAT16",
            "dstAutoCastType":"DT_FLOAT"
        },
        ```

        - Add the “DT\_FLOAT16” data type to the supported "type". For the supported data types, see the definition of the Cast operator in the corresponding operator information library.
        - Add the "srcAutoCastType" configuration, which indicates the type of the input data.
        - Add the "dstAutoCastType" configuration, which indicates the target data type to convert to.

    2. Modify the output information to add the supported data types and add data type conversion rules.

        For example, for the MatrixInverse operator, add support for the float16 type to the output and add a Cast rule to convert float32 to float16, which means that a Cast operator that converts float32 to float16 is inserted after this output.

        ```json
        "output0":{
            "name":"y",
            "type":"DT_FLOAT,DT_DOUBLE,DT_COMPLEX128,DT_COMPLEX64,DT_FLOAT16",
            "srcAutoCastType":"DT_FLOAT",
            "dstAutoCastType":"DT_FLOAT16"
        }
        ```

        - Add the “DT\_FLOAT16” data type to the supported "type". For the supported data types, see the definition of the Cast operator in the corresponding operator information library.
        - Add the "srcAutoCastType" configuration, which indicates the type of the input data.
        - Add the "dstAutoCastType" configuration, which indicates the target data type to convert to.

    > [!NOTE]
    >
    >- If multiple inputs and multiple outputs of an operator require the same data type, each input and output must be modified according to the preceding rules.
    >- Because a Cast operator is inserted, precision is lost to a certain extent. The specific loss depends on the data type to convert to.
