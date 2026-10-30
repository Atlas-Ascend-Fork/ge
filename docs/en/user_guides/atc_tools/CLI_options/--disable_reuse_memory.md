# --disable\_reuse\_memory

<!-- md-trans-meta sourceCommit=6910bdd915e3528be24078d698c2568ed03b113b translatedAt=2026-10-22T02:56:14.593Z pushedAt=2026-10-23T13:36:00.684Z -->

## Product Support

Supported on all chips.

## Function Description

Memory reuse switch

Memory reuse refers to reusing non-conflicting memory based on lifecycle and memory size to reduce network memory usage.

## Related Parameters

None.

## Parameter Values

- 0: (default) Enables memory reuse.
- 1: Disables memory reuse. If the network model is large, disabling memory reuse prevents memory on the Device side from being reused during subsequent inference, which may lead to insufficient memory.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --disable_reuse_memory=0 ...
```

## Dependencies

In the memory reuse scenario (memory reuse is enabled by default), you can allocate memory separately for specified operators (by node name or operator type). Use the OP\_NO\_REUSE\_MEM environment variable to specify one or more nodes for which memory is allocated separately. Mixed configuration is supported. When configuring multiple nodes, separate them with commas \(“,”\). For details, see [*Environment Variable Reference*](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

- Configuring by node name

    The node name must be the node name in the network after it is converted to the CANN platform. You can obtain the node name by setting the DUMP\_GE\_GRAPH environment variable and then checking the "name" field in the exported ge\_onnx\_xxx\_Build.pbtxt final graph.

    ```bash
    export OP_NO_REUSE_MEM=gradients/logits/semantic/kernel/Regularizer/l2_regularizer_grad/Mul_1,resnet_v1_50/conv1_1/BatchNorm/AssignMovingAvg2
    ```

- Configuring by operator type

    ```bash
    export OP_NO_REUSE_MEM=FusedMulAddN,BatchNorm
    ```

- Mixed configuration

    ```bash
    export OP_NO_REUSE_MEM=FusedMulAddN,resnet_v1_50/conv1_1/BatchNorm/AssignMovingAvg
    ```
