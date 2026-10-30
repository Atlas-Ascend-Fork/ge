# ASCEND\_GE\_PY\_PASS\_PATH

<!-- md-trans-meta sourceCommit=2461b5215e5b8745a550d67c26b5f9840c83434b translatedAt=2026-10-20T12:43:37.903Z pushedAt=2026-10-28T11:22:47.793Z nl=2 -->

## Feature Overview

ASCEND\_GE\_PY\_PASS\_PATH is the path discovery environment variable of the GE (Graph Engine) Python Pass system. It tells the GE engine which paths to load user-written Python fusion Pass plugins from.

This environment variable controls a complete "discovery-registration-execution" chain:

1. When the GE compilation phase starts the Pass loading process, the C++ side checks whether ASCEND\_GE\_PY\_PASS\_PATH is set and non-empty.
2. If it is set, GE synchronizes this environment variable to the Python side through the C++/Python bridge.
3. The Python side scans the paths and loads the user-written Pass modules.
4. During module loading, the user registers the Pass with the Python registry through decorators such as @register\_fusion\_pass or @register\_decompose\_pass.
5. The registration information is passed back to the PassRegistry on the C++ side, and is uniformly executed by the GE Pass scheduling framework in the subsequent compilation phases.

At the current stage, ASCEND\_GE\_PY\_PASS\_PATH is the **primary path** (rather than a fallback mechanism) for Python Pass discovery. An automatic discovery mechanism based on entry\_points\(group="ge.passes.plugins"\) will be added in later versions.

Configuration value:

```bash
ASCEND_GE_PY_PASS_PATH=<path1>[:<path2>[:...]]
```

Path requirements: Multiple paths are separated by colons (:). Each path can be a .py file or a directory. Absolute paths are recommended.

Path type description:

| Path Type | Scanning Behavior |
| --- | --- |
| A single .py file | Loaded directly as a Python module |
| A directory | Traverses .py files that do not start with an underscore (_) and loads them as modules; subdirectories containing __init__.py are imported as Python packages |

## Configuration Example

- Points to a single Python file

    ```bash
    export ASCEND_GE_PY_PASS_PATH=/path/to/my_pass.py
    ```

- Points to a directory (all .py files and Python packages in the directory are scanned)

    ```bash
    export ASCEND_GE_PY_PASS_PATH=/path/to/pass_dir/
    ```

- Supports multiple paths separated by colons

    ```bash
    export ASCEND_GE_PY_PASS_PATH=/path/to/pass1.py:/path/to/pass_dir2/
    ```

## Scanning Rules

- **Directory Scanning Rules**

    When a path points to a directory, the bootstrap module scans it according to the following rules:

    1. Files and subdirectories whose names start with \_ are skipped and do not participate in scanning.
    2. .py files that do not start with an underscore are loaded directly as standalone modules.
    3. Subdirectories containing \_\_init\_\_.py are imported as Python packages through importlib.import\_module\(\).
    4. Subdirectories under the directory are scanned in sequence after being sorted by name.

- **Module Naming**

    Each module loaded through a file path is assigned a unique internal name in the format \_ge\_py\_pass\_\{stem\}\_\{hash\}, which avoids conflicts with user-defined module names. The same file path is not loaded repeatedly.

## Constraints

- This environment variable must be set before GE initialization. GE reads this variable when loading Passes for the first time during the compilation phase. Modifying the environment variable at runtime does not take effect.
- The .py file pointed to by the path must contain valid Python code and must register at least one Pass using the @register\_fusion\_pass or @register\_decompose\_pass decorator. A module that does not register any Pass is loaded but has no effect.
- The directory pointed to by the path must exist; otherwise, FileNotFoundError is thrown during the Pass loading phase.
- The file path must end with the .py suffix; otherwise, ValueError is thrown.
- If the environment variable is empty or not set, GE skips the Python Pass loading process, which does not affect the normal loading of custom C++ Passes.
- The same file path is loaded only once. If it appears repeatedly in multiple path segments, it does not cause duplicate registration.

## Applicable Products

Supported on all chips
