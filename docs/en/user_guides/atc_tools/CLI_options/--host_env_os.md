# --host\_env\_os

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-22T03:09:59.532Z pushedAt=2026-10-23T14:17:51.074Z -->

## Product Support

All chips are supported.

## Function Description

If the operating system and its architecture of the model compilation environment differ from those of the model running environment, use this parameter to set the operating system type of the model running environment.

If this parameter is not set, the operating system type of the model compilation environment, that is, the operating system type of the environment where the ATC tool resides, is used by default.

## Related Parameters

- If the operating system and its architecture of the model compilation environment differ from those of the model running environment, this parameter must be used together with the [--host\_env\_cpu](--host_env_cpu.md) parameter. Set the operating system type through the --host\_env\_os parameter and the operating system architecture through the [--host\_env\_cpu](--host_env_cpu.md) parameter.

## Parameter Values

**Parameter values:**
Run the **atc --help** command to view all values supported by the “--host\_env\_os” parameter.

**Default value:**
Run the **atc --help** command to view the default value of the “--host\_env\_os” parameter, or view the value in the $\{INSTALL\_DIR\}/opp/scene.info file.

Replace $\{INSTALL\_DIR\} with the file storage path after CANN is installed. For example, if you install CANN as the root user, the default file storage path is /usr/local/Ascend/cann.

## Recommended Configuration and Benefits

None.

## Example

```bash
atc --host_env_os=linux --host_env_cpu=x86_64 ...
```

- If the converted offline model contains the operating system type and architecture, for example: _xxx\_linux\_x86\_64_.om, the model can run only in the *Linux* operating system of the *x86\_64* architecture.
- If the converted offline model does not contain the operating system type and architecture, for example: *xxx*.om, the model can run in all operating systems supported by the CANN software package.

## Dependency Constraints

None.
