# --oo\_level

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-11-16T14:19:23.493Z pushedAt=2026-11-17T11:19:46.684Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

**This is an extended debugging parameter. It is not supported in production environments and will be released as a formal feature in a later version.**

Multi-level graph compilation optimization option, including subgraph optimization, whole-graph optimization, and static shape model offloading.

Static shape model offloading: For a static shape model, the input and output shapes of all operators can be determined at compile time. Host-side computations such as model-level memory planning and operator Tiling computation are completed at compile time. When the model is loaded, these computations are delivered to the device stream as a whole but are not executed immediately. All tasks in the model are triggered by issuing a model execution task.

## Related Parameters

None.

## Parameter Values

- O1: Disables all graph fusion and UB fusion passes, and performs only the optimizations that facilitate static sinking, such as InferShape (which derives the shape of output tensors), constant folding, and dead edge elimination.

    <!-- npu="950" id1 -->
    The Ascend 950PR&950DT products do not support UB fusion, so only the graph fusion passes are disabled.
    <!-- end id1 -->

- O3: (Default) Enables all optimizations.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --oo_level=O1 ...
```

## Constraints

When the value is O1, all graph fusion and UB fusion passes are disabled, and only the passes related to static sinking are enabled. However, the graph fusion passes in the file at the following path are enabled by default because disabling them would cause functional issues:

All graph fusion passes under the "ExceptionalPassOfO1Level" field in the `${INSTALL_DIR}/x86_64-linux/lib64/plugin/opskernel/fusion_pass/config/fusion_config.json` file.

In the command, replace $\{INSTALL\_DIR\} with the file storage path after CANN software installation. For example, if the software is installed by the root user, the default storage path is /usr/local/Ascend/cann.
