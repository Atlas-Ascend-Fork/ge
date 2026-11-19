# build\_initialize

<!-- md-trans-meta sourceCommit=8e99316f00319d82c87d56ea60105e7f826da7c5 translatedAt=2026-10-15T03:30:39.654Z pushedAt=2026-10-16T10:53:07.542Z -->

## Applicable Products

Supported on all chips.

## Description

Model build initialization, which is used to apply for resources.

## Prototype

```python
build_initialize(global_options: Optional[dict] = None) -> None
```

## Parameters

| Parameter | Input/Output | Description |
| --- | --- | --- |
| global_options | Input | Global configuration parameters.<br>A dict mapping of configuration parameters, where the key is the parameter type and the value is the parameter value, both in string format, used to describe the initialization information for offline model compilation.<br>For the configuration parameters supported in the dict, see [Configuration Parameters Supported by aclgrphBuildInitialize](../../../cpp/ge/aclgrphBuildInitialize_config_params/aclgrphbuildinitialize_config_params.md), where the key is the string value corresponding to the C++ constant. |

## Return Value

None

## Constraints

- If **global\_options** is not a valid string dict, a **TypeError** is thrown.
- If initialization fails, a **RuntimeError** is thrown.
- The **build\_initialize** API can be called only once within a process.
- After this API is called, the **build\_model** API can be called multiple times consecutively within the same process.
- **For duplicate **options** compilation configuration parameters in build\_model and build\_initialize, it is recommended not to configure them repeatedly. If duplicates are configured, the values passed in build\_model take precedence**.
