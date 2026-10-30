# Header Files and Library Files

<!-- md-trans-meta sourceCommit=c37a8745dd3fcf90b9b83f1bd319e462f91f3367 translatedAt=2026-10-28T13:17:35.041Z pushedAt=2026-10-29T06:07:29.360Z nl=2 -->

The related header files are stored in the `${INSTALL_DIR}/include/llm_datadist` directory, and the library files are stored in the `${INSTALL_DIR}/lib64/` directory. Replace `${INSTALL_DIR}` with the file storage path after the CANN software is installed. For example, when installed as the root user, the default file storage path is `/usr/local/Ascend/cann`.

**Table 1** Header file list

| Header File Defining the Interface | Purpose | Corresponding Library File |
| --- | --- | --- |
| llm\_datadist.h | For large model KV Cache transfer scenarios, provides interface capabilities with KV Cache semantics. Supports role management, link management, KV Cache registration, continuous KV Cache transfer, and KV Block transfer. | libllm\_engine.so |
