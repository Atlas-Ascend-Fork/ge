# (Optional) Generating ES Graph Construction APIs

<!-- md-trans-meta sourceCommit=6693584152164a17682ed389feb8e51961ab9533 translatedAt=2026-10-21T09:56:52.770Z pushedAt=2026-10-28T08:20:23.838Z nl=2 -->

This section describes how to use the ES Code Generator to generate graph construction APIs that users can directly use. This scenario requires that the user already has an application and that a CMake file exists in the application.

The content in this section is optional. The specific usage scenarios are as follows:

- **Optional scenario**, the built-in operator scenario, in which users do not need to perform the steps in this section:
  - The CANN software package installed by the user already contains the ES API artifacts for built-in operators. The path is: $\{INSTALL\_DIR\}/include/es.

    In the path, replace $\{INSTALL\_DIR\} with the file storage path after the CANN software is installed. For example, when installed as the root user, the default file storage path is: /usr/local/Ascend/cann.

  - Users can directly call the ES APIs by referencing the relevant header files and linking to the corresponding .so library, or by importing the corresponding Python module.

- **Mandatory scenario**, in which the steps in this section must be performed:
  - The installed CANN software package does not contain the ES graph construction API deliverables, and ES is required for graph construction.
  - Custom scenario:
    - If the user-defined custom operators (Custom Ops) also need to use the ES APIs to build graphs, Codegen can be manually executed to generate the default ES graph construction APIs strictly based on the IR prototypes.
    - If the existing ES APIs cannot meet the requirements, a new ES API can be redefined to replace the existing implementation.

## Prerequisites

- The user has installed the CANN software package and set the following environment variables: (The following environment variables use the root user as an example, and the CANN software package is installed in the default path.)

    ```bash
    source /usr/local/Ascend/cann/set_env.sh
    ```

- The user environment has the following dependencies, which must meet the corresponding version requirements:
  - CMake version: \>= 3.17
  - Python3: \>= 3.7 (required only for add\_es\_library\_and\_whl)
  - Install Python dependencies: setuptools, wheel (required only for add\_es\_library\_and\_whl)

- The user's CMake file has already defined the prototype dynamic library:

    ```cmake
    add_library(opgraph_math SHARED)
    ```

## Modifying the CMake File

1. Importing Functions

    ```cmake
    # Add the module path so that CMake can find the GenerateEsPackage module.
    list(APPEND CMAKE_MODULE_PATH "/usr/local/Ascend/cann/include/ge/cmake")
    # Find the module.
    find_package(GenerateEsPackage REQUIRED)
    ```

    In the current version, you need to manually add CMAKE\_MODULE\_PATH to CMakeLists.txt.

2. Generating the ES Graph APIs

    1. Call add\_es\_library\_and\_whl to generate the C/C++ dynamic library and Python wheel package (complete artifact: the carrier of the APIs):

        ```cmake
        add_es_library_and_whl(
            ES_LINKABLE_AND_ALL_TARGET es_math
            OPP_PROTO_TARGET  opgraph_math # prototype library target in the prerequisites
            OUTPUT_PATH       ${CMAKE_BINARY_DIR}/output
        )
        ```

    2. Call add\_es\_library to generate **only** the C/C++ dynamic library (for pure C/C++ projects):

       ```cmake
       add_es_library(
          ES_LINKABLE_AND_ALL_TARGET es_math
          OPP_PROTO_TARGET  opgraph_math
          OUTPUT_PATH       ${CMAKE_BINARY_DIR}/output
        )
        ```

    The add\_es\_library\_and\_whl and add\_es\_library functions are wrappers of the gen\_esb binary tool, including behaviors such as source file compilation and Python whl package compilation and packaging. This tool has been integrated into the CANN software package. For details about how to use the tool, see [Usage of the gen\_esb Binary Tool](../appendix/gen_esb_tool_usage.md). For detailed parameter descriptions of the functions, see the following table.

    **Table 1** CMake file parameters

    |Parameter|Description|Example|
    |--|--|--|
    |ES_LINKABLE_AND_ALL_TARGET|Required. Name of the target in the generated artifact.|es_math, es_nn, es_cv. It is recommended to use the es_ prefix (for example, es_math and es_nn), lowercase letters, and underscores. Avoid special characters and C++ keywords.|
    |OPP_PROTO_TARGET|Required. CMake target name of the operator prototype library. ES APIs are generated for the operators in this prototype library.|opgraph_math, opgraph_nn|
    |OUTPUT_PATH|Optional. Root directory for the generated artifact output.|${CMAKE_BINARY_DIR}/output|
    |EXCLUDE_OPS|Optional. Operators to be excluded from generation.|Add, Conv2D|

3. Using the generated artifacts

    ```cmake
    # Create an executable file named my_app with main.cpp as the source file.
    add_executable(my_app main.cpp)
    # Link the my_app executable with the es_math target. PRIVATE indicates that the dependency is visible only to my_app itself.
    target_link_libraries(my_app PRIVATE es_math)
    ```

    > [!NOTE]
    >
    >Because ES graph APIs are generated based on operator IR prototypes, some operator IRs cannot express all information (for example, Conv-type operators) or contain redundant information (for example, the IdentityN operator). Therefore, it is recommended that you encapsulate ES APIs by yourself. In this scenario, you can customize ES APIs. For details, see [Custom ES API](../../../../../examples/custom_es_api/README.md).

The following is a complete CMake file example. Replace the CANN software package installation path /usr/local/Ascend/ with the actual path. For more CMake examples, see [Common CMake Examples](../appendix/cmake_examples.md).

```cmake
# ===== Basic settings =====
cmake_minimum_required(VERSION 3.17)
project(my_es_project LANGUAGES CXX)

# ===== Import functions (recommended: use find_package) =====
list(APPEND CMAKE_MODULE_PATH "/usr/local/Ascend/cann/include/ge/cmake")
find_package(GenerateEsPackage REQUIRED)

# ===== Part 1: Prerequisites: define the prototype library =====
add_library(opgraph_math SHARED
)

# ===== Part 2: Generate the ES API package =====
add_es_library_and_whl(
    ES_LINKABLE_AND_ALL_TARGET es_math
    OPP_PROTO_TARGET  opgraph_math
    OUTPUT_PATH       ${CMAKE_BINARY_DIR}/output
)

# ===== Part 3: Use in the application =====
add_executable(my_app
    src/main.cpp
    src/inference.cpp
)

# Link ES_LINKABLE_AND_ALL_TARGET
target_link_libraries(my_app PRIVATE
    es_math  # Automatically obtain dependencies, header files, and libraries
)
```

The build command is:

```bash
make my_app
# Output:
# [Smart Build] Step 1: Building internal targets...
# [Smart Build] Found xx generated source file(s)
# [Smart Build] Step 2: Reconfiguring to include generated sources...
# [Smart Build] Step 3: Rebuilding with all generated sources...
# [Smart Build] Successfully built ES package 'es_math' with xx source file(s)
# Built target my_app
```

## Product Description

- The product of add\_es\_library\_and\_whl is:

    ```text
    OUTPUT_PATH/
    ├── include/
    │   └── es_math/               # Header file directory
    │       ├── es_math_ops.h      # C++ API aggregate header file, which contains the graph construction APIs of all operators under es_math
    │       ├── es_math_ops_c.h    # C API aggregate header file
    │       └── es_Add.h            # Header file of a single operator
    │       └── es_xxx.h            # ES graph construction header file corresponding to other operator prototypes
    ├── lib64/
    │   └── libes_math.so          # Dynamic library
    └── whl/
        └── es_math-1.0.0-py3-none-any.whl  # Python package. When using Python for graph construction, you need to manually install this package.
    ```

- The product of add\_es\_library is:

    ```text
    OUTPUT_PATH/
    ├── include/
    │   └── es_math/               # Header file directory
    │       ├── es_math_ops.h      # C++ API aggregate header file
    │       ├── es_math_ops_c.h    # C API aggregate header file
    │       └── es_Add.h           # Header file of a single operator
    │       └── es_xxx.h           # ES graph header file corresponding to other operator IR prototypes
    └── lib64/
    │   └── libes_math.so          # Dynamic library
    └── whl/                        # No whl package is generated in C/C++ scenarios, and this directory is empty
    ```

The artifact naming rules are as follows:

**Table 2** Artifact naming rules

|Artifact Type|Naming Rule|Example (Using ES_LINKABLE_AND_ALL_TARGET=es_math as an Example)|
|--|--|--|
|Dynamic library|lib<ES_LINKABLE_AND_ALL_TARGET>.so|libes_math.so|
|Dynamic library: external library|<ES_LINKABLE_AND_ALL_TARGET>|For es_math, the es_ prefix is recommended (for example, es_math and es_nn). Use lowercase letters and underscores. Avoid special characters and C++ keywords.|
|Python package|<ES_LINKABLE_AND_ALL_TARGET>-1.0.0-py3-none-any.whl|es_math-1.0.0-py3-none-any.whl|
|Aggregate header file|es_\<name>_ops.h|es_math_ops.h|
