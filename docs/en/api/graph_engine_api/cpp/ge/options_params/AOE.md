# AOE

<!-- md-trans-meta sourceCommit=dab2d32812d032272e3871f14805454ee3acafe7 translatedAt=2026-10-26T12:37:12.001Z pushedAt=2026-10-28T07:42:19.737Z nl=2 -->

## ge.mdl\_bank\_path

<!-- npu="950" id1 -->
Ascend 950PR&950DT products do not support this parameter.
<!-- end id1 -->

Loads the path of the custom knowledge base after subgraph tuning.

This parameter must be used together with the ge.bufferOptimize parameter and takes effect only when data cache optimization is enabled. It improves performance by temporarily storing data in the high-speed cache.

**Parameter value**: Path of the custom knowledge base after model tuning.

**Parameter value format**: Supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), and periods (.).

**Default value:** **$HOME/Ascend/latest/data/aoe/custom/graph/_<soc\_version\>_

**Constraints:**

Priority of loading the custom knowledge base path after subgraph tuning: the path loaded by the **ge.mdl_bank_path** parameter \> the path set by the **TUNE\_BANK\_PATH** environment variable \> the default custom knowledge base path after subgraph tuning.

1. If the custom knowledge base path for subgraph tuning is specified through the **TUNE\_BANK\_PATH** environment variable before model compilation, and the custom knowledge base path is also loaded through the **ge.mdl\_bank\_path** parameter during model compilation, the path loaded through the **ge.mdl\_bank\_path** parameter takes precedence, and the path set by the **TUNE\_BANK\_PATH** environment variable does not take effect.
2. If neither the **ge.mdl\_bank\_path** parameter nor the environment variable specifies a valid path, or no custom knowledge base is available, the default custom knowledge base path is used.
3. If no custom knowledge base is available in any of the paths above, the built-in subgraph tuning knowledge base is searched for at `${INSTALL_DIR}/<arch>-linux/data/fusion_strategy/built-in`. Replace <arch\> with the specific operating system architecture.

**Required/Optional**: Optional

**Effective Level**: Global/session/graph

## ge.op\_bank\_path

<!-- npu="950" id2 -->
Ascend 950PR&950DT products do not support this parameter.
<!-- end id2 -->

Custom knowledge base path after operator tuning.

**Parameter value format**: Supports uppercase and lowercase letters (a-z, A-Z), digits (0-9), underscores (\_), hyphens (-), and periods (.).

**Parameter default value:** **$\{HOME\}/Ascend/latest/data/aoe/custom/op

**Constraints:**

Priority for loading the custom knowledge base path after operator tuning: the path set by the **TUNE_BANK_PATH** environment variable \> the path loaded by the **\>ge.op\_bank\_path** parameter \> the default custom knowledge base path after operator tuning.

1. If the custom knowledge base path for operator tuning is specified through the **TUNE\_BANK\_PATH** environment variable before model conversion, and the custom knowledge base path is also loaded through the ge.op\_bank\_path parameter during model compilation, the path set by the **TUNE\_BANK\_PATH** environment variable takes precedence, and the path loaded by the **ge.op\_bank\_path** parameter does not take effect.
2. If neither the path specified by the **ge.op\_bank\_path** parameter nor the path specified by the environment variable takes effect, the default custom knowledge base path is used.
3. If no usable custom knowledge base is available in any of the above paths, the built-in operator tuning knowledge base is searched.

**Required/Optional**: Optional

**Effective level**: Global/session/graph
