# ASCEND\_ENGINE\_PATH

<!-- md-trans-meta sourceCommit=2461b5215e5b8745a550d67c26b5f9840c83434b translatedAt=2026-10-20T12:43:34.124Z pushedAt=2026-10-28T11:22:43.088Z nl=2 -->

## Description

In the scenario of converting a single-operator JSON file into an offline model, if you want only TBE operators to be used during model conversion (AI CPU operators are not searched for, and an error is reported if no TBE operator is found), you need to use this environment variable.

After this environment variable is set, if you want to perform other operations, you need to delete the environment variable by running the **unset ASCEND\_ENGINE\_PATH** command to make it invalid.

## Configuration Example

```bash
export ASCEND_ENGINE_PATH=${INSTALL_DIR}/lib64/plugin/opskernel/libfe.so:${INSTALL_DIR}/lib64/plugin/opskernel/libge_local_engine.so:${INSTALL_DIR}/lib64/plugin/opskernel/librts_engine.so
```

Replace $\{INSTALL\_DIR\} with the file storage path after CANN software installation. For example, when installed as the root user, the default file storage path is /usr/local/Ascend/cann.

## Constraints

This environment variable applies only to the scenario where an operator JSON file is converted into an offline OM model.

## Applicable Products

Supported on all chips
