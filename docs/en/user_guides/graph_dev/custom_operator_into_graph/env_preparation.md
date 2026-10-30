# Environment Preparation

<!-- md-trans-meta sourceCommit=1c0a36284515311df07ca620682d500844a67127 translatedAt=2026-10-21T09:35:59.354Z pushedAt=2026-10-28T03:30:34.498Z nl=2 -->

- Install the firmware and CANN software package: See [Environment Preparation](env_preparation.md) to install the firmware and CANN software package, and set the environment variables.

    The CANN software package provides header files for GE, acl, and others (include/graph, include/register, include/external).

- Install PyTorch, TorchNPU, and TorchAir: These are required in the PyTorch scenario. For the compatibility between them and the CANN software package, click [Link](https://www.hiascend.com/document/detail/en/Pytorch/latest/releasenote/docs/en/release_notes/release_notes.md) and see the "Version Description \> Related Product Version Compatibility" section.

    For detailed installation, click [Link](https://www.hiascend.com/document/detail/en/Pytorch/latest/index/index.html) and see the "Installation Guide \> Software Installation \> Installing TorchNPU" section. TorchAir, as a third-party library of TorchNPU, is installed together with the TorchNPU package.
