# Environment Preparation

<!-- md-trans-meta sourceCommit=557fc263a581ed94fc4bb5447e90358eea64338d translatedAt=2026-11-06T11:09:27.069Z pushedAt=2026-11-13T07:37:03.619Z nl=2 -->

- Install the firmware and CANN software package: See [Environment Preparation](env_preparation.md) to install the firmware and CANN software package, and set the environment variables.

    The CANN software package provides header files for GE, acl, and others (include/graph, include/register, include/external).

- Install PyTorch, TorchNPU, and TorchAir: These are required in the PyTorch scenario. For the compatibility between them and the CANN software package, click [Link](https://www.hiascend.com/document/detail/en/Pytorch/latest/releasenote/docs/en/release_notes/release_notes.md) and see the "Version Description \> Related Product Version Compatibility" section.

    For detailed installation, click [Link](https://www.hiascend.com/document/detail/en/Pytorch/latest/index/index.html) and see the "Installation Guide \> Software Installation \> Installing TorchNPU" section. TorchAir, as a third-party library of TorchNPU, is installed together with the TorchNPU package.
