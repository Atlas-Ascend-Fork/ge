# OP\_NO\_REUSE\_MEM

<!-- md-trans-meta sourceCommit=2461b5215e5b8745a550d67c26b5f9840c83434b translatedAt=2026-11-13T10:02:34.449Z pushedAt=2026-11-16T06:11:08.657Z nl=2 -->

## Feature Description

During the compilation of a computation graph on the Ascend platform, memory reuse is adopted by default. In problem locating scenarios, if developers suspect that a memory reuse error causes abnormal computation results, they can use this environment variable to allocate memory separately for a specific operator.

This environment variable can be configured as the name of a network node or the type of an operator (mixed configuration is supported). One or more configurations are supported. When multiple nodes are configured, separate them with commas (,).

## Configuration Example

- Configuration based on node names

    The node name must be configured as the node name after the network is converted to the CANN platform network. The node name can be obtained by setting the DUMP\_GE\_GRAPH environment variable and then viewing the "name" field in the exported ge\_onnx\_xxx\_Build.pbtxt final graph.

    ```bash
    export OP_NO_REUSE_MEM=gradients/logits/semantic/kernel/Regularizer/l2_regularizer_grad/Mul_1,resnet_v1_50/conv1_1/BatchNorm/AssignMovingAvg2
    ```

- Configuration based on operator types

    ```bash
    export OP_NO_REUSE_MEM=FusedMulAddN,BatchNorm
    ```

- Mixed configuration

    ```bash
    export OP_NO_REUSE_MEM=FusedMulAddN,resnet_v1_50/conv1_1/BatchNorm/AssignMovingAvg
    ```

## Constraints

None

## Applicable Products

All chips supported
