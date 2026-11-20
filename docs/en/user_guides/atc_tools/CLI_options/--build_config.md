# --build\_config

<!-- md-trans-meta sourceCommit=aaa82213df16687539020ed5cae4f006363bb63c translatedAt=2026-11-16T13:40:07.571Z pushedAt=2026-11-17T06:30:29.747Z nl=2 -->

## Product Support

Full chip support.

## Function Description

Specifies the make command template used to generate the dynamic library during source code compilation in the graph compilation phase, for customizing parameters such as the cross-compiler, compilation options, preprocessing options, and linking options. GE automatically supplements the generated Makefile file when performing compilation, so the user does not need to specify a Makefile file.

## Related Parameters

This parameter takes effect only in the source code compilation phase when [--mode](--mode.md)=7 is used to generate an offline model in the \*.om2 format.

## Parameter Values

**Parameter value:** make command template, used to specify the make command, make options, and compilation-related variables used in the source code compilation phase.

**Parameter value format:** The parameter value must be enclosed in double quotation marks. The format is as follows:

```bash
--build_config="<make command> [make options] [variable assignment...]"
```

- \<make command \>: must be make or an absolute path of the make command ending with /make, for example, make or /usr/bin/make.
- \[make options \]: optional item. Common make options are supported, for example, -s and -j16.
- \[ variable assignment \]: optional item. The format is variable name=variable value. A variable value containing spaces can be enclosed in single or double quotation marks, for example, CXXFLAGS='-O2 -fPIC'. Only the following make variables can be configured:

    CXX, CXXFLAGS, CPPFLAGS, LDFLAGS, LDLIBS

**Parameter value constraints:**

- Arbitrary shell commands are not supported. Only the make command template is supported.
- Specifying a Makefile file by the user is not supported, for example, -f, --file, and --makefile.

## Recommended Configuration and Benefits

When the compilation environment is x86\_64, the runtime environment is aarch64, and an offline model in the \*.om2 format is generated ([--mode](--mode.md)=7), if the cross-compilation tool built into the development kit package (used to compile g++ source code for the aarch64 environment in the x86\_64 environment) is required, configure the [--host\_env\_cpu](--host_env_cpu.md) and [--host\_env\_os](--host_env_os.md) parameters. In other cases, it is recommended to use the --build\_config configuration.

## Example

```bash
atc --build_config="make -s -j16 CXX=/usr/bin/aarch64-linux-gnu-g++ CXXFLAGS='-O2 -fPIC'"
```

## Dependency Constraints

None.
