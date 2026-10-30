# bundle\_save\_model

<!-- md-trans-meta sourceCommit=8e99316f00319d82c87d56ea60105e7f826da7c5 translatedAt=2026-10-15T03:31:25.143Z pushedAt=2026-10-16T10:53:07.549Z -->

## Product Support

Supported on all chips.

## Description

Serializes an offline model and saves it to a specified file. This API is applicable to weight update scenarios.

## Prototype

```python
bundle_save_model(output_file: str, model: ModelBuffer) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| output_file | Input | Name of the file for saving the offline model. The generated offline model file name automatically ends with the .om suffix. If the .om file name contains the operating system and architecture, the .om file can be used only in the running environment of that operating system and architecture. |
| model | Input | Offline model buffer data. |

## Return Value

None

## Constraints

- If output\_file is not a string, a TypeError is thrown.
- If **model** is not of the ModelBuffer type, a TypeError is thrown.
- If saving fails, a RuntimeError is thrown.
- If the generated OM model file name contains the operating system and architecture, but the operating system and its architecture are inconsistent with the model running environment, use it together with the OPTION\_HOST\_ENV\_OS and OPTION\_HOST\_ENV\_CPU parameters to set the operating system type and architecture of the model running environment. For details about the parameters, see [Configuration Parameters Supported by aclgrphBuildInitialize](../../../cpp/ge/aclgrphBuildInitialize_config_params/aclgrphbuildinitialize_config_params.md).
