# Environment Preparation

<!-- md-trans-meta sourceCommit=6910bdd915e3528be24078d698c2568ed03b113b translatedAt=2026-10-21T10:04:35.694Z pushedAt=2026-10-28T09:05:22.638Z nl=2 -->

Before development, you need to set up the corresponding environment, including installing software packages and setting environment variables. This chapter provides detailed instructions.

## Installing Driver Firmware and the CANN Software Package

Prepare a hardware environment with an AI processor. Complete the environment setup by referring to [CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/920/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum), and ensure that the CANN Toolkit development suite and the ops operator package are installed (**for version 8.5.0 and later**, when compiling a Graph into an offline model, you must install the ops operator package that matches the target AI processor; otherwise, compilation fails). After the installation is complete:

- “$\{INSTALL\_DIR\}/**opp/built-in/op\_graph/inc**” provides the CANN operator prototype definitions, which are used to build a Graph through operator prototypes.
- “$\{INSTALL\_DIR\}/**include/graph**” provides the Graph building APIs.
- “$\{INSTALL\_DIR\}/**include/ge**” provides the Graph running APIs.
- “$\{INSTALL\_DIR\}/**_<arch\>_-linux/devlib**” contains the related dependency libraries. <arch\> is the operating system architecture.

Here, $\{INSTALL\_DIR\} must be replaced with the storage path of the files after the CANN software is installed. Taking installation by the root user as an example, the default storage path of the files after installation is /usr/local/Ascend/cann.

## Setting Environment Variables

- **Required environment variables**

    After installing the CANN software, when you use the CANN running user to compile and run programs, log in to the environment as the CANN running user and execute the following environment variable settings:

    ```bash
    source /usr/local/Ascend/cann/set_env.sh
    ```

    In the preceding commands, /usr/local/Ascend/ is the default installation path of the root user. Replace it with the actual path.

- **Optional environment variables**

    If you want the files generated during program compilation and running to be stored in a unified directory, use the environment variables ASCEND\_CACHE\_PATH and ASCEND\_WORK\_PATH to set the storage path of shared files and the storage path of process-exclusive files, respectively.

    ```bash
    export ASCEND_CACHE_PATH=/repo/task001/cache
    export ASCEND_WORK_PATH=/repo/task001/172.16.1.12_01_03
    ```

    For details about the usage constraints of the environment variables ASCEND\_CACHE\_PATH and ASCEND\_WORK\_PATH and the description of the files written to disk, see [Environment Variables](https://gitcode.com/cann/docs/blob/9.2.0/docs/en/env-vars/README.md).

>[!NOTE]
>
>- After you set environment variables using the export command, the environment variables take effect only in the current window. You can write the preceding commands to an environment variable configuration file (such as the .bashrc file) as needed.
>- Before configuring optional environment variables, run the **env** command to check whether the ASCEND\_CACHE\_PATH and ASCEND\_WORK\_PATH environment variables already exist. It is recommended that all system functions use a unified planned path.
