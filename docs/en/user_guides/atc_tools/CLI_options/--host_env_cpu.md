# --host\_env\_cpu

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-16T14:01:28.708Z pushedAt=2026-11-17T08:55:15.174Z nl=2 -->

## Product Support

Supported on all chips.

## Function Description

If the operating system and its architecture of the model compilation environment differ from those of the model running environment, use this parameter to set the operating system architecture of the model running environment.

If this parameter is not set, the operating system architecture of the model compilation environment, that is, the operating system architecture of the environment where the ATC tool resides, is used by default.

## Related Parameters

If the operating system and its architecture of the model compilation environment are inconsistent with those of the model running environment, this parameter must be used together with the [--host\_env\_os](--host_env_os.md) parameter. Use the [--host\_env\_os](--host_env_os.md) parameter to set the operating system type and the --host\_env\_cpu parameter to set the operating system architecture.

## Parameter Values

**Parameter values:**
Run the **atc --help** command to view all values supported by the "--host\_env\_cpu" parameter.

**Default value:**
Run the **atc --help** command to view the default value of the "--host\_env\_cpu" parameter, or view the value in the $\{INSTALL\_DIR\}/opp/scene.info file.

In the command, replace $\{INSTALL\_DIR\} with the file storage path after CANN software installation. For example, if the software is installed by the root user, the default storage path is /usr/local/Ascend/cann.

## Recommended Configuration and Benefits

When the compilation environment is x86\_64, the operating environment is aarch64, and an \*.om2 offline model is generated ([--mode](--mode.md)=7), if the cross-compilation tool built into the development kit package is required (for compiling g++ source code for the aarch64 environment in the x86\_64 environment), configure the --host\_env\_cpu and [--host\_env\_os](--host_env_os.md) parameters. In other cases, the [--build\_config](--build_config.md) configuration is recommended.

## Example

```bash
atc --host_env_os=linux --host_env_cpu=x86_64 ...
```

- If the converted offline model contains the operating system type and architecture, for example: _xxx\_linux\_x86\_64_.om, the model can run only in the *Linux* operating system of the *x86\_64* architecture.
- If the converted offline model does not contain the operating system type and architecture, for example: *xxx*.om, the model can run in all operating systems supported by the CANN software package.

## Dependency Constraints

None.
