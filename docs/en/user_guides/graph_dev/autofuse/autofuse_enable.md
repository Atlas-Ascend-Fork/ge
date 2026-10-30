# Enabling AutoFuse

<!-- md-trans-meta sourceCommit=bd3de52c8a808742e9c01b524792105f46cad44c translatedAt=2026-10-21T09:00:47.967Z pushedAt=2026-10-28T03:30:34.439Z nl=2 -->

Depending on the fusion implementation path, AutoFuse can be enabled in two ways:

- **GE path**: describes how to enable AutoFuse automatic fusion based on the GE path, as well as the dependency versions, environment variables, and so on.
- **Inductor path**: describes how to enable AutoFuse automatic fusion based on the Inductor path (PyTorch framework), as well as the dependency versions, `torch.compile` configuration, environment variables, and so on.

The following sections describe how to enable AutoFuse in each of the two scenarios.

## Enabling AutoFuse for the GE Path

### Prerequisites

#### Set Up the Runtime Environment

| Dependency | Requirement |
| :---------------------------- | :----------------------------------------------------------- |
| Hardware and Basic Software | Prepare a hardware environment equipped with Ascend AI Processors, and install the matching driver, firmware, and CANN software package. For installation steps, see [CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum). |
| TensorFlow and TF Adapter Plugin | See [Matching Version](https://www.hiascend.com/document/detail/en/TensorFlowCommunity/latest/releasenote/releasenote_01.html) to select a version that matches CANN.<br>**This dependency is required when TensorFlow is used as the frontend framework.** |
| PyTorch and TorchNPU Plugin | See [Matching Version](https://www.hiascend.com/document/detail/en/Pytorch/latest/releasenote/docs/en/release_notes/release_notes.md) to select a version that matches CANN.<br> **This dependency is required when a PyTorch model enters the GE path after being compiled by TorchAir.** |
| TorchAir | Use a version that matches PyTorch/TorchNPU. For installation and source code compilation requirements, see [Official TorchAir Repository](https://gitcode.com/Ascend/torchair).<br>**This dependency is required when a PyTorch model enters the GE path after being compiled by TorchAir.** |
| GCC | 9.5.0 and above; 9.5.0 is recommended. |
| CMake | 3.20.0 and above; 3.20.0 is recommended. |

#### Set Environment Variables

After installing the CANN software, when you use the CANN running user to compile and run, log in to the environment as the CANN running user and run the following command to set the environment variables:

```bash
source ${INSTALL_DIR}/set_env.sh
```

$\{INSTALL\_DIR\} Replace it with the path where the CANN software is stored after installation. For example, if you install the software as the root user, the default storage path after installation is /usr/local/Ascend/cann.

### Enabling AutoFuse

In the GE path, enable AutoFuse by setting environment variables:

```bash
export AUTOFUSE_FLAGS="--enable_autofuse=true"
```

After `--enable_autofuse=true` is configured, the basic AutoFuse fusion feature is enabled (the simplest configuration), which supports automatic fusion between Elemwise operators and Broadcast operators.

For more configurations, see [Environment Variable Reference](../appendix/autofuse_env_vars.md).

## Enabling AutoFuse for the Inductor Path

### Prerequisites

#### Set Up the Runtime Environment

| Dependency | Requirement |
| :----------------------- | :----------------------------------------------------------- |
| Hardware and Basic Software | Prepare a hardware environment equipped with Ascend AI Processors, and install the matching driver firmware and CANN software package. For installation steps, see [CANN Software Installation](https://www.hiascend.com/document/detail/en/CANNCommunityEdition/latest/softwareinst/instg/instg_0000.html?OS=openEuler&InstallType=netyum). |
| PyTorch and TorchNPU Plugin | See [Matching Version](https://www.hiascend.com/document/detail/en/Pytorch/latest/releasenote/docs/en/release_notes/release_notes.md) to select and install a version that matches CANN. |
| GCC | 9.5.0 and above, 9.5.0 recommended. |
| CMake | 3.20.0 and above, 3.20.0 recommended. |

#### Set Environment Variables

After installing the CANN software, when you use the CANN running user to compile and run, log in to the environment as the CANN running user and run the following command to set the environment variables:

```bash
source ${INSTALL_DIR}/set_env.sh
```

$\{INSTALL\_DIR\} Replace it with the storage path of the CANN software after installation. For example, if you install the software as the root user, the default storage path after installation is /usr/local/Ascend/cann.

### Enabling AutoFuse

- On the Inductor path, specify the Ascend C backend in `torch.compile` to enable AutoFuse:

  ```python
  model = torch.compile(
     model,
     options={"npu_backend": "ascendc"},
  )
  ```

  Here, `options={"npu_backend": "ascendc"}` is used to select the Ascend C backend and enable AutoFuse.

- Enable AutoFuse using the decorator form:

  ```python
  @torch.compile(options={"npu_backend": "ascendc"})
  def test_add_ge(x, y, z):
      return torch.ge(torch.add(x, y), z)
  ```

- Enable AutoFuse by setting the environment variable `TORCHINDUCTOR_NPU_BACKEND`:

  ```bash
  export TORCHINDUCTOR_NPU_BACKEND="ascendc"
  ```

  After setting this environment variable, you can directly use `torch.compile` to compile the model without specifying the backend through `options`:

  ```python
  model = torch.compile(model)
  ```

For more environment variables, see [Environment Variable Reference](../appendix/autofuse_env_vars.md).
