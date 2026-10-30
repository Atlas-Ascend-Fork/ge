# DUMP\_GRAPH\_PATH

<!-- md-trans-meta sourceCommit=1dea726c9fa59e3b81454a7fd0930f2a4914fdd3 translatedAt=2026-10-20T12:47:26.660Z pushedAt=2026-10-28T11:30:33.649Z nl=2 -->

## Function Description

Specifies the path for saving DUMP graph files. The path can be an absolute path or a relative path to the script execution directory.

The specified path supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), periods (.), and Chinese characters. The executing user must have read, write, and execute permissions.

- If the specified path does not exist, the corresponding directory is automatically created.
- If the executing user has insufficient permissions on the specified path, DUMP graph files are generated in the current execution directory.

> [!NOTE]
>
>1. When the “DUMP\_GRAPH\_PATH” environment variable is set, the program creates a `pid_${pid}_deviceid_${deviceid}` subfolder under the specified path to store DUMP graph files. Here, `${pid}` is the process ID and `${deviceid}` is the device ID.
>2. If the “DUMP\_GRAPH\_PATH” environment variable is not set, DUMP graph files are stored directly in the current script execution path, and the `pid_${pid}_deviceid_${deviceid}` subfolder is not created.

The priority of DUMP graph file storage paths is as follows:

environment variable “ASCEND\_DUMP\_PATH”\> environment variable “DUMP\_GRAPH\_PATH” \> environment variable “ASCEND\_WORK\_PATH” \> default storage path (current script execution path).

## Configuration Example

```bash
export DUMP_GRAPH_PATH=/home/dumpgraph
```

The DUMP graph file is stored in the `/home/dumpgraph/pid_${pid}_deviceid_${deviceid}` path, for example, /home/dumpgraph/pid\_53343\_deviceid\_0.

## Usage Constraints

This environment variable must be used together with [DUMP\_GE\_GRAPH](DUMP_GE_GRAPH.md). That is, when [DUMP\_GE\_GRAPH](DUMP_GE_GRAPH.md) is enabled, you can use DUMP\_GRAPH\_PATH to specify the storage path of DUMP graph files.

## Applicable Products

Supported on all chips
