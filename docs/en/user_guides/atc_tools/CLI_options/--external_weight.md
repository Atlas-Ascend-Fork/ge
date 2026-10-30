# --external\_weight

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:04:43.527Z pushedAt=2026-10-23T14:14:14.591Z -->

## Product Support

Supported on all chips

## Feature Overview

When generating an om offline model file, specify whether to externalize the weights of Const/Constant nodes in the original network and convert the node type to FileConstant.

In offline scenarios, if the model weights are large and the environment imposes a limit on the om offline model size, it is recommended to enable weight externalization to save the weights separately and reduce the om size.

## Related Parameters

It must be used together with the [--output](--output.md) parameter. The generated weight file is saved in the weight directory at the same level as the om offline model file.

## Parameter Values

- 0: (default) The weights are not externalized and are directly stored in the om offline model file.
- 1: The weights are externalized but not normalized. The weight files of all Const/Constant nodes in the network are written to disk, and the node type is converted to the FileConstant type. The weights of different nodes are saved to different files, named by the weight\_\<hash value \>.
- 2: Weights are externalized and normalized. The weight files of all Const/Constant nodes in the network are written to disk, and the node type is converted to the FileConstant type. All weights are saved to the same file, named with the file name specified by --output suffixed with \_weight\_combined.

## Recommended Configuration and Benefits

None.

## Example

Take the ONNX network model as an example:

```bash
atc --framework=5 --model=$HOME/module/resnet50.onnx --output=$HOME/module/out/onnx_resnet50 --soc_version=<soc_version>  --external_weight=1 ...
```

## Dependencies

In the weight externalization scenario, when developing an inference application and loading a model:

- If you use the [aclmdlLoadFromFile](../../../api/graph_engine_api/c/acl/aclmdlLoadFromFile.md) API to load the model, save the weight file in the `weight` directory at the same level as the om offline model file.
- If you use the [aclmdlSetConfigOpt](../../../api/graph_engine_api/c/acl/aclmdlSetConfigOpt.md) and [aclmdlLoadWithConfig](../../../api/graph_engine_api/c/acl/aclmdlLoadWithConfig.md) APIs to load the model, there is no requirement on the weight externalization directory. When loading the model later, specify the weight externalization directory through the [aclmdlLoadWithConfig](../../../api/graph_engine_api/c/acl/aclmdlLoadWithConfig.md) API.
