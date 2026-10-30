# Usage of the gen\_esb Binary Tool

<!-- md-trans-meta sourceCommit=6910bdd915e3528be24078d698c2568ed03b113b translatedAt=2026-10-19T12:29:41.480Z pushedAt=2026-10-20T12:30:22.780Z nl=2 -->

gen\_esb is a code generation binary tool for ES APIs. It reads the operator prototype .so files in the **op\_graph** subdirectory built into the CANN package based on the **ASCEND\_OPP\_PATH** environment variable, and generates the corresponding ES products based on the prototype information contained in the .so files. For custom **op\_proto** prototype .so files, it is recommended that [add\_es\_library](../es_graph/generate_es_graph_api_optional.md) provided by **generate\_es\_package.cmake** be used to generate and compile the ES products. The generated products include:

- C APIs for all supported operators (ops).
- C++ APIs for all supported operators.
- Python APIs for all supported operators.
- An aggregate header file, allowing users to include all operators at once.
- An aggregate Python file, allowing users to import all operators at once.

For details about the ASCEND\_OPP\_PATH environment variable, see [Environment Variables](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

## Prerequisites

- Before using this tool, install the CANN package. After the installation, the tool is installed in the `$\{INSTALL\_DIR\}/bin` directory, where `$\{INSTALL\_DIR\}` is the storage path of the installed CANN software. For example, when installed by the root user, the default storage path is **/usr/local/Ascend/cann**.
- Set environment variables. The following environment variables use the root user as an example, and the CANN package is installed in the default path.

    ```bash
    source /usr/local/Ascend/cann/set_env.sh
    ```

After the preceding steps are complete, run the **gen\_esb --help** command in any path on the server where the CANN package is located. If information similar to the following is displayed, the tool can be used normally:

```text
ES Graph Builder Code Generator v1.0
Usage: ./gen_esb [--output_dir=DIR] [--module_name=NAME] [--h_guard_prefix=PREFIX] [--exclude_ops=OP_TYPE]
... ...
```

## Usage Examples and Parameters

Run the following command to use the tool.

```bash
./gen_esb [--output_dir=DIR] [--module_name=NAME] [--h_guard_prefix=PREFIX] [--exclude_ops=OP_TYPE1,OP_TYPE2]
```

The parameters are described as follows:

- **--output\_dir**: optional parameter that specifies the target directory for code generation. If not specified, the current directory is used by default.
- **--module\_name**: optional parameter that controls the naming of the aggregate header files:
  - **math** -\> es\_math\_ops\_c.h, es\_math\_ops.h, es\_math\_ops.py
  - **all** -\> es\_all\_ops\_c.h, es\_all\_ops.h, es\_all\_ops.py
  - If **-\>** is not passed, **all** is used by default.

- **--h\_guard\_prefix**: optional parameter that controls the macro guard prefix of the generated header file, which is used to distinguish between internal and external operators that may have the same name:
  - If not specified, the default prefix is used.
  - If specified, the default prefix is concatenated.
  - Python files are not aware of this parameter. In the case of duplicate names, conflicts are avoided by using different paths.

- **--exclude\_ops**: optional parameter that controls the operators to be excluded from generation. Operator names are separated by commas (,).

The following shows usage examples:

- Generate to the current directory, using the default module name **all** and the default macro guard prefix.

    ```bash
    ./gen_esb
    ```

- Generate to a specified directory, using the default module name **all** and the default macro guard prefix.

    ```bash
    ./gen_esb --output_dir=./output
    ```

- Generate to a specified directory, using the module name **math** and the default macro guard prefix.

    ```bash
    ./gen_esb --output_dir=./output --module_name=math
    ```

- Generate to a specified directory, using the module name **all** and the default macro guard prefix.

    ```bash
    ./gen_esb --output_dir=./output --module_name=all
    ```

- Generate to a specified directory, using the module name **math** and the custom macro guard prefix **MY\_CUSTOM**.

    ```bash
    ./gen_esb --output_dir=./output --module_name=math --h_guard_prefix=MY_CUSTOM
    ```

- Generate to a specified directory, using the module name **math** and the custom macro guard prefix **MY\_CUSTOM**, and exclude the Add operator from generation.

    ```bash
    ./gen_esb --output_dir=./output --module_name=math --h_guard_prefix=MY_CUSTOM --exclude_ops=Add
    ```

## Output Files

- **es\_\{module\_name\}\_ops\_c.h**: C API aggregate header file
- **es\_\{module\_name\}\_ops.h**: C++ API aggregate header file
- **es\_\{module\_name\}\_ops.py**: Python API aggregate file
- **es\_<op\_type\>\_c.h**: C API header file of a single operator
- **es\_<op\_type\>.cpp**: C API implementation file of a single operator
- **es\_<op\_type\>.h**: C++ API header file of a single operator
- **es\_<op\_type\>.py**: Python API file of a single operator

> [!NOTE]
> It is recommended to use the CMake function for encapsulation, because the CMake function internally keeps the name of the Python-dependent .so file. The name is consistent with the name of the .so compiled from the ES products. Otherwise, you need to manually name the .so file generated by ES as **libes\_\{module\_name\}.so** in the CMake project to ensure consistency.
