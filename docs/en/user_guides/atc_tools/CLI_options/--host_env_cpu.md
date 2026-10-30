# --host\_env\_cpu

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:08:30.274Z pushedAt=2026-10-23T14:16:57.578Z -->

## Product Support

All chips supported

## Function Description

If the operating system and its architecture of the model compilation environment differ from those of the model running environment, use this parameter to set the operating system architecture of the model running environment.

If this parameter is not set, the operating system architecture of the model compilation environment, that is, the operating system architecture of the environment where the ATC tool resides, is used by default.

## Related Parameters

If the operating system and its architecture of the model compilation environment are inconsistent with those of the model running environment, this parameter must be used together with the [--host\_env\_os](--host_env_os.md) parameter. Use the [--host\_env\_os](--host_env_os.md) parameter to set the operating system type and the --host\_env\_cpu parameter to set the operating system architecture.

## Parameter Values

**Parameter values:**
Run the **atc --help** command to view all values supported by the “--host\_env\_cpu” parameter.

**Default value:**
Run the **atc --help** command to view the default value of the “--host\_env\_cpu” parameter, or view the value in the $\{INSTALL\_DIR\}/opp/scene.info file.

In the preceding path, replace $\{INSTALL\_DIR\} with the file storage path after CANN software installation. For example, when installed as the root user, the default file storage path is /usr/local/Ascend/cann.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --host_env_os=linux --host_env_cpu=x86_64 ...
```

- If the converted offline model contains the operating system type and architecture, for example: _xxx\_linux\_x86\_64_.om, the model can run only on the *Linux* operating system of the *x86\_64* architecture.
- If the converted offline model does not contain the operating system type and architecture, for example: *xxx*.om, the model can run on all operating systems supported by the CANN software package.

## Dependency Constraints

None.
