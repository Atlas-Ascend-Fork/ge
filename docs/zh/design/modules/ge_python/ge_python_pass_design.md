# GE Pass Python化V1设计文档

## 1. 背景

当前GE已具备两类与本需求直接相关的基础能力：

- 现有自定义pass装载链路已经存在，GE会通过`opp/vendors/*/custom_fusion_passes/*.so`的方式发现并`dlopen` pass库。
- Python侧已经具备`ge.es`构图能力与`ge.graph`基础图接口。

本设计的目标是在不推翻现有GE pass执行框架的前提下，引入正式的Python pass开发能力，使用户既可以快速本地开发和验证，也可以将pass以标准Python包的形式进行团队分发。

## 2. 目标与范围

### 2.1 目标

- 支持用户使用Python开发GE pass。
- 复用现有GE pass执行链路，不新增第二套pass调度框架。
- 复用已有`ge.es` Python构图能力，不重新设计Python ES。
- 首先支持环境变量驱动的开发态接入，后续再补发布态自动发现。
- 为后续Python ATC入口复用同一套pass注册和发现协议预留扩展点。

### 2.2 V1范围

- 支持三类pass：
- `FusionBasePass`
- `PatternFusionPass`
- `DecomposePass`

- bring-up与独立bridge拆分阶段仍以`FusionBasePass`回归作为最小验证链路；正式wrapper化后的首个完整验收目标转为`PatternFusionPass`，`DecomposePass`在其后补齐，但整体仍属于本设计文档的V1范围。

- 当前阶段发现机制先收敛为：
- 环境变量`ASCEND_GE_PY_PASS_PATH`

- 后续阶段再补：
- `entry_points(group="ge.passes.plugins")`

- 当前代码已先收敛到环境变量主路径，`entry_points`作为后续阶段能力补齐。

- 补齐Python pass编写所需的最小图接口能力。
- 当前仓直接产出`ge_py`主wheel和多版本native子wheel。
- `examples/fusion_pass`下现有9个sample规划提供Python对照版本。
- `REGISTER_CUSTOM_PASS`需要支持，但不作为首批主链路目标，放在后续扩展阶段基于同一套bridge / registry / session机制复用实现。
- 当前优先整改项：
  - 发现机制先收敛到环境变量`ASCEND_GE_PY_PASS_PATH`
  - `entry_points`后续补齐
  - `python_pass_bootstrap_test.py`迁移到`tests/ge/ut/ge/graph/pyge_tests/`并接入当前前端脚本
  - 新增文件年份统一使用`2026`，现有老文件不做批量改年份
  - 第一优先级要以“`FusionBasePass`正式sample通过环境变量方式端到端跑通”为目标，凡是这个目标涉及的能力都要正式完成，不涉及的能力可后置
  - 阶段2收口后，`PassContext` / `MatchResult` / `Pattern` / `PatternMatcherConfig`的正式Python形态统一直接依赖`_ge_pass_native.so`，不再保留bring-up期兼容shim

### 2.3 非目标

- V1不强制用户将pass打成whl。
- V1首批不以legacy `REGISTER_CUSTOM_PASS`体系作为主要Python化对象，优先覆盖`PassRegistry`体系的三类pass；但架构和文档需要预留后续接入能力。
- V1不新建第二套Python-only pass执行器。

## 3. 用户体验设计

### 3.1 开发态

用户安装CANN提供的`ge_py`后，只需要写普通`.py`文件或普通Python package，并通过环境变量告诉GE/ATC：

- `ASCEND_GE_PY_PASS_PATH=/abs/path1:/abs/path2`

当前阶段先把这一路径打磨稳定，不要求用户自己写wheel打包逻辑，也不要求用户理解`entry_points`。

### 3.2 发布态（后续阶段）

当用户需要团队共享、版本固化、自动发现时，可以把pass做成独立Python包，并声明：

- `entry_points = {"ge.passes.plugins": [...]}`

GE在运行时自动发现已安装的pass插件包。

### 3.3 说明

当前阶段环境变量不是兜底，而是主路径：

- `ASCEND_GE_PY_PASS_PATH=/abs/path1:/abs/path2`

后续再补`entry_points`自动发现。

## 4. 总体架构

### 4.1 架构原则

- GE在初始化阶段通过一个上层统一loader收口pass插件加载。
- legacy custom pass继续沿用现有`.so + dlopen`机制。
- 从长期产品化和扩展性角度，Python pass bridge应从一开始就按“独立内部bridge `.so`”设计，而不是直接编入`ge_compiler.so`。
- 这里仍然不采用“作为pass插件被`custom_fusion_passes`发现和装载的bridge `.so`”方案；推荐的是一个由GE内部loader显式装载的私有bridge `.so`。
- 设计目标是让`ge_compiler.so`尽量保持Python ABI中立，只保留稳定的pass runtime、注册表和adapter协议；所有直接依赖`Python.h` / `pybind11` / `libpython`的逻辑都应收敛到可替换的独立bridge `.so`中。
- 这样后续无论走预编译还是fallback codegen，替换目标都是独立bridge `.so`，而不是run包中的`ge_compiler.so`。
- Python侧统一通过`ge.passes.bootstrap`管理插件发现、模块导入和注册表。
- C++ 侧只关心“得到可执行pass descriptor并注册到PassRegistry”，不关心用户Python文件具体在哪里。

### 4.2 核心组件

- `PassPluginLoader`
- 位于compiler层的统一pass插件加载入口
- 内部统一调legacy `CustomPassHelper::Load()`和Python pass注册逻辑
- 保持“一个调用入口闭环”，同时不把Python逻辑反向塞进`graph_metadef/register`

- `ge.passes.bootstrap`
- Python侧统一发现入口
- 当前以环境变量发现为主，后续再补`entry_points`

- `ge.passes.registry`
- Python侧注册表
- 负责存放pass元数据、类对象、阶段、类型、附加参数

- `ge.passes._bridge`
- Python与C++ bridge的协议层
- 负责把Python注册表对象规范化为C++ 可消费的数据结构

- `ge.passes.fallback_runtime`
- Python侧native artifact运行时管理入口
- 负责预制artifact选择、fallback codegen触发和`_ge_pass_native.so`装载

- `_ge_pass_native`
- 由`PYBIND11_MODULE`导出的Python helper模块
- 只承载`Graph` / `PassContext` / `MatchResult`等native-backed wrapper与helper
- 不承载`FusionBasePass` / `PatternFusionPass` / `DecomposePass`这些用户可继承的pass基类

- C++ pass adapter
- 为三类Python pass提供对应C++ 适配类
- 在适配类中回调Python对象方法

- 作为pass插件被`dlopen`的独立bridge `.so`
- 当前主方案已明确放弃
- 文档中凡是基于“新增一个被`custom_fusion_passes`发现的bridge `.so`”展开的描述，均应理解为“统一loader + 私有内部bridge `.so`”，而不是新的pass插件发现链

- 私有内部bridge `.so`
- 这是本设计推荐的正式方向，而不是可选优化项
- 它不是pass插件，不参与`custom_fusion_passes`发现；它承载完整的Python版本敏感bridge逻辑，包括解释器初始化、GIL、对象转换、异常翻译和Python回调
- 正式交付时它与`_ge_pass_native.so`组成同一套bridge artifact set，二者都需要纳入预编译与fallback管理

### 4.3 Native绑定策略

V1采用“两层绑定”策略，而不是把所有Python接口一次性迁到同一种绑定方式：

- `ge.graph` / `ge.es`
- 继续复用仓内现有的C wrapper + `ctypes`路线
- 这样改动面最小，可以优先复用现有Python图接口与eager-style构图能力

- Python pass bridge / adapter层
- 使用`pybind11`作为核心实现策略
- 原因是这部分需要更自然地处理`MatchResult`包装、Python对象生命周期、异常翻译和GIL管理

- 版本发布策略
- 主wheel负责Python代码、发现逻辑和运行时选择逻辑
- `cp37-cp314`提供预编译`pybind11` native子wheel
- 未命中预编译版本时，运行时fallback codegen作为最后兜底

也就是说，V1并不是“全量pybind化”，而是“图接口沿用现有ctypes/C wrapper，pass bridge采用pybind11”。

### 4.4 pybind11使用方式选择

`pybind11`在本方案里有两种典型使用方式：

- embed模式
- 由C++ 进程初始化Python解释器、`import` Python模块并回调Python对象

- extension模式
- 通过`PYBIND11_MODULE`把C++ 能力导出为Python可直接`import`的native模块

当前已存在的`compiler/graph/fusion/pass/python_fusion_base_pass_pybind_bridge.cc`采用的是embed模式，而不是extension模式。原因是：

- 当前主链的方向是“GE compiler调Python pass”，不是“Python主动import一个C++ pass运行时再反向驱动compiler”
- `FusionBasePass`第一阶段只需要让C++ 安全地创建Python对象并调用其`run()`，不需要先把C++ 基类暴露给Python继承
- embed模式可以先复用现有`ge.passes.bootstrap / registry / _bridge`的纯Python组织方式，降低首批落地成本

因此当前bridge文件不会出现`PYBIND11_MODULE`宏，这不是缺失，而是有意选择的模式差异。

但从长期设计看，embed bridge不应继续直接编入`ge_compiler.so`。当前工作区已经把它拆到独立`libge_python_pass_bridge.so`，这也是后续应持续保持的正式边界。更合理的形态是：

- `ge_compiler.so`
- 只持有稳定的loader、descriptor / adapter协议和最小C/C++ 交互面

- 独立内部bridge `.so`
- 采用embed或extension中更合适的具体实现
- 承担所有Python版本敏感的native逻辑

- `_ge_pass_native.so`
- 作为Python可直接`import`的helper extension
- 为Python层提供native-backed wrapper与helper，但不定义用户可继承的pass基类

需要进一步强调的是，当前方案有两条明确边界，不能混用：

- 用户pass定义层
- 继续保持纯Python形式，用户继承`ge.passes.base.FusionBasePass` / `PatternFusionPass` / `DecomposePass`

- native helper / wrapper层
- 由`_ge_pass_native.so`提供`Graph` / `PassContext` / `MatchResult`等wrapper
- 由`libge_python_pass_bridge.so`提供embed路径的运行时桥接

也就是说，本方案不要求也不推荐把C++ `FusionBasePass` / `PatternFusionPass`基类通过`PYBIND11_MODULE`暴露给Python继承。

需要特别说明的是，Python版本敏感并不是只有`PYBIND11_MODULE`才会存在。只要native代码直接依赖：

- `Python.h`
- `pybind11`
- `libpython`

无论是embed还是extension，都天然会和Python minor version产生二进制耦合。`PYBIND11_MODULE`只是extension模式的导出入口，不是版本敏感的根因。

#### 4.4.1 `_ge_pass_native.so`与`libge_python_pass_bridge.so`的装载位置差异

虽然这两个产物都属于Python pass bridge artifact set，但它们处在两条不同的装载路径上：

- `_ge_pass_native.so`
  - 作为Python extension module，由Python解释器通过`import ge.passes._ge_pass_native`装载
  - 它进入进程时，宿主解释器已经存在，因此在Linux上可以不把`libpython.so`显式写入ELF `NEEDED`
  - 这类extension通常在import时直接向当前解释器进程解析`Py_*` / `PyObject_*`符号

- `libge_python_pass_bridge.so`
  - 作为GE内部loader视角的embed bridge，由`ge_compiler`侧显式`dlopen`
  - 它负责解释器初始化、解释器复用、GIL管理、Python模块导入和异常翻译
  - 因为它不能假设进程里已经存在Python解释器，所以需要显式链接`libpython.so`

因此，“`_ge_pass_native.so`在ELF中不显式依赖`libpython.so`”只说明其装载上下文不同，并不说明它天然比embed bridge更容易跨版本复用。

#### 4.4.2 ABI兼容边界：是否显式`NEEDED libpython`与是否受Python版本约束不是一回事

需要区分两个层面：

- ELF层是否显式出现`NEEDED libpythonX.Y.so`
- 该产物是否仍然绑定某个CPython minor version的C API / ABI

对当前方案而言：

- `_ge_pass_native.so`
  - 即使不在ELF中显式携带`libpython`
  - 仍然是基于当前`HI_PYTHON`对应的`Python.h` / `pybind11`头文件构建出来的extension
  - 只要没有采用`Py_LIMITED_API` / `abi3`路线，它默认仍绑定具体的CPython minor version ABI

- `libge_python_pass_bridge.so`
  - 因为采用embed路径，这种绑定会更直接地体现为显式`NEEDED libpythonX.Y.so`
  - 其运行时约束也会更早、更明确地暴露出来

所以：

- `_ge_pass_native.so`不显式`NEEDED libpython`，不等于它能安全跨多个Python minor version复用
- `libge_python_pass_bridge.so`的限制更显式、更硬，但二者在ABI意义上都属于Python版本敏感产物
- 如果未来需要扩大跨版本复用能力，方向应是评估`abi3` / limited API，而不是仅凭“extension不显式链接`libpython`”来推断兼容性

结论上，这两个产物仍应被视为同一套Python-version-sensitive artifact set，一起进入预编译与fallback管理。

#### 4.4.3 为什么pass基类继续保持纯Python，而不是也迁到native wrapper

当前方案刻意把“用户可继承的pass合同层”和“生命周期敏感的native helper层”分开：

- 用户可继承的pass合同层
  - `FusionBasePass`
  - `PatternFusionPass`
  - `DecomposePass`
  - 保持纯Python

- 生命周期敏感的native helper层
  - `PassContext`
  - `MatchResult`
  - `Pattern`
  - `PatternMatcherConfig`
  - `release_graph()`等helper
  - 收敛到`_ge_pass_native.so`

这样分层的原因是：

- pass基类本质上是用户DSL / contract
- 若把`FusionBasePass` / `PatternFusionPass`也迁到`_ge_pass_native.so`
  - 会把用户侧API一并拉入Python ABI敏感面
  - 增加import、发布和环境装配约束
  - 降低Python层协议、错误提示、类型约束和注册逻辑的可演进性

- 更重要的是，这样做并不能消除`libge_python_pass_bridge.so`
  - 因为“GE从C++ 主动调Python pass”这条embed链仍然存在
  - 也就是说，把pass基类native化只会扩大版本敏感面，而不会减少bridge的必要性

因此，本方案的正式边界仍然是：

- `libge_python_pass_bridge.so`
  - 负责C++ 视角的embed bridge

- `_ge_pass_native.so`
  - 负责Python视角的native-backed wrapper与helper

- `ge.passes.base` / `registry` / `bootstrap` / `_bridge`
  - 继续承载用户DSL、注册协议和桥接协议的纯Python部分

这也是为什么当前方案更推荐“基类纯Python + helper native化”，而不是“把pass基类也一起做成wrapper模块”。

另外，当前构建里的`pybind_options`只是一个CMake `INTERFACE`目标，用于屏蔽pybind头文件带来的部分编译告警，不承载运行时逻辑，也不是pybind的独立依赖产物。

## 5. 运行链路

### 5.1 初始化阶段

1. GE启动并触发统一入口`LoadPassPlugins()`
2. loader内部先调用legacy `CustomPassHelper::Load()`
3. loader仅通过环境变量`ASCEND_GE_PY_PASS_PATH`判断是否需要拉起Python pass发现链路；`options`不再承载`ge.py_path / py_path`这类用户pass路由参数
4. `ge_compiler.so`内部的bridge loader调用`RegisterPythonFusionBasePassesFromPlugin()`
5. bridge loader解析目标Python runtime key
6. bridge loader按“预制artifact、runtime fallback codegen”的顺序选择并装载`libge_python_pass_bridge.so`
7. bridge loader通过`GeGetPythonFusionBasePassBridgeApi()`获取bridge导出的稳定入口，并把registrar回调传给bridge
8. `libge_python_pass_bridge.so`初始化或复用Python运行时并导入`ge.passes._bridge`
9. bridge将当前进程环境中的`ASCEND_GE_PY_PASS_PATH`同步到Python `os.environ`，再调用`load_and_get_pass_descriptors()`
10. bootstrap发现并导入用户pass模块
11. 用户pass模块通过装饰器把pass注册到`ge.passes.registry`
12. bridge读取注册表并通过registrar回调把descriptor动态注册回`PassRegistry`

对应的调用关系可以简化为下面这条时序：

```text
PassPluginLoader / ge_compiler.so
  -> python_fusion_base_pass_bridge_loader.cc
  -> dlopen(libge_python_pass_bridge.so)
  -> GeGetPythonFusionBasePassBridgeApi()
  -> register_fusion_base_passes(registrar)
  -> ge.passes._bridge.load_and_get_pass_descriptors()
  -> ge.passes.bootstrap.load_pass_plugins()
  -> registrar.register_pass(pass_desc, callbacks)
  -> RegisterPythonFusionBasePass(...)
  -> PassRegistry / runtime registry
```

其中：

- `registrar`由loader构造，代表“如何把descriptor注册回compiler”
- `bridge`负责发现Python pass，并在拿到descriptor后回调`registrar`
- `RegisterPythonFusionBasePass(...)`是真正把descriptor、callbacks和creator挂回compiler侧注册中心的落点

阶段1拆分初期，`libge_python_pass_bridge.so`可以先只依赖纯Python `bootstrap / _bridge`协议完成最小联调；阶段2收口后的正式方案则要求`_ge_pass_native.so`同步到位，bridge与Python API默认基于同一套native helper运行。

### 5.2 执行阶段

1. `FusionPassExecutor`按现有流程从`PassRegistry`获取pass
2. 若pass为Python pass，则实际创建的是对应的C++ adapter实例
3. adapter在`Run`或相关阶段回调Python pass对象
4. Python pass通过`ge.graph`与`ge.es`接口读写图和构建replacement graph
5. 返回值映射为GE `Status`

## 6. Python公共接口设计

### 6.1 包结构

新增`ge.passes`包，提供以下公共接口：

- `FusionBasePass`
- `PatternFusionPass`
- `DecomposePass`
- `register_fusion_pass`
- `register_decompose_pass`
- `PassStage`
- `PassContext`
- `Pattern`
- `NodeIo`
- `MatchResult`
- `PatternMatcherConfig`
- `PatternMatcherConfigBuilder`
- `create_pattern`
- `create_replacement`
- `FuseCheckResult`
- `can_fuse`
- `report_fuse`
- `load_pass_plugins`
- `get_registered_passes`

### 6.2 方法风格

默认采用Python风格命名

### 6.3 注册接口

建议形态如下：

```python
from ge.passes import FusionBasePass, PassStage, register_fusion_pass

@register_fusion_pass(name="ConvFormatPass", stage=PassStage.BEFORE_INFER_SHAPE)
class ConvFormatPass(FusionBasePass):
    def run(self, graph, context):
        return 0
```

```python
from ge.passes import DecomposePass, PassStage, register_decompose_pass

@register_decompose_pass(
    name="DecomposeGroupedConv",
    stage=PassStage.AFTER_INFER_SHAPE,
    op_types=["Conv2D"],
)
class DecomposeGroupedConv(DecomposePass):
    def meet_requirements(self, node):
        return True

    def replacement(self, node):
        ...
```

### 6.4 Python侧返回值约定

- `run`返回`StatusLike`
- `meet_requirements`返回`bool`
- `patterns`返回`list[Pattern | Graph]`
- `replacement`返回`Graph`
- 若希望跳过当前匹配，必须在`meet_requirements`返回`False`，不支持通过`replacement`返回`None`表达“放弃替换”

其中`StatusLike`在Python层统一转换为GE `Status`。

## 7. 发现机制设计

### 7.1 统一入口

Python侧统一提供：

- `ge.passes.bootstrap.load_pass_plugins()`
- `ge.passes.bootstrap.get_registered_passes()`

bridge注册链路在每轮加载前先由C++ 按当前进程环境刷新Python `os.environ`中的`ASCEND_GE_PY_PASS_PATH`，避免常驻Python解释器中的环境缓存影响下一轮pass发现。

### 7.2 发现优先级

当前阶段优先级收敛为：

1. 环境变量`ASCEND_GE_PY_PASS_PATH`

后续阶段再补：

2. `entry_points(group="ge.passes.plugins")`

### 7.3 环境变量模式

- `ASCEND_GE_PY_PASS_PATH`支持多个目录，以`:`分隔
- 目录中允许存在单文件模块或普通Python package
- bootstrap负责将这些目录临时加入`sys.path`

### 7.4 entry_points模式（后续阶段）

- group固定为`ge.passes.plugins`
- value可指向模块路径，或返回模块的callable
- 模块import后通过装饰器完成注册

## 8. 三类pass的桥接设计

### 8.1 FusionBasePass

最直接的类型，C++ adapter调用Python对象：

- `run(graph, context)`
- 返回值约束为`None` / `bool` / `int`三类状态值
- 正式pass合同中，`context`始终为`PassContext`
- 仅`_bridge.py`的direct bridge/pytest辅助入口允许传`None`

该类型优先打通，作为整条链路的最小闭环。

### 8.2 PatternFusionPass

该类型继续复用现有C++ PatternMatcher机制。Python侧只负责：

- 提供pattern graph
- 根据`MatchResult`判断是否满足条件
- 构造replacement graph

这意味着C++ 侧需要有一个Python adapter继承`PatternFusionPass`，在以下点回调Python：

- `Patterns()`
- `MeetRequirements()`
- `Replacement()`

这里有一个明确的方案约束：

- 不要求Python用户类直接继承一个通过`PYBIND11_MODULE`暴露出来的C++ `PatternFusionPass`
- 用户继续继承纯Python的`ge.passes.base.PatternFusionPass`
- 复用C++ 基类公共`Run()`流程的职责放在`PythonPatternFusionPassAdapter`上，而不是放在Python用户类上
- Python子类禁止覆写`run()`；若误覆写，基类在类定义阶段直接抛出`TypeError`，避免“实现了但永远不会被调用”的歧义

推荐形态是：

- `PythonPatternFusionPassAdapter : public PatternFusionPass`
- adapter覆盖`Patterns()` / `MeetRequirements()` / `Replacement()`
- 覆盖函数内部再回调Python pass实例
- `Run()`直接复用现有C++ `PatternFusionPass::Run()`

选择这个方案的原因是：

- 能最大化复用现有C++ `PatternMatcher`、rewrite、统计和错误处理逻辑
- 不强迫Python用户环境必须先成功import一个native C++ 基类模块，保持`ge.passes`纯Python API的可用性
- 能让`FusionBasePass`、`PatternFusionPass`、`DecomposePass`三类pass在用户侧保持统一风格
- 能把Python版本敏感问题尽量收敛在adapter / wrapper / native bridge层，而不是扩散到用户pass基类定义层

### 8.3 DecomposePass

该类型继续复用现有`DecomposePass`语义。Python侧只负责：

- `MeetRequirements(const GNode &)`
- `Replacement(const GNode &)`

构造时需要保留`op_types`信息。

与`PatternFusionPass`一样，Python用户类不直接接管`Run()`主流程，而是只实现hook：

- `meet_requirements(node) -> bool`
- `replacement(node) -> Graph`

这里的额外合同约束需要明确写死在Python基类里：

- Python子类禁止覆写`run()`；若误覆写，基类在类定义阶段直接抛出`TypeError`
- `replacement()`必须返回replacement Graph
- 若希望跳过当前节点，必须在`meet_requirements()`阶段返回`False`，不支持通过`replacement()`返回`None`

与`PatternFusionPass`相同，这里也不要求Python用户类直接继承pybind暴露的C++ `DecomposePass`。推荐形态是：

- `PythonDecomposePassAdapter : public DecomposePass`
- adapter在C++ 侧复用`DecomposePass::Run()`
- adapter覆盖`MeetRequirements()` / `Replacement()`并转调Python pass实例

这样可以保留现有C++ `DecomposePass`的主流程语义，同时避免把构造参数、`op_types`和Python版本敏感逻辑直接暴露给Python基类继承体系。

### 8.4 creator与上下文获取设计

当前`CreateFusionPassFn`是裸函数指针：

- `using CreateFusionPassFn = FusionBasePass *(*)();`

V1不建议把它直接改成`std::function<FusionBasePass *()>`。原因有两点：

- Python pass来自bridge `.so`的动态注册，如果creator持有可捕获lambda，析构链容易和`dlclose`顺序耦合
- 一旦`PassRegistry`或其他全局对象在bridge `.so`已卸载后才析构，`std::function`内部对象析构就可能访问已卸载代码，存在`coredump`风险
这里当前不再以“独立bridge `.so`的`dlclose`风险”作为主原因。更准确的原因是：

- 现有creator ABI仍是无参裸函数指针，直接改成可捕获对象会扩大影响面
- `std::function`会把运行时路由信息、对象析构和调用路径耦合进creator本身，不利于维持“creator只做最小路由、运行时资源放在bridge/runtime registry”的分层
- 保留裸函数指针 + TLS路由上下文，仍然是当前影响面最小、最稳妥的方案

因此这里建议采用更稳妥的方案：

- 保留`CreateFusionPassFn`为裸函数指针
- 增加一个“创建期TLS上下文”
- Python pass的运行时对象和元数据放在bridge持有的进程级注册表中

这里说的“标识信息”不是Python运行时上下文本身，而是**用于注册表查找的稳定key和元数据**，例如：

- `pass_name`
- `pass_kind`，即`fusion` / `pattern` / `decompose`
- `stage`
- Python模块名
- Python类全名
- `decompose`场景下的`op_types`

这些信息属于注册期静态信息，不是执行期上下文。真正的Python解释器状态、模块对象、pass实例，不放在creator里携带，而是放在bridge的全局注册表中统一管理。

推荐实现方式如下：

1. 在消费`create_fn()`的位置设置TLS创建上下文
- 最终建议传入`descriptor_key`
- 某些现有调用点短期更容易拿到`pass_name`时，可先做过渡映射，但不建议把`pass_name`固化为最终creator路由键

2. 为三类Python pass提供少量通用creator函数
- `CreatePythonFusionPass()`
- `CreatePythonPatternPass()`
- `CreatePythonDecomposePass()`

3. 通用creator从TLS中读取当前`descriptor_key`
- 再据此从bridge注册表中找到对应descriptor
- 然后构造对应adapter

4. adapter在执行时再从bridge注册表获取Python pass实例或其holder

这个设计的优点是：

- 不需要把Python对象上下文塞进`create_fn`
- 不引入`std::function`的析构顺序风险
- 与当前GE的creator调用方式兼容性更好

### 8.5 TLS创建上下文细化

当前实现需要保留一个轻量的创建期TLS上下文。它的必要性不是“为了传更多信息”，而是因为当前`FusionPassRegistrationData::CreatePassFn`仍是无参creator，而Python pass侧多个descriptor会共用同一个adapter factory；如果没有这份TLS，上层`PassRegistry::CreatePass()`虽然知道“正在创建哪个pass”，共享factory却不知道应该绑定哪个Python descriptor。

当前代码形态可以表示为：

```cpp
struct PythonPassCreateContext {
  std::string descriptor_key;
  std::string pass_name;
  PythonPassKind kind;
};
```

并提供如下辅助设施：

- `SetCurrentPythonPassCreateContext(descriptor_key)`
- `GetCurrentPythonPassCreateContext()`
- `ClearCurrentPythonPassCreateContext()`
- RAII scope guard，例如`PythonPassCreateScope`

使用方式如下：

1. 在调用`create_fn()`前，由调用方设置当前`descriptor_key`
2. 通用creator读取TLS中的`descriptor_key`
3. 通用creator再到bridge注册表中查找对应descriptor
4. 构造对应adapter
5. `create_fn()`返回后自动清理TLS

其中：

- `descriptor_key`是真正的路由主键
- `pass_name` / `kind`当前主要承担一致性校验，避免共享factory误绑定到错误descriptor

后续如果调用链进一步统一，仍建议把`PythonPassCreateContext`收敛到“只保留`descriptor_key`这一最小必要字段”，避免状态重复，并与运行时descriptor / runtime entry的主键模型保持一致。

V1建议在以下调用点接入该scope：

- `compiler/graph/fusion/pass/fusion_pass_executor.cc`
- `FusionPassExecutor::InitPassesIfNeed`

其中：

- 首批与后续主链路都只覆盖`FusionPassExecutor`
- `graph_fusion.cc`不在本方案后续支持范围内

这样可以避免把Python化范围扩散到legacy兼容链路，同时保证creator / TLS / descriptor方案围绕主链闭环演进。

### 8.6 bridge进程级注册表细化

bridge内部建议维护两个层次的注册信息，而不是继续让一个`holder_key`同时承担“静态身份”和“运行时实例”两种语义。逻辑上可拆成两部分：

- `PythonPassDescriptor`
- 注册期静态信息

- `PythonPassInstanceHolder`
- 执行期实例信息
- Python pass实例
- 运行期状态
- 异常状态
- session / instance关联信息

其中`PythonPassDescriptor`建议至少包含：

- `pass_name`
- `pass_kind`
- `stage`
- `module_name`
- `class_qualname`
- `op_types`
- `descriptor_key`

其中：

- `descriptor_key`
- 表示“这个pass类是谁”的静态key
- 建议格式为`module_name + class_qualname + pass_name`
- 用于注册去重、descriptor查找、日志定位

- `instance_id`
- 表示“这次运行时实例是谁”的动态key
- 由adapter / session在创建时生成
- 用于holder查找、实例生命周期管理、执行期隔离

最小实现阶段曾把`holder_key`同时用于descriptor查找和holder查找。当前`FusionBasePass`已完成拆分，后续其余pass也应保持这套模型：

- `descriptor_key`
- 静态身份

- `instance_id`
- 动态实例身份

V1建议注册表只由bridge自己持有和析构，不暴露给其他全局单例持有对象，避免再次产生跨so生命周期耦合。

这里刻意不把Python pass实例做成进程级单例。原因是：

- 用户更容易自然地把`self`当成“本次pass执行的临时状态容器”
- 可以避免跨图、跨执行残留状态污染
- 可以降低多线程并发下实例共享导致的锁和重入要求

### 8.7 Python pass adapter细化

V1建议为三类pass分别提供adapter：

- `PythonFusionBasePassAdapter`
- `PythonPatternFusionPassAdapter`
- `PythonDecomposePassAdapter`

三者共同特征：

- 构造时只接收`descriptor_key`或`pass_name`
- 构造时不直接持有Python临时对象裸指针
- 构造期完成descriptor绑定，并为当前adapter创建独立`instance_id`
- adapter生命周期内独占自己的Python pass实例，不复用长期共享holder
- 执行时通过`instance_id`在bridge实例仓库中查找当前adapter对应的holder
- 析构时通过`instance_id`释放holder
- 执行时统一做GIL获取、异常转译、状态映射

这样即使adapter本身由GE长期持有，它也只依赖bridge稳定管理的holder，不依赖creator闭包对象。

三类adapter的分工还需要进一步明确：

- `PythonFusionBasePassAdapter`
- 直接覆盖`Run()`，内部调用Python `run(graph, context)`

- `PythonPatternFusionPassAdapter`
- 继承C++ `PatternFusionPass`
- 不重写基类公共`Run()`主流程
- 只覆盖`Patterns()` / `MeetRequirements()` / `Replacement()`三个hook，并在hook内部转调Python

- `PythonDecomposePassAdapter`
- 继承C++ `DecomposePass`
- 不重写基类公共`Run()`主流程
- 只覆盖`MeetRequirements()` / `Replacement()`，并在hook内部转调Python

这也是当前设计里“为什么不急着把C++ pass基类通过`PYBIND11_MODULE`暴露给Python继承”的核心原因：真正需要复用C++ 非纯虚主流程的是adapter，不是用户写的Python pass类。

### 8.8 执行期session设计

为避免对Python pass写法施加不必要限制，V1建议引入“每次执行一个session”的模型：

- 一个adapter的一次`Run`调用，对应一个`PythonPassExecutionSession`
- session内创建新的Python pass实例，并分配唯一`instance_id`
- 同一session内，多次Python回调共用同一个实例
- session结束时统一释放该实例及其临时包装对象

这意味着：

- `FusionBasePass`
- 一次`Run`对应一个Python实例

- `PatternFusionPass`
- 一次`Run`内部的`Patterns`、`MeetRequirements`、`Replacement`共用同一个Python实例

- `DecomposePass`
- 一次`Run`内部对多个匹配节点的处理共用同一个Python实例

这样设计后，Python用户可以自然地使用：

- `self.xxx`作为一次执行期间的临时缓存
- 普通Python对象作为辅助状态
- 普通异常作为失败信号

而无需理解“这个实例是不是跨图复用”这种bridge内部细节。

### 8.9 内存管理细化

V1的内存管理目标是：

- 不要求Python用户手工释放任何bridge对象
- 不要求Python用户显式使用`with`、`close()`、`release()`之类接口
- 不允许因为用户把对象存在局部变量或成员变量里就触发双释放或悬挂指针

建议按三层对象分别处理。

#### 8.9.1 注册期对象

注册期对象包括：

- descriptor
- Python模块对象
- Python类对象
- descriptor注册表

这些对象由bridge注册表统一持有，桥接层负责引用计数和清理。对Python用户透明。

#### 8.9.2 执行期对象

执行期对象包括：

- instance holder
- Python pass实例
- callback中创建的`Graph` / `Node` / `MatchResult` / `NodeIo` Python包装对象
- 可能的临时`TensorDesc` / `Shape` / `Tensor`包装对象
- `instance_id`

这些对象都绑定到execution session，而不是绑定到全局descriptor。

session结束时：

- Python pass实例释放
- 执行期包装缓存释放
- 执行期有效性token失效

#### 8.9.3 borrowed图对象

`Graph`、`Node`、`Tensor`等对象很多是对GE当前执行图的借用视图。为保证Python体验不变差，建议：

- Python包装对象内部持有一个执行期owner token
- 在owner token有效时，所有访问都正常工作
- 一旦用户把对象跨session保存并再次访问，不允许崩溃，而是抛出明确的Python异常，例如：
- `RuntimeError: graph handle has expired`

这样做的效果是：

- 不要求文档里给用户增加“不要缓存这些对象”的硬限制
- 即使用户这么写，也应得到可理解的Python错误，而不是coredump

#### 8.9.4 TensorDesc / Shape的值语义

`TensorDesc`、`Shape`这类对象建议按值语义暴露给Python：

- Python获得的是独立对象
- 可安全保存在局部变量或`self`上
- 不依赖原始borrowed graph句柄继续存活

这样更符合Python用户预期，也能减少悬挂引用问题。

#### 8.9.5 bridge卸载与析构顺序

GE提供两级卸载语义，分别对应"一轮业务结束"和"进程退出"两个生命周期：

##### Unload — 业务级卸载

一轮图编译完成后，GE调用`UnloadPassPlugins()`清理本轮pass注册态，但不关闭Python解释器也不卸载bridge so。这样下一轮业务可以复用已初始化的Python运行时，避免反复初始化/终结带来的开销。

当前实现链路：

```
UnloadPassPlugins()
  → PassPluginLoader::Unload()                              [pass_plugin_loader.cc]
    ├─ UnloadPythonFusionBasePasses()                        // 仅在 python_pass_loaded_ 为 true 时执行
    │   → BridgeLoader::Unload()                             [bridge_loader.cc]
    │     ├─ api_->reset_bridge_state()                      // 通知 bridge 清理 Python 侧状态并释放 bridge 模块引用
    │     ├─ ClearPythonFusionBasePassRuntimeRegistry()      // 清理 C++ 侧 runtime 注册表
    │     └─ PassRegistry::ClearPythonPasses()               // 清理 C++ 侧 pass 注册表
    │   python_pass_loaded_ = false
    └─ CustomPassHelper::Unload()                            // 清理 C++ 自定义 pass
```

Unload不触及Python解释器生命周期和bridge so句柄，确保下一轮`Load()`可以直接复用。

##### 进程级关闭

进程退出时，GE通过`UnloadPassPlugins()`在引用计数归零时执行完整的资源释放。当前所有入口统一调用`UnloadPassPlugins()`：

- `GEFinalizeV2()` — 在线模式进程结束时
- `aclgrphBuildFinalize()` — 离线编译结束时
- `GeGenerator::Finalize()` — 生成器模式结束时
- `atc main_impl` — ATC结束时

`UnloadPassPlugins()`内部在引用计数归零时执行进程级清理：

```
UnloadPassPlugins()
  → PassPluginLoader::Unload()                               [pass_plugin_loader.cc]
    ├─ active_users_--
    ├─ if (active_users_ == 0):
    │   ├─ if (python_pass_loaded_):
    │   │   UnloadPythonFusionBasePasses()                   // 先清理注册态
    │   │     → BridgeLoader::Unload()
    │   │       ├─ api_->reset_bridge_state()
    │   │       ├─ ClearPythonFusionBasePassRuntimeRegistry()
    │   │       └─ PassRegistry::ClearPythonPasses()
    │   │   python_pass_loaded_ = false
    │   │
    │   ├─ if (cpp_pass_loaded_):
    │   │   cpp_pass_loaded_ = false
    │   │   CustomPassHelper::Unload()                        // 清理 C++ 自定义 pass
    │   │
    │   └─ if (!shutdown_done_):
    │       shutdown_done_ = true
    │       ShutdownPythonFusionBasePassesForProcess()        // 进程级 Python bridge 清理
    │         → BridgeLoader::ShutdownForProcess()            [bridge_loader.cc]
    │           ├─ if (api_ != nullptr):
    │           │   api_->shutdown_bridge()
    │           │     → PybindBridge::Shutdown()              [pybind_bridge.cc]
    │           │       ├─ ResetBridgeStateUnlocked()
    │           │       └─ if (owns_interpreter_):
    │           │           py::finalize_interpreter()
    │           │     owns_interpreter_ = false
    │           ├─ api_ = nullptr
    │           ├─ if (handle_ != nullptr):
    │           │   dlclose(handle_)
    │           │   handle_ = nullptr
    │           └─ loaded_path_.clear()
```

##### 幂等性保证

由于`UnloadPassPlugins()`可能从多个入口被重复调用，整条链路通过以下守卫保证幂等：

1. **PassPluginLoader层** — `active_users_`引用计数：每次调用递减，归零时才执行卸载；`shutdown_done_`标志确保进程级清理只执行一次
2. **BridgeLoader层** — `api_` / `handle_`空指针守卫：首次执行后置为`nullptr`，后续调用跳过shutdown和dlclose
3. **PybindBridge层** — `Py_IsInitialized()`守卫：解释器已终结后不再进入Python清理逻辑；`owns_interpreter_`守卫确保只终结自己初始化的解释器

##### 卸载顺序的核心约束

当前实现遵循以下顺序原则：

1. **先清理C++ 注册表，再dlclose bridge so** — `UnloadPythonFusionBasePasses()`先清理`PassRegistry`和`PythonFusionBasePassRuntimeRegistry`，之后才执行`ShutdownForProcess()`进行`dlclose`。这保证dlclose时没有任何C++ 对象仍在持有bridge侧的回调函数指针。
2. **先清理Python对象，再终结解释器** — `PybindBridge::Shutdown()`先调用`ResetBridgeStateUnlocked()`清理Python侧注册表、holder和动态加载的pass模块，并在reset内释放`bridge_module_`引用、调用`gc.collect()`打破循环引用，最后才调用`py::finalize_interpreter()`。
3. **先终结解释器，再dlclose so** — `shutdown_bridge()`在`BridgeLoader::ShutdownForProcess()`中先于`dlclose(handle_)`执行，保证dlclose时Python解释器已不再运行。
4. **如果解释器已被外部终结** — `Py_IsInitialized()`返回0，bridge跳过所有Python清理逻辑，仅清理C++ 侧状态，不会对已释放的Python对象做`DECREF`。

这里优先保证"不崩溃"，而不是极限回收所有尾声内存。CPython的内部arena分配器在`Py_Finalize()`后可能仍有残余内存不被回收，这是CPython的已知行为，不影响进程正常退出。

### 8.10 锁与GIL策略细化

锁和GIL的设计目标是：

- 不把锁概念暴露给Python用户
- 不要求Python pass作者自己理解或管理GIL
- 在bridge内部把锁粒度控制到最小，避免把整个GE pass执行路径串行化

建议分三类锁。

#### 8.10.1 bridge管理锁

用于保护：

- 注册表初始化
- 插件发现
- holder懒加载
- unload / finalize状态切换

这类锁只包围bridge自己的状态管理，不包围用户pass逻辑执行。

#### 8.10.2 execution session锁

每个execution session可有自己的轻量状态保护，但不建议让多个session共享粗粒度互斥锁。

目标是允许：

- 不同pass的不同执行互不阻塞
- 非Python的纯C++ 匹配/改图逻辑继续按原有路径运行

#### 8.10.3 Python GIL

统一规则如下：

- 进入Python前获取GIL
- 离开Python后立即释放GIL
- 纯C++ 图匹配、图遍历、数据整理逻辑不持有GIL

对三类pass的具体策略：

- `FusionBasePass`
- 回调`run`时持有GIL
- Python返回后立刻释放GIL

- `PatternFusionPass`
- C++ pattern匹配过程不持有GIL
- 调`Patterns`、`MeetRequirements`、`Replacement`时短时持有GIL

- `DecomposePass`
- C++ 搜索匹配节点时不持有GIL
- 调`meet_requirements`、`replacement`时短时持有GIL

这样可以把Python执行和C++ 执行边界清楚分开，减少不必要的全局串行化。

#### 8.10.4 回调重入策略

V1建议默认支持“多session并发、单session内串行”：

- 一个execution session内部不做并发Python回调
- 不同session之间如果由GE上层并发触发，则通过GIL自然串行进入Python

这对Python用户的含义是：

- 不需要为了bridge额外给pass写锁
- 如果用户自己使用模块级全局可变状态，仍需自行保证逻辑正确

bridge不额外限制用户这么写，但也不为用户自己的全局共享状态提供自动事务语义。

### 8.11 Pythonic体验约束

V1的设计原则是“把生命周期和并发复杂度收敛在bridge内部”，尽量不给Python用户增加非Pythonic规则。具体要求如下：

- 不要求用户手工管理内存
- 不要求用户手工管理锁或GIL
- 不要求用户通过特定上下文管理器才能写pass
- 不要求用户为了避免复用问题而人为拆散普通Python写法

在可做到的范围内，Python用户应当能按普通类来写：

- 用构造函数初始化固定配置
- 用`self`保存一次执行内的临时状态
- 用普通Python异常表示错误
- 用普通返回值表示结果

需要如实说明的边界只有两类：

- 注册协议边界
- 用户仍需通过装饰器或等价注册接口声明pass，这属于框架接入协议，不属于非Pythonic限制

- 过期对象边界
- 如果用户跨执行长期保存borrowed graph视图对象，后续再次访问时会得到Python异常，而不是被静默支持到无限期

这两类边界是框架接入所必需的，但不应把用户逼到“必须按非Pythonic模式写代码”。

### 8.12 `REGISTER_CUSTOM_PASS`后续支持设计

`REGISTER_CUSTOM_PASS`需要支持，但建议放在三类`PassRegistry` pass稳定之后的扩展阶段实施。原因是：

- 其执行路径与`FusionPassExecutor`体系不同，当前更多走`CustomPassHelper` / legacy custom pass链路
- 首批优先打通三类pass，能更快把descriptor、session、bridge、holder、GIL、异常隔离这些公共底座做稳定
- 等底座稳定后，再接`REGISTER_CUSTOM_PASS`，可以显著提高代码复用度，避免再做第二套Python bridge

推荐复用方式如下：

- 继续复用同一套Python发现机制
  - `ge.passes.bootstrap`
  - 当前阶段以环境变量为主路径，后续再补`entry_points`

- 继续复用同一套Python注册表与descriptor机制
  - 在`PythonPassKind`中新增`legacy_custom`
  - descriptor新增legacy custom pass所需元数据

- 继续复用同一套pybind bridge
  - 不另起第二套Python runtime初始化
  - 不另起第二套holder / session管理

- 继续复用“静态`descriptor_key` + 动态`instance_id`”模型
  - 避免legacy custom pass再次引入共享Python实例导致的状态限制

- 在C++ 侧新增`PythonLegacyCustomPassAdapter`
  - 适配`REGISTER_CUSTOM_PASS`所需的接口与执行入口
  - 仅在最外层接口适配上与三类pass区分

换句话说，`REGISTER_CUSTOM_PASS`的后续支持应该是：

- 复用同一套`bootstrap`
- 复用同一套`registry`
- 复用同一套`pybind bridge`
- 复用同一套`session / holder / instance_id`
- 仅新增一层legacy custom path的adapter和少量descriptor字段

这能保证后续扩展不需要再造一套平行体系。

### 8.13 PatternFusionPass桥接协议

本节定义C++ pybind bridge与`_bridge.py`之间用于`PatternFusionPass`的跨语言调用协议。

#### 8.13.1 协议函数

`_bridge.py`需实现以下三个函数，供`libge_python_pass_bridge.so`在embed模式下回调：

1. **`get_pass_patterns(instance_id: str) -> list`**
   - 由C++ 侧`PythonPatternFusionPassAdapter::Patterns()`通过bridge回调
   - `_bridge.py`调用Python pass实例的`patterns()`方法
   - 返回Pattern对象列表，每个Pattern由`_ge_pass_native.so`构造

2. **`call_meet_requirements(instance_id: str, match_result_handle: int) -> bool`**
   - 由C++ 侧`PythonPatternFusionPassAdapter::MeetRequirements()`通过bridge回调
   - `match_result_handle`为C++ `MatchResult*`的`uintptr_t`表示
   - `_bridge.py`通过`_ge_pass_native.so`将其还原为borrowed MatchResult wrapper
   - 调用Python pass实例的`meet_requirements()`方法
   - 返回是否满足条件

3. **`call_replacement(instance_id: str, match_result_handle: int) -> int`**
   - 由C++ 侧`PythonPatternFusionPassAdapter::Replacement()`通过bridge回调
   - `match_result_handle`同上
   - 调用Python pass实例的`replacement()`方法
   - 要求Python pass必须返回replacement Graph
   - 若当前匹配不应继续，必须在`meet_requirements()`阶段返回`False`
   - `_bridge.py`负责校验返回值类型，并将Graph所有权转交给C++

#### 8.13.2 所有权与生命周期约定

##### Pattern所有权转移（get_pass_patterns）

- Python侧通过`_ge_pass_native.so`构造Pattern对象
- Pattern为`unique_ptr`语义，函数返回前Python侧必须调用`release()`释放所有权
- C++ 侧通过`unique_ptr<Pattern>`接管裸指针，负责后续析构
- **约束**：Python侧在函数返回后不得继续持有Pattern引用

##### MatchResult借用语义（call_meet_requirements / call_replacement）

- `MatchResult`所有权始终在C++ 侧（由`PatternFusionPass::Run()`持有）
- Python侧获得的MatchResult为borrowed视图，不拥有所有权
- **约束**：Python侧不得在回调返回后继续持有或缓存MatchResult引用
- native binding应提供borrowed wrapper，并在过期访问时抛出`RuntimeError`而非崩溃
- `uintptr_t`传递为过渡方案，待MatchResult native binding就绪后替换为类型安全的方式

##### Replacement Graph所有权转移（call_replacement）

- Python侧构造replacement Graph
- Graph所有权通过`release()`转移给C++ 侧（`GraphUniqPtr` / `unique_ptr<Graph>`）
- **约束**：Python侧在函数返回后不得继续持有该Graph引用

#### 8.13.3 DecomposePass桥接协议

`DecomposePass`复用C++ `DecomposePass::Run()`的匹配与替换主流程，因此`_bridge.py`只需补两段按节点回调的协议函数：

1. **`call_decompose_meet_requirements(instance_id: str, node_handle: int) -> bool`**
   - 由C++ `PythonDecomposePassAdapter::MeetRequirements()`通过bridge回调
   - `node_handle`为当前`GNode*`的`uintptr_t`表示
   - `_bridge.py`通过`_ge_pass_native.so`的`borrow_node()` helper把它还原为短生命周期Python `Node`视图
   - 调用Python pass实例的`meet_requirements()`方法

2. **`call_decompose_replacement(instance_id: str, node_handle: int) -> int`**
   - 由C++ `PythonDecomposePassAdapter::Replacement()`通过bridge回调
   - `_bridge.py`调用Python pass实例的`replacement()`方法
   - 要求Python pass必须返回replacement Graph
   - 若当前节点不应继续，必须在`meet_requirements()`阶段返回`False`
   - `_bridge.py`负责校验返回值类型，并通过`release_graph()`把所有权转交给C++

3. **`_bridge.py`内部实例分派采用明确基类，而不是`Any`**
   - holder中保存的实例类型收敛为`FusionBasePass`
   - `PatternFusionPass` / `DecomposePass`协议入口在调用前分别做`isinstance`收敛
   - `replacement()`的bridge参数与返回值按正式`Graph` / `Node`接口处理，不再以无约束`Any`传递

##### Node借用语义（call_decompose_meet_requirements / call_decompose_replacement）

- `DecomposePass::Run()`在C++ 侧枚举匹配节点
- Python侧看到的`Node`是由`_ge_pass_native.so`基于当前`GNode`构造的短生命周期视图
- Python侧不应跨回调缓存该`Node`
- 因为该视图本质上仍指向真实图节点，所以读取名称、类型、属性和张量描述等信息时能保持与当前图一致

建议把legacy custom pass的接入拆成两层：

- 发现与注册层
  - 继续复用`ge.passes.bootstrap`、`ge.passes.registry`、`ge.passes._bridge`
  - Python侧只需新增`legacy_custom` descriptor字段与装饰器/注册入口

- 执行适配层
  - 在现有`CustomPassHelper` / legacy custom pass链路上增加`PythonLegacyCustomPassAdapter`
  - 该adapter继续复用同一套pybind bridge、同一套`descriptor_key -> instance_id`生命周期协议

这样做的直接收益是：

- Python pass的发现、模块加载、异常翻译、GIL、session、holder回收只维护一套实现
- legacy custom pass只新增最外层接口适配，而不复制Python runtime托管逻辑
- 后续若要同时支持`PassRegistry` pass与`REGISTER_CUSTOM_PASS`，两边仍然共享同一份Python用户开发体验

## 9. Python图接口补齐设计

### 9.1 必补能力

为支持Python改写现有`examples/fusion_pass`，至少补齐：

- `Graph` borrowed/non-owning句柄模式
- `Node.get_input_desc`
- `Node.get_output_desc`
- `Node.update_input_desc`
- `Node.update_output_desc`
- `Node.get_input_const_tensor`
- `Shape`
- `TensorDesc`
- `GeUtils.InferShape`
- `GeUtils.CheckNodeSupportOnAicore`

### 9.2 borrowed handle

运行时从C++ 回传给Python的`Graph`、`Node`、`Tensor`很多都不应由Python析构。当前`Graph._create_from(handle, owns_handle, owner)`已支持borrowed / non-owning形态，bridge在执行期通过：

- `_create_from(handle, owns_handle=False, owner=...)`

把runtime `GraphPtr`映射为正式`ge.graph.Graph`视图，并用`owner`持有创建期token，避免Python wrapper提前析构后把底层runtime graph误释放。

## 10. 打包与发布设计

### 10.1 产物

当前仓负责构建：

- `ge_py`主wheel
- 多版本native子wheel
- bridge artifact set与其装载逻辑

这里的bridge artifact set指：

- `libge_python_pass_bridge.so`
- 由GE内部loader `dlopen`的私有bridge so
- 负责embed路径、解释器管理、GIL、Python回调、异常翻译

- `_ge_pass_native.so`
- 由Python import的helper extension
- 负责`Graph` / `PassContext` / `MatchResult`等native-backed wrapper与helper
- 不承载用户可继承的pass基类

### 10.2 版本策略

正式支持矩阵：

- `cp37`
- `cp38`
- `cp39`
- `cp310`
- `cp311`
- `cp312`
- `cp313`
- `cp314`

这些子wheel承载的是Python pass bridge / adapter相关的`pybind11`扩展，而不是替换现有`ge.graph`的全部Python封装。

发布链路需要覆盖上述Python版本的构建能力，但这不等价于“单机必须同时安装所有Python版本”。推荐按多版本构建矩阵组织：

- 每个构建job/container只负责一个Python minor版本
- 每个job产出对应tag的native子wheel
- 主wheel仍保持单份构建

换句话说，要求的是“整体发布流水线覆盖`cp37-cp314`”，而不是“每台开发机或构建机必须同时具备全部版本”。

当前实现状态：

- native子wheel的ABI tag取自目标解释器的`SOABI`，3.8及以后与Python tag相同，3.7为`cp37m`；artifact set目录名仍只用Python tag（`cp37-linux-aarch64`）
- `build_python_native_matrix.py`内置支持版本集合`cp37/cp38/cp39/cp310/cp311/cp312/cp313/cp314`；CMake为pass/runtime/custom-op分别生成组件配置并通过`--component-config`传入target、源码、artifact、wheel元数据、ABI和Python链接策略，`--tag`仅用于手工调用指定构建版本
- `ge_python`正式target产出主wheel，并触发`ge_python_pass_native_wheel_matrix`尽力构建可用Python minor版本的native子wheel
- `ge_python_native_wheel`是单版本开发target，只输出当前`HI_PYTHON`对应的artifact set和`ge_py_pass_bridge` native子wheel
- 仓内在`python_pass_native_build`构建工具目录下提供通用的`build_python_native_matrix.py`、`build_python_native_wheel.py`与`ge_python_pass_native_wheel_matrix`目标；构建工具只读取CMake生成的`--component-config`，用于在CI/本地环境中自动嗅探可用的`python3.7`到`python3.14`。matrix不重新配置整仓CMake，而是复用父构建已生成的compile/link元数据，仅替换Python include/lib后重编`libge_python_pass_bridge.so`与`_ge_pass_native.so`
- 解释器发现顺序为：显式传入的`--python`、`PATH`中的`python3.7`到`python3.14`/`python3`/`python`、Conda环境中的`bin/python`；Conda环境优先通过`conda env list --json`获取，失败时再扫描`CONDA_PREFIX`/`CONDA_EXE`推导出的`envs`目录以及常见`~/miniconda3/envs`、`~/anaconda3/envs`、`~/.conda/envs`
- matrix构建依赖父构建先完成当前`HI_PYTHON`的`ge_python_native_wheel`，并复用父构建的编译器、编译选项、include/link元数据、已构建GE依赖库和`CMAKE_CXX_COMPILER_LAUNCHER`；该流程不再重新进入`cmake/package.cmake`或重新计算`BUILD_COMPONENT`
- 主wheel只装配纯Python代码，不再内置当前构建Python的默认native artifact set
- native子wheel通过标准`bdist_wheel`生成，承载`ge/passes/python_pass_artifacts/<python_tag>-<platform>`下的一套bridge/native artifact set
- ge-compiler run包中继续安装原有`ge_py-0.0.1-py3-none-any.whl`，并额外把matrix输出目录中的`ge_py_pass_bridge-*.whl`安装到`ge-compiler/lib64`
- `ge_python_pass_fallback_codegen`目标通过`gen_fallback_resources.py`生成`ge/passes/fallback_codegen/`资源，包括`build_config.json`和可临时展开`src/{bridge,native}`、`include/{bridge,native}`的Python资源模块
- 运行时选择优先级为“预制artifact set > runtime fallback codegen”

run包安装脚本需要注意：run包可以携带多个`ge_py_pass_bridge` native子wheel，但单次安装只应安装一个与“执行安装脚本的Python解释器”兼容的子wheel。推荐让pip基于wheel tag自动选择：

```bash
PYTHON_BIN=${PYTHON_BIN:-python3}
LIB_DIR=<run-package>/ge-compiler/lib64

"${PYTHON_BIN}" -m pip install \
  --no-index \
  --find-links "${LIB_DIR}" \
  "${LIB_DIR}/ge_py-0.0.1-py3-none-any.whl" \
  ge-py-pass-bridge
```

该方式下，`ge_py`主wheel通过文件路径明确安装，`ge-py-pass-bridge`通过`--find-links`在同一目录中选择当前Python tag与平台tag匹配的native子wheel。例如Python 3.11只会选择`cp311-cp311` wheel，Python 3.7只会选择`cp37-cp37m` wheel。安装脚本不应把`cp37-cp314`的所有native wheel文件路径一次性传给pip，否则不兼容当前解释器的wheel会被判定为不可安装。

#### 10.2.1 artifact set manifest与选择机制

Python pass bridge artifact set采用固定目录布局：

```text
ge/passes/python_pass_artifacts/<python_tag>-<platform>/manifest.json
ge/passes/python_pass_artifacts/<python_tag>-<platform>/libge_python_pass_bridge.so
ge/passes/python_pass_artifacts/<python_tag>-<platform>/_ge_pass_native.so
```

当前manifest字段如下：

```json
{
  "python_tag": "cp311",
  "platform": "linux-x86_64",
  "bridge_abi": 2,
  "artifacts": {
    "bridge": "libge_python_pass_bridge.so",
    "native": "_ge_pass_native.so"
  }
}
```

字段含义：

| 字段 | 含义 | 匹配方式 |
|------|------|----------|
| `python_tag` | artifact绑定的CPython minor version tag，例如`cp39`、`cp310`、`cp311`、`cp312` | 必须与当前进程内目标Python runtime key一致 |
| `platform` | artifact绑定的平台tag，当前由系统名和机器架构组成，例如`linux-x86_64` | 必须与当前运行平台一致 |
| `bridge_abi` | `ge_compiler.so`内loader与`libge_python_pass_bridge.so`之间的C ABI协议版本 | 必须与`kPythonFusionPassBridgeAbiVersion`一致 |
| `artifacts.bridge` | 相对manifest所在目录的bridge `.so`路径 | 必须能解析到真实文件，作为`dlopen`目标 |
| `artifacts.native` | 相对manifest所在目录的`_ge_pass_native.so`路径 | 必须能解析到真实文件，并通过`set_artifact_config`传给bridge，保证Python import使用同源native |


选择流程：

1. loader先解析当前进程目标Python runtime key。若当前进程已加载CPython C API符号，则通过`Py_GetVersion()`得到`cpXY`；若进程内尚无可见CPython符号，则按内部原生入口场景探测`PATH`中的`python3`、`python`。
2. loader根据自身`.so`路径推导`ge`包目录，并扫描`ge/passes/python_pass_artifacts`下的manifest。
3. manifest先做JSON结构和artifact文件存在性校验，再按`python_tag`、`platform`、`bridge_abi`过滤。
4. 命中的artifact set优先进入候选列表。
5. 若没有命中或候选加载失败，触发runtime fallback codegen，调用`ge.passes.fallback_runtime.run_fallback_codegen()`；若fallback产物仍不可用，loader直接返回失败。
6. bridge `dlopen`成功后，loader再读取当前进程实际Python runtime key，与加载前的目标key做一致性校验，避免同一进程中误拉起另一套CPython minor version。

`kPythonFusionPassBridgeAbiVersion`只描述loader与bridge C API的协议版本，例如函数表字段、函数语义或调用时序出现不兼容变化时才需要滚动。本次资源规划pass registrar扩展新增统一回调字段，故当前协议版本为`2`；后续只有不兼容的函数表或调用语义变化才继续滚动版本。

#### 10.2.2 配套升级与回退边界

ge-compiler和ge-executor不承诺独立升级。Python主包、native wheel、bridge及其依赖的GE库应使用同一配套版本；升级或回退时应同步替换相关组件，并结束使用旧组件的进程。不要通过单独替换wheel或共享库构造混装组合。

bridge ABI版本只校验loader与bridge的协议，不代表底层GE库兼容。ABI 1与ABI 2组件不支持混用；fallback仅构建当前包对应的实现，不能为旧GE库补充新增符号。ABI 1的历史交付状态需由发布记录确认，不能仅凭协议版本号推断兼容承诺。

### 10.3 安装策略

V1不做安装时codegen，也不规划单独的`prepare`预编译命令。原因包括：

- 标准wheel缺少稳定、通用的post-install编译机制
- 安装时的Python环境不一定就是最终运行时环境
- 安装机不一定具备编译器和开发头文件
- 一旦安装期自动编译失败，可能把纯Python部分的可用性也一起拖垮

因此建议采用两层策略：

1. 主流版本走预编译native子wheel
2. 非覆盖版本走运行时fallback codegen兜底

也就是说，安装阶段只负责把主wheel和可用的native子wheel安装到位；若当前Python版本没有命中预编译产物，再由运行时按需触发fallback。

对于run包中的packaged default路径，还需要满足以下固定约束：

- `libge_python_pass_bridge.so`随native artifact wheel放置到`ge/passes/python_pass_artifacts/<python_tag>-<platform>/`
- GE内部loader通过manifest解析artifact set中的bridge/native路径
- `_ge_pass_native.so`与`libge_python_pass_bridge.so`遵循同一套artifact set装配约束

### 10.4 fallback

当当前Python版本没有命中预编译子wheel时：

- 主wheel直接进入runtime fallback流程
- 在本地缓存目录生成并编译对应版本的bridge artifact set
- 成功则启用Python pass
- 失败则禁用Python pass，但不影响原有C++ pass流程

这里需要特别约束fallback的边界：

- fallback codegen不生成用户pass代码
- fallback codegen不改写`ge_compiler.so`
- fallback codegen的目标是生成可替换的bridge artifact set，而不是局部patch
- fallback产物需要同时覆盖`libge_python_pass_bridge.so`与`_ge_pass_native.so`
- fallback产物承载完整的Python版本敏感bridge逻辑，并复用与`ge_compiler.so`约定好的稳定协议

### 10.5 本地验证约束

当前开发环境中Python为`3.13`，而正式矩阵为`cp37-cp314`。因此本地验证只能覆盖当前环境可用的Python tag；完整矩阵需要CI或发布流水线在具备对应Python版本的环境中分别构建。

### 10.6 pybind模块边界

V1建议把pybind侧内容控制在“pass bridge必需能力”范围，不扩大到整个`ge.graph`。建议拆成以下边界：

- 主wheel中的纯Python代码
- `ge.passes.base`
- `ge.passes.registry`
- `ge.passes.bootstrap`
- `ge.passes.fallback_runtime`
- `ge.passes._bridge`

- `ge_compiler.so`内部稳定核心
- `PassPluginLoader`
- `PassRegistry`注册与运行期路由
- `descriptor_key + instance_id`生命周期协议
- adapter回调、异常翻译、错误码映射
- 与独立内部bridge `.so`交互的稳定接口

- 预编译 / fallback生成的独立内部bridge `.so`
- Python版本敏感的`Python.h` / `pybind11`绑定代码
- Python解释器初始化、GIL管理、模块导入、异常转译
- 调用`ge.passes._bridge`的统一运行路径

- 预编译 / fallback生成的`_ge_pass_native.so`
- `Graph` / `PassContext` / `MatchResult` / `NodeIo`等正式Python wrapper的构造与转换
- Python侧直接import的helper与工厂接口
- 对`ge.passes.base` / `ge.passes.pattern`提供正式的native-backed对象来源

换句话说：

- `libge_python_pass_bridge.so`的职责是“支撑Python pass接入GE”
- `_ge_pass_native.so`的职责是“为Python pass暴露native-backed对象和helper”
- 二者都不是“重写现有Python图API”

不同Python版本之间的差异，原则上不体现在维护多份不同语义的`.cc`文件，而主要体现在：

- 使用同一套模板或同一套源码
- 编译时的Python include / libpython / extension suffix / rpath等构建参数不同
- 个别兼容宏、条件编译或生成元数据不同

也就是说，后续不建议按`cp37/cp38/cp39/cp310/cp311/cp312/cp313/cp314`长期维护多份手写bridge源码分支。

### 10.7 pybind子wheel组织建议

建议采用“主wheel + 内部bridge native wheel”的组织方式：

- 主wheel包名
- 保持`ge_py`

- native bridge wheel
- 以单独包名承载，例如逻辑上可命名为`ge_py_pass_bridge`
- wheel tag对应`cp37-cp314`
- 采用标准`bdist_wheel`生成，避免手工拼装wheel metadata / RECORD / tag

主wheel中的`ge.passes.fallback_runtime`负责：

1. 识别当前Python版本
2. 解析并装载匹配的bridge产物元数据
3. 若未命中预编译模块，则进入fallback codegen
4. 将最终bridge产物路径与入口信息提供给GE内部loader
5. GE内部loader `dlopen` `libge_python_pass_bridge.so`并建立回调入口
6. `libge_python_pass_bridge.so`再从同目录加载`_ge_pass_native.so`

这样主wheel逻辑固定，native wheel只负责不同Python版本下的bridge实现。

这里要进一步明确一条兼容原则：

- 运行时最终选中的bridge产物，应以“预编译产物 > fallback产物”的优先级覆盖
- 不能要求在`ge_compiler.so`链接期就决定要用哪一份生成产物
- 更合理的方式是先由Python侧或配置侧解析出目标bridge产物，再由GE内部loader `dlopen`
- 因此生成产物需要通过稳定模块名或稳定C ABI被动态加载，而不是让run包内置`.so`成为唯一硬链接目标

从扩展性角度看，bridge不应继续编入`ge_compiler.so`。当前若存在这类实现，只能视为临时bring-up偏差，而不是正式架构。

### 10.8 fallback codegen边界

fallback codegen建议直接生成整套bridge artifact set，而不是只生成某个局部wrapper helper。输入固定为：

- 当前Python版本
- `libge_python_pass_bridge.so` / `_ge_pass_native.so`的统一源码或模板
- 当前仓导出的稳定头文件
- 当前Python环境对应的include / libpython / extension suffix等构建参数
- 当前run包暴露的稳定链接依赖

输出固定为：

- `ge/passes/python_pass_artifacts/<python_tag>-<platform>/libge_python_pass_bridge.so`
- `ge/passes/python_pass_artifacts/<python_tag>-<platform>/_ge_pass_native.so`
- 对应manifest / 元数据文件

推荐的链接与装载关系如下：

1. `ge_compiler.so`不在链接期依赖某个fallback生成物
2. 运行时解析出最终要使用的bridge产物路径
3. GE内部loader `dlopen`该bridge `.so`
4. bridge `.so`再通过稳定ABI与`ge_compiler.so`交互

这样才能保证“生成出来的`.so`优先生效”，而不是永远被run包里预置的版本覆盖。

### 10.9 当前工程与后续codegen的兼容策略

当前工程已经把`python_fusion_base_pass_pybind_bridge.cc`从`ge_compiler.so`中拆出，第一批改为由`python_fusion_base_pass_bridge_loader.cc`负责运行时装载`libge_python_pass_bridge.so`。为了兼容后续codegen演进，后续实现仍需持续遵守以下边界：

- 不把Python版本敏感逻辑固化进run包自带的`ge_compiler.so`
- `ge_compiler.so`只保留稳定loader、注册表、adapter协议和最小交互面
- 独立bridge `.so`承担完整的Python敏感逻辑，并成为预编译 / fallback的统一替换目标
- `descriptor_key + instance_id`、adapter回调协议、错误码翻译、holder生命周期等核心语义保持在compiler侧稳定

换句话说，若目标是产品化支持后续扩展，那么独立bridge `.so`不应作为“后面再挪”的优化项，而应作为一开始的正式架构边界。

### 10.10 Python解释器来源与fallback选择约束

Python pass bridge不能假设“进程里只有一种Python拉起方式”。后续做多版本预编译与fallback codegen时，必须把解释器来源、artifact版本选择和finalize顺序作为同一个生命周期问题处理。

当前需要覆盖以下典型场景：

| 场景 | 当前进程 | Python解释器状态 | bridge行为 | fallback / codegen约束 |
|------|----------|------------------|-------------|--------------------------|
| 外部Python launcher，例如当前实际的`python -m ge.pyatc` | bridge所在进程仍是`atc.bin` | 外层Python只负责拉起`atc.bin`，不等价于`atc.bin`内已有解释器；实际日志显示TBE先在`atc.bin`内初始化Python | bridge在`atc.bin`内看到`Py_IsInitialized()`为true后复用TBE解释器，`owns_interpreter=false` | artifact选择必须以`atc.bin`当前进程内解释器版本为准，不能把launcher Python或TBE worker Python直接当作bridge版本依据 |
| Python进程内通过`ctypes` / C API直接调用GE/ATC入口 | 当前Python主进程 | 进入GE前解释器已经初始化 | TBE与bridge都应看到`Py_IsInitialized()==1`，并复用当前解释器，均不拥有解释器 | artifact选择必须以当前Python进程的`sys.version_info`为唯一权威版本；`PATH`中的`python3`不参与该场景的版本选择 |
| 内部原生入口，例如`atc.bin`，且Python pass最先用Python | `atc.bin` | bridge进入前解释器未初始化 | bridge可调用`py::initialize_interpreter()`，并记录`owns_interpreter=true` | artifact选择必须在初始化前基于明确的目标Python环境完成，例如配置的Python executable或统一runtime selector；不能隐式随机使用不同来源的`python3` |
| `atc.bin`中TBE先初始化Python | `atc.bin` | TBE的`TbeInitialize -> PythonAdapterManager::Initialize -> py_decouple.cc`已初始化解释器 | bridge必须复用现有解释器，`owns_interpreter=false` | 预编译或fallback artifact必须匹配TBE已经拉起的进程内Python minor version；若已选artifact与进程内版本不一致，必须报错禁用Python pass，不能再拉第二个解释器 |
| TBE并行编译worker，例如日志中的`TBE(pid,python3)` | 独立`python3`子进程 | 只影响worker自己的地址空间 | 与`atc.bin`内的bridge无共享解释器关系 | 不能把worker进程的Python版本作为bridge artifact选择依据；bridge只看当前进程 |

#### 10.10.1 外部Python launcher实际流程

当前`python -m ge.pyatc`这类外部拉起场景，如果实现方式是`subprocess` / `execve`启动原生`atc.bin`，应按“Python launcher启动原生`atc.bin`”理解，而不是“GE直接运行在launcher Python进程里”。因此对Python pass bridge有效的当前进程仍然是`atc.bin`。

需要明确的是，`subprocess.Popen(["atc.bin", ...])`经过`execve`后，`atc.bin`子进程不会继承父Python进程内的解释器状态、GIL、`sys.modules`、Python object或已加载的libpython句柄；它只继承环境变量、当前工作目录、部分文件描述符等进程属性。因此外层launcher Python版本只能通过`PATH`、`LD_LIBRARY_PATH`、`PYTHONPATH`、`ASCEND_GE_PY_PASS_PATH`等环境变量间接影响`atc.bin`，不能直接作为bridge artifact版本依据。

实际日志对应的流程是TBE先拉起Python：

```text
用户 shell
  -> python -m ge.pyatc
      -> 外层 Python launcher 准备参数 / 环境
      -> 启动 atc.bin

atc.bin 进程
  -> TbeInitialize()
      -> PythonAdapterManager::Initialize()
          -> HandleManager::Initialize()
              -> dlsym(RTLD_DEFAULT, "Py_Initialize")
                  -> 若能找到符号:
                       -> LoadFuncs(RTLD_DEFAULT)
                       -> 若 Py_IsInitialized() == 1:
                            -> 复用已有解释器
                       -> 若 Py_IsInitialized() == 0:
                            -> TE_Py_Initialize()
                  -> 若找不到符号:
                       -> LaunchDynamicLib()
                       -> 按 PATH 中的 python3 / python 推导 libpython
                       -> dlopen(libpythonX.Y.so.1.0, RTLD_GLOBAL)
                       -> TE_Py_Initialize()
          -> import te_fusion.* / parallel_compilation
          -> init_multi_process_env()
          -> 拉起独立 python3 worker 子进程

atc.bin 进程
  -> LoadPassPlugins()
      -> dlopen(libge_python_pass_bridge.so)
      -> bridge EnsureBridgeReady()
          -> Py_IsInitialized() == 1
          -> 复用 TBE 已初始化的解释器
          -> owns_interpreter = false
          -> import ge.passes._ge_pass_native / ge.passes._bridge
          -> 注册 Python pass descriptor
```

这个场景下，TBE是解释器owner，Python pass bridge是解释器使用方。外层launcher Python与TBE worker Python都不是bridge所在的解释器。

#### 10.10.2 Python进程内`ctypes` / C API直接调用场景

如果后续存在不启子进程、而是在Python主进程内通过`ctypes.CDLL(...)`、C API wrapper或类似方式直接调用GE/ATC入口函数的模式，GE、TBE和Python pass bridge都运行在同一个Python进程地址空间内：

```text
python 主进程
  -> import ge.pyatc / ctypes.CDLL(...)
  -> 调用 main_impl / GE build 入口

同一 python 进程
  -> TbeInitialize()
      -> dlsym(RTLD_DEFAULT, "Py_Initialize")
          -> 应能找到当前 CPython 符号
      -> Py_IsInitialized() == 1
      -> TBE 复用当前 Python 解释器
      -> pyEnvStatusBeforeTbe = true
      -> TBE 不拥有解释器，不应 Py_Finalize

同一 python 进程
  -> LoadPassPlugins()
      -> dlopen(libge_python_pass_bridge.so)
      -> bridge EnsureBridgeReady()
          -> Py_IsInitialized() == 1
          -> 复用当前 Python 解释器
          -> owns_interpreter = false
          -> import ge.passes._ge_pass_native / ge.passes._bridge
          -> 注册 Python pass descriptor
```

这个场景下，解释器owner是外层Python主进程。TBE与Python pass bridge都只能作为解释器使用方，不能调用`Py_Finalize()` / `py::finalize_interpreter()`。

该模式下的版本选择最直接，核心原则是复用当前进程已有解释器：

- 当前Python进程的`sys.version_info`是唯一权威版本。
- `libge_python_pass_bridge.so`与`_ge_pass_native.so`必须匹配该Python minor version。
- TBE的`py_decouple.cc`在已能通过`RTLD_DEFAULT`找到CPython符号、且`Py_IsInitialized()==1`时，必须复用当前解释器；`PATH` / `python3-config`分支只应在当前进程没有可见CPython符号时进入。
- 如果发现当前进程已初始化的Python版本与bridge artifact manifest不一致，必须报错禁用Python pass；同一进程内并存两套CPython minor version不属于允许的fallback方案。

#### 10.10.3 内部`atc.bin`场景：TBE先拉起

`atc.bin`原生进程内，如果TBE先使用Python，则TBE的`py_decouple.cc`负责寻找和初始化libpython：

```text
atc.bin
  -> TbeInitialize()
      -> dlsym(RTLD_DEFAULT, "Py_Initialize")
          -> 已找到符号:
               -> 说明当前进程已可见 CPython C API
               -> 再调用 Py_IsInitialized()
                    -> true: 复用已有解释器
                    -> false: TBE 调 TE_Py_Initialize()
          -> 未找到符号:
               -> popen("python3 -V")
               -> 失败则 popen("python -V")
               -> 解析 Python major.minor
               -> 拼 so 名:
                    Python 3.7  -> libpython3.7m.so.1.0
                    Python 3.8+ -> libpython3.X.so.1.0
               -> popen("python3-config --prefix")
               -> 拼 $prefix/lib/libpythonX.Y[ m ].so.1.0
               -> mmDlopen(绝对路径, RTLD_NOW | RTLD_GLOBAL)
               -> 若失败:
                    -> mmDlopen("libpythonX.Y[ m ].so.1.0", RTLD_NOW | RTLD_GLOBAL)
                    -> 依赖 LD_LIBRARY_PATH / RUNPATH / ld.so.cache / 系统 lib 路径
               -> TE_Py_Initialize()

atc.bin
  -> LoadPassPlugins()
      -> dlopen(libge_python_pass_bridge.so)
      -> Py_IsInitialized() == 1
      -> bridge 复用 TBE 解释器
      -> owns_interpreter = false
```

这个场景下，fallback / codegen必须匹配TBE已经初始化的`atc.bin`进程内Python minor version。如果bridge artifact与该版本不一致，必须禁用Python pass或报清晰错误，不能再尝试加载另一套CPython。

#### 10.10.4 内部`atc.bin`场景：Python pass先拉起

如果某条流程中Python pass bridge早于TBE使用Python，则解释器owner会变成bridge：

```text
atc.bin
  -> LoadPassPlugins()
      -> 解析 bridge artifact set
      -> dlopen(libge_python_pass_bridge.so)
          -> libpython 由 bridge 的 ELF NEEDED / RUNPATH / LD_LIBRARY_PATH / ld.so.cache 解析
      -> bridge EnsureBridgeReady()
          -> Py_IsInitialized() == 0
          -> py::initialize_interpreter()
          -> owns_interpreter = true
          -> import ge.passes._ge_pass_native / ge.passes._bridge
          -> 注册 Python pass descriptor

atc.bin
  -> TbeInitialize()
      -> dlsym(RTLD_DEFAULT, "Py_Initialize")
          -> 能找到符号
      -> Py_IsInitialized() == 1
      -> TBE 复用 bridge 已初始化的解释器
      -> pyEnvStatusBeforeTbe = true
      -> TBE 不拥有解释器，不应 Py_Finalize
```

这个场景下要特别小心shutdown：bridge虽然`owns_interpreter=true`，但如果TBE后续复用了该解释器，bridge不能在TBE清理自己的Python module / parallel compilation前提前`py::finalize_interpreter()`。因此后续设计必须把“bridge清理Python pass状态”和“解释器最终finalize”拆成两个动作，或引入统一的进程级Python runtime manager。

这里有几个必须写死的判断原则：

- `Py_IsInitialized()`只描述**当前进程**中的CPython interpreter状态，不描述外部`python3`子进程状态。
- `Py_IsInitialized()`返回true只能说明当前进程已有解释器，不能单独说明应该选择哪个wheel；版本仍需通过进程内`sys.version_info`或`Py_GetVersion()`明确读取。
- 能通过`dlsym(RTLD_DEFAULT, "Py_IsInitialized")`或`Py_Initialize`找到符号，只说明当前进程可见CPython C API符号，不等价于解释器已经初始化。
- 同一进程只允许存在一套CPython minor version；任何模块只要发现当前进程已有解释器，后续Python pass artifact选择都必须以该解释器版本为准，不能再根据`PATH`中另一个`python3`重新选择不同minor version的产物。
- `libge_python_pass_bridge.so`与`_ge_pass_native.so`必须匹配同一个CPython minor version，并作为同一manifest描述的artifact set一起启用。
- 若bridge自己初始化解释器，bridge才能在进程级shutdown时`py::finalize_interpreter()`；若解释器来自用户Python主进程或TBE，bridge只能清理自己持有的module、holder和registry状态，不能finalize解释器。

因此，fallback runtime selector的推荐策略是：

1. 若当前进程内解释器已经初始化，以进程内Python版本作为唯一权威版本。
2. 若当前进程内解释器尚未初始化，以显式配置的目标Python executable或统一selector结果作为目标版本，并用该版本生成 / 选择artifact set。
3. bridge初始化后再次读取进程内Python版本，与artifact manifest做一致性校验。
4. 发现版本不一致时，优先清晰报错并禁用Python pass；同一进程混用两个CPython minor version不属于可接受的恢复路径。

这里特别需要防止一个shutdown顺序问题：

```text
TBE 初始化 Python
  -> Python pass bridge 复用解释器并缓存 py::object / holder
  -> TBE 先 Py_Finalize
  -> Python pass bridge 后续继续访问 py::object
```

上述顺序是非法的。更准确的原则不是“bridge永远先finalize”或“TBE永远先finalize”，而是：

- 所有模块持有的Python object、module、holder必须在解释器finalize前清理完。
- 真正调用`Py_Finalize()` / `py::finalize_interpreter()`的模块，必须是解释器owner，且必须等同进程内其他Python使用方都完成清理后才能执行。
- bridge的“清理Python pass状态”和“finalize interpreter”必须在设计上可分离，不能把二者永久绑成一个动作。

因此不同拉起顺序下的shutdown约束不同：

```text
TBE 先初始化 Python:
UnloadPassPlugins() (引用计数归零时)
  -> reset / clear Python pass holder、module、registry
GELib::Finalize()
  -> TBE / op store finalize
  -> TBE 清理自己的 Python module / parallel compilation
  -> TBE 在自己拥有解释器时 Py_Finalize

Python pass bridge 先初始化 Python:
UnloadPassPlugins() (引用计数归零时)
  -> reset / clear Python pass holder、module、registry
  -> 不能立即 py::finalize_interpreter，除非确认 TBE 等其他 Python 用户尚未初始化
GELib::Finalize()
  -> TBE / op store finalize
  -> TBE 只清理自己的 Python module，不 Py_Finalize
最后由 bridge owner 或进程级 Python runtime manager 再 finalize interpreter
```

后续若调整`GEFinalize`、`GeGenerator::Finalize`、`aclgrphBuildFinalize`、TBE plugin manager或pass plugin loader的顺序，必须重新检查该约束。只要bridge不拥有解释器，就不能假设自己能控制解释器生命周期；只要bridge拥有解释器但TBE已经复用过该解释器，就不能在TBE清理自身Python module前先finalize解释器。长期更稳妥的方向是引入进程级Python runtime manager或引用计数式owner协议，把“解释器初始化/复用/最终finalize”从单个bridge或TBE模块中抽象出来。

## 11. ATC扩展设计

为后续Python ATC入口做统一预留，当前建议分成两层：

- 当前主路径
- `ASCEND_GE_PY_PASS_PATH`

- 后续产品化入口
- CLI / 内部option可以再补显式参数，但最终都应归一到`ge.passes.bootstrap.load_pass_plugins()`的统一发现协议

- 与native companion模块相关的选择逻辑
- 不建议让用户直接指定某个pybind bridge源码或生成脚本
- 更适合由`ge.passes.fallback_runtime`统一决定“预编译 / fallback”产物选择

后续Python ATC入口不应再设计第二套发现机制，而是直接复用：

- `ge.passes.bootstrap.load_pass_plugins()`
- `ge.passes.bootstrap.get_registered_passes()`

## 12. 文件级开发计划

### 12.1 Python包与发现层

修改和新增如下文件：

- `api/python/ge/setup.py`
- `api/python/ge/ge/__init__.py`
- `api/python/ge/ge/passes/__init__.py`
- `api/python/ge/ge/passes/base.py`
- `api/python/ge/ge/passes/registry.py`
- `api/python/ge/ge/passes/pattern.py`
- `api/python/ge/ge/passes/replacement.py`
- `api/python/ge/ge/passes/bootstrap.py`
- `api/python/ge/ge/passes/_bridge.py`

职责如下：

- 定义三类pass的Python基类
- 提供装饰器和注册表
- 实现环境变量主路径发现逻辑，并为后续`entry_points`预留扩展
- 统一bridge对外接口
- 承载wheel选择与fallback管理

其中`base.py`的正式方向需要明确分两层：

- 用户继承层
- 继续暴露纯Python的`FusionBasePass` / `PatternFusionPass` / `DecomposePass`
- 这里不要求用户继承native C++ 基类

- wrapper来源层
- `PassContext` / `MatchResult` / `PatternMatcherConfig`等对象直接从`_ge_pass_native`取得native-backed实现
- import `_ge_pass_native`失败属于环境或产物装配问题，不再作为Python API的正式fallback路线处理

### 12.2 Python graph wrapper补齐

修改和新增如下文件：

- `api/python/ge/ge/graph/__init__.py`
- `api/python/ge/ge/graph/graph.py`
- `api/python/ge/ge/graph/node.py`
- `api/python/ge/ge/graph/tensor.py`
- `api/python/ge/ge/graph/tensor_desc.py`
- `api/python/ge/ge/_capi/pygraph_wrapper.py`

职责如下：

- 补齐borrowed handle
- 暴露tensor desc/shape
- 增补node的输入输出desc能力
- 补常量tensor读取接口

### 12.3 C wrapper与native bridge

修改和新增如下文件：

- `api/python/ge/ge_api_c_wrapper/c_graph.cc`
- `api/python/ge/ge_api_c_wrapper/c_gnode.cc`
- `api/python/ge/ge_api_c_wrapper/c_tensor.cc`
- `api/python/ge/ge_api_c_wrapper/c_match_result.cc`
- `api/python/ge/ge_api_c_wrapper/ge_api_c_wrapper_utils.h`
- `compiler/graph/fusion/pass/pass_plugin_loader.cc`
- `compiler/graph/fusion/pass/pass_plugin_loader.h`
- `compiler/graph/fusion/pass/python_fusion_base_pass_bridge_c_api.h`
- `compiler/graph/fusion/pass/python_fusion_base_pass_bridge_loader.cc`
- `compiler/graph/fusion/pass/python_fusion_base_pass_pybind_bridge.cc`
- `compiler/graph/fusion/pass/python_fusion_base_pass_pybind_bridge.h`
- 新增`_ge_pass_native.so`的源码、导出头和构建脚本
- `api/python/ge/ge_api_c_wrapper/CMakeLists.txt`
- `api/python/ge/CMakeLists.txt`
- `compiler/CMakeLists.txt`

职责如下：

- 为Python graph wrapper提供C接口
- 提供基于`pybind11`的Python pass bridge / helper so
- 接入wheel打包与安装

其中建议进一步拆分责任：

- `c_graph.cc` / `c_gnode.cc` / `c_tensor.cc` / `c_match_result.cc`
- 继续服务`ge.graph` / `ge.es`的ctypes路线

- 独立bridge `.so`
- 负责Python运行时初始化、descriptor同步、holder管理、adapter原生逻辑以及Python/C++ 对象转换
- 负责承接预编译 / fallback产物

- `_ge_pass_native.so`
- 负责`Graph` / `PassContext` / `MatchResult`等native-backed wrapper与helper
- 负责与`base.py`的对象来源对接
- 不承载用户pass基类，也不要求用户直接import C++ pass基类继承

- `pass_plugin_loader.cc/.h`
- 负责定位并`dlopen` bridge `.so`
- 负责和bridge `.so`的稳定ABI对接

- `python_fusion_base_pass_bridge_c_api.h`
- 定义bridge loader与`libge_python_pass_bridge.so`之间的稳定C ABI
- 当前入口为`GeGetPythonFusionBasePassBridgeApi()`

- `python_fusion_base_pass_bridge_loader.cc`
- 位于`ge_compiler.so`一侧
- 负责`dladdr`定位、`dlopen/dlsym`、缓存bridge API、以及把registrar回调传入bridge
- 当前显式使用`RTLD_GLOBAL`装载bridge，以便embedded CPython后续导入标准库/native extension时能解析到`libpython`符号

- `python_fusion_base_pass_pybind_bridge.cc/.h`
- 位于`libge_python_pass_bridge.so`一侧
- 负责Python运行时初始化、descriptor同步、holder生命周期和`create/run/destroy`回调实现
- 对`pass_plugin_loader`暴露稳定ABI，对Python侧复用`bootstrap / _bridge`协议

正式架构边界应是“bridge artifact set可替换，`ge_compiler.so`稳定”。其中：

- `libge_python_pass_bridge.so`是GE内部loader视角的主入口
- `_ge_pass_native.so`是Python视角的helper extension
- 二者必须作为同一版本、同一构建key的配套产物管理
- 当前第一批已把`python_fusion_base_pass_pybind_bridge.cc`从`ge_compiler` target中迁出，并新增`ge_python_pass_bridge` target产出`libge_python_pass_bridge.so`
- `ge_compiler.so`当前只保留loader、adapter、registry/runtime entry等稳定语义；bridge so才直接依赖`Python3::Python`与`pybind_options`

### 12.4 Pass注册核心改造

修改如下文件：

- `compiler/graph/fusion/pass/pass_registry.cc`
- `compiler/graph/fusion/pass/fusion_pass_executor.cc`
- 新增创建期上下文管理文件，例如：
- `compiler/graph/fusion/pass/pass_create_context.h`
- `compiler/graph/fusion/pass/pass_create_context.cc`

职责如下：

- 在`create_fn()`调用点注入TLS创建上下文
- 让通用creator能按`pass_name`找到对应Python descriptor
- 保持现有C++ pass行为不变

建议职责再细化为：

- `pass_create_context.h/.cc`
- 定义TLS上下文与RAII scope

- `fusion_pass_executor.cc`
- 在`InitPassesIfNeed`中围绕`create_fn()`加scope

- bridge注册函数
- 将Python pass descriptor注册为“固定creator函数 + pass_name元数据”

说明：

- `graph_fusion.cc`属于legacy兼容链路，不纳入本方案后续支持范围
- `REGISTER_CUSTOM_PASS`的后续支持走独立扩展阶段，但仍复用同一套descriptor / bridge / session机制

### 12.5 A/B分工与联调边界

当前建议按以下边界并行推进：

- A负责`libge_python_pass_bridge.so`、`pass_plugin_loader`、`ge_compiler.so`稳定ABI、adapter路由、fallback装载，以及现有`ge.graph.Graph`的borrowed view接入
- B负责`_ge_pass_native.so`、`base.py`、`PassContext` / `MatchResult`的native-backed wrapper，以及Python sample / Python API的补齐

B需要明确交付：

- `_ge_pass_native.so`的构建脚本与模块导出
- `PassContext` borrowed view wrapper
- `MatchResult`最小可用wrapper
- 必要的helper / 工厂接口，供`libge_python_pass_bridge.so`构造Python对象
- `base.py` / `pattern.py`中`PassContext` / `MatchResult` / `Pattern` / `PatternMatcherConfig`的native-backed直接导出
- Python sample和Python API补齐所需的最小能力清单

A需要明确交付：

- `graph.py`中`Graph._create_from(handle, owns_handle, owner)`的borrowed / non-owning语义
- `python_fusion_base_pass_pybind_bridge.cc`中`BuildPythonGraph()`对现有`ge.graph.Graph`的正式接入
- `libge_python_pass_bridge.so`与`_ge_pass_native.so`的桥接集成点

关于`Graph`这条边界，需要特别固定如下原则：

- `Graph`优先复用当前已经存在的`ge.graph.Graph`
- `_ge_pass_native.so`不再引入第二套用户可见的`Graph`类型
- A负责把runtime `GraphPtr`以borrowed view方式接到现有`ge.graph.Graph`
- B不直接拥有`Graph`类型本身，而是围绕`PassContext` / `MatchResult` / helper提供配套能力

B完成后，`base.py`应收敛为：

- `FusionBasePass` / `PatternFusionPass` / `DecomposePass`仍然保持纯Python基类
- `PassContext` / `MatchResult` / `PatternMatcherConfig`直接re-export `_ge_pass_native`提供的类型
- `Pattern`通过`ge.passes.pattern`直接导出`_ge_pass_native`提供的类型
- 不再为`_ge_pass_native`缺失场景保留兼容shim

A在本地拆分bridge `.so`与loader时，阶段1可先不依赖`_ge_pass_native`做最小验证，但这只是一条临时bring-up策略，不应继续保留到阶段2收口后的正式代码中：

- 先继续使用`FusionBasePass`现有纯Python合同类
- `_bridge.py`与bridge `.so`先验证descriptor、holder、create/run/destroy、`dlopen`、fallback产物选择
- 涉及`PatternFusionPass`的正式端到端验收，等B的`_ge_pass_native`落地后再合流完成

补充约束：

- A不拥有`base.py`，只消费B暴露出来的稳定Python接口
- B不直接拥有`libge_python_pass_bridge.so`，只提供bridge需要使用的稳定Python / native helper能力

### 12.6 ATC参数接入

修改如下文件：

- `api/atc/main_impl.cc`

职责如下：

- 当前不新增新的用户pass路由参数
- 后续若补CLI / option，也应最终归一到`ASCEND_GE_PY_PASS_PATH`或`ge.passes.bootstrap`的统一发现协议

### 12.7 文档、样例、测试

新增和修改如下文件：

- `examples/fusion_pass/README.md`
- `examples/fusion_pass/python_pass开发指南.md`
- `examples/fusion_pass/*/python/*`
- `tests/ge/ut/ge/graph/pyge_tests/*_test.py`
- `tests/ge/python_pass/*`

职责如下：

- 给出Python sample
- 给出用户使用文档
- 完成单测与集成验证

## 13. 协作与推进方式

说明：

- 本设计文档负责维护长期稳定的架构边界、A/B协作约束、接口冻结点和验收原则
- `PLAN.md`是唯一的阶段进展、checklist与完成状态来源
- 阶段目标、A/B子目标、已完成/未完成状态后续只在`PLAN.md`更新；本设计文档不再重复维护同类进度信息

### 13.1 协作总原则

建议持续按两条并行工作流推进，但以“接口先冻结、写集尽量分离、阶段状态统一回写`PLAN.md`”为基本原则。

并行协作遵循以下约束：

- 先冻结公共接口，再并发开发：
  - `ge.passes._bridge`与native bridge之间的descriptor / callback协议
  - `ge.graph` / `ge.es` / `_ge_pass_native.so`对Python可见的最小接口
- 进度跟踪统一维护在`PLAN.md`
- 设计文档只保留长期有效的边界，不保留“谁完成了哪一项”的过程性记录
- 阶段验收可以按阶段逐步执行，但是否“阶段完成”以`PLAN.md`的checklist为准

### 13.2 A/B工作流边界

两条工作流的长期边界如下：

- A聚焦compiler / native bridge / loader / adapter / fallback / 现有`ge.graph.Graph`接入
- B聚焦`_ge_pass_native.so`、`base.py`、`PassContext` / `MatchResult`、Python sample与Python API补齐

与当前实现直接相关的固定边界如下：

- A负责`libge_python_pass_bridge.so`、`pass_plugin_loader`、`ge_compiler.so`稳定ABI，以及现有`ge.graph.Graph`的borrowed view接入
- B负责`_ge_pass_native.so`、`base.py`、`PassContext` / `MatchResult`的native-backed形态，以及sample / Python API的后续补齐
- `Graph`优先复用当前已经存在的`ge.graph.Graph`
- `_ge_pass_native.so`不再引入第二套用户可见的`Graph`类型
- A不拥有`base.py`
- B不拥有`libge_python_pass_bridge.so`

### 13.3 阶段推进与交付方式

后续阶段按下面的推进顺序协作：

1. 先冻结阶段边界与完成定义，并写入`PLAN.md`
2. 再冻结A/B之间的最小接口
3. 各自按写集并行开发
4. 合流时优先完成测试内sample和定向验证
5. 通过后把checklist状态更新回`PLAN.md`

对阶段定义本身，建议保持以下长期顺序：

- `FusionBasePass`正式打通
- `PatternFusionPass`打通
- `DecomposePass`打通
- fallback与预制版本打通
- sample的Python等价实现
- 配套文档、验证与交付资料完成

这些阶段的详细子项、状态和阻塞项统一引用`PLAN.md`，不在设计文档中再次展开。

### 13.4 阶段验收与文档同步

每个阶段收口时，建议同步完成以下动作：

- 更新`PLAN.md`中对应checklist的完成状态
- 若接口边界发生变化，再更新本设计文档
- 若只是状态变化，不再修改本设计文档中的阶段说明
- 不再新增独立的阶段进展/验收markdown；过程性进展统一回写`PLAN.md`
- 为阶段验收沉淀最小验证命令、结果摘要和已知阻塞项

这样可以保证：

- `PLAN.md`反映实时进度
- 设计文档保持稳定，不被过程性状态污染
- A/B在联调时始终有唯一的进展来源

## 14. 验证与验收要求

### 14.1 测试分层

V1的测试建议分四层组织：

- Python单测：
  - `registry`、`decorator`、`bootstrap`
  - 环境变量主路径、后续`entry_points`自动发现
  - descriptor规范化
  - borrowed handle的stale检查
- C++ / native单测：
  - bridge初始化与重复初始化
  - TLS创建上下文
  - 动态注册
  - session生命周期
  - 异常隔离
- 集成测试：
  - `FusionBasePass`
  - `PatternFusionPass`
  - `DecomposePass`
  - `MatchResult`
  - `GeUtils.InferShape`
  - `GeUtils.CheckNodeSupportOnAicore`
- 打包与安装测试：
  - 主wheel安装
  - native子wheel选择
  - 无匹配版本时fallback编译
  - 开发态路径 / 模块直传

### 14.2 阶段验收原则

每个阶段的完成定义、A/B子目标、状态和阻塞项统一以`PLAN.md`为准，本设计文档不再重复维护“阶段一/阶段二/阶段三”的硬编码验收节点。

阶段验收建议统一采用以下结构：

- 完成定义：
  - 直接引用`PLAN.md`中对应阶段的“阶段完成定义”
- 必过验证：
  - 至少覆盖一条正向主链路
  - 至少覆盖该阶段新增接口的异常路径或失败隔离
  - 至少覆盖生命周期、重复加载或资源释放中与该阶段直接相关的部分
- 收口要求：
  - `PLAN.md`对应checklist状态完成更新
  - 设计文档中受影响的接口边界、装载关系或职责分工完成同步
  - 阶段验收记录中保留验证命令、结果摘要和已知阻塞项

另外需要遵循一个约束：

- 某项能力属于哪个阶段，就在哪个阶段验收，不提前并入其他阶段
- 例如`entry_points`、预制版本、多版本native产物、fallback等产品化能力，应以`PLAN.md`中对应阶段为准，而不是提前写入前置阶段的完成标准

### 14.3 里程碑组织建议

考虑到项目以“两周”为节奏推进，正式里程碑建议按`PLAN.md`当前优先级动态组织，而不是在本设计文档中写死“某两个阶段必须绑在一起验收”。

建议每个里程碑遵循以下原则：

- 一个里程碑尽量只收口`1~2`个可以形成闭环的主题
- 优先围绕当前主目标组织，例如：
  - `FusionBasePass`闭环
  - `PatternFusionPass`闭环
  - `DecomposePass`闭环
  - fallback / 预制版本闭环
  - sample与交付资料闭环
- 每个里程碑都应输出：
  - 本轮完成的checklist delta
  - 定向验证命令与结果摘要
  - 仍然存在的阻塞项
  - 下一里程碑的交接前提

扩展项建议单独组织里程碑，不与V1主链路验收强绑定。例如`REGISTER_CUSTOM_PASS`更适合在主链稳定后单独验收。

### 14.4 建议验收输出物

每次正式验收建议至少沉淀以下输出物：

- 测试结果汇总表：
  - 用例名称
  - 覆盖能力点
  - 执行结果
  - 失败原因与结论
- sample执行记录：
  - 输入模型或脚本
  - 触发的pass
  - 关键日志或结果摘要
- 已知问题列表：
  - 是否阻塞下一阶段
  - 规避方式
  - 计划修复阶段

### 14.5 总体验收维度

V1最终建议按以下维度做总体验收：

- 发现与装载：
  - 当前纳入范围的发现机制与`PLAN.md`一致，并且与用户文档口径一致
  - 若本轮纳入`entry_points`、预制版本或fallback，则对应链路具备独立验收记录
- 三类pass主链：
  - `FusionBasePass`、`PatternFusionPass`、`DecomposePass`在各自纳入范围内可发现、注册、创建和执行
- Python / native wrapper：
  - `Graph`、`PassContext`、`MatchResult`以及阶段所需helper具备对应能力
- 稳定性：
  - Python pass导入失败可隔离
  - Python pass执行异常可隔离
  - 生命周期、重复加载、资源释放语义具备验证记录
- 交付与资料：
  - `PLAN.md`、设计文档、sample、验证记录和限制说明保持一致

### 14.6 阶段进展引用

当前阶段完成状态、A/B子目标、未完成项和阻塞项不在本设计文档中重复维护，统一以`PLAN.md`为准。

本设计文档只保留以下长期验收要求：

- 每个阶段收口时必须同步更新`PLAN.md`
- 若接口、职责边界或装载关系发生变化，必须同步更新本设计文档
- 若只是任务状态变化，只更新`PLAN.md`
- 阶段验收需要沉淀验证命令、结果摘要和已知阻塞项

## 15. 风险与关注点

- `CreateFusionPassFn`如果直接改成`std::function`，可能引入`dlclose`与全局析构顺序耦合风险，优先采用“函数指针 + TLS创建上下文 + 进程级注册表”的方案。
- `Graph` borrowed wrapper已经落地，但后续新增运行时wrapper仍必须遵守“默认不接管runtime句柄所有权”的约束，避免再次引入双释放风险。
- 本地开发环境是Python 3.13，而正式wheel规划是`cp37-cp314`，本地只能覆盖当前环境可用的Python tag，完整矩阵需要发布流水线按Python minor版本分别验证。
- V1需要在文档和实现中明确区分两类native策略：`ge.graph`继续沿用ctypes/C wrapper，Python pass bridge采用`pybind11`。
- 如果继续把更多Python版本敏感逻辑直接编进`ge_compiler.so`，后续bridge artifact set与fallback的演进空间会被压缩；因此后续实现必须把版本差异收敛到可替换的`libge_python_pass_bridge.so` / `_ge_pass_native.so`中。
- `atc.bin`内可能由TBE先初始化Python，也可能由Python pass bridge先初始化Python；fallback选择必须以当前进程内解释器或统一selector为准，并把bridge状态清理与解释器finalize拆开设计，保证所有Python使用方都清理完之后才由owner finalize。
- `PatternFusionPass`的Python化不是简单函数回调，必须保证pattern/match/replacement三段语义与现有C++ 框架一致。
- 在B的`_ge_pass_native`落地前，本地验证可以先不依赖它，但这只能用于独立bridge `.so`拆分与`FusionBasePass`回归；不能替代`PatternFusionPass`的最终验收。
- 9个sample全部提供Python对照版会扩大wrapper覆盖面，需要优先保证主链路可用，再逐步补齐边角接口。

## 16. 与后续自定义算子Python化的复用边界

本方案不只是为Python pass单点服务，更是在铺设“GE/CANN主框架安全调用Python扩展能力”的通用底座。

后续如果推进“自定义算子Python化”，本方案中的一大部分基础设施可以直接复用，但算子定义、交付和下沉相关能力仍然需要新增专属设计。

### 16.1 可直接复用的能力

以下能力后续可以直接复用到自定义算子Python化中：

- Python插件发现协议：当前以环境变量主路径为基线，后续再补`entry_points`自动发现，这套协议可以抽象成通用的Python扩展发现框架。
- 主wheel + native子wheel + fallback codegen：这一套“纯Python主包 + 多Python版本native伴生包 + 未命中版本本地兜底编译”的发布模式，本质上与pass/custom op无关。
- bridge生命周期管理：Python解释器初始化、重复初始化幂等、异常隔离、卸载顺序控制、日志与诊断都可以复用。
- GIL与锁策略：当前文档中定义的“管理锁 + session轻量状态 + GIL精确包围Python回调”的策略，适用于后续绝大多数Python扩展接入点。
- 执行期session / holder模型：运行期对象和Python wrapper的owner token、stale handle检查、borrowed对象失效后转Python异常而不是崩溃，这一套机制同样适用于自定义算子回调期对象。
- Python注册表与descriptor模型：当前pass设计中的registry、descriptor、bootstrap、bridge出口可以演化为通用“Python extension registry”。
- Pythonic约束：不要求用户手动释放对象、不要求手动管理GIL、不要求用户显式close/release的体验目标，后续应继续保持。

如果只看“Python能力正式接入GE/CANN”的底座层，这一层的复用度预估可达到`60%~70%`。

### 16.2 可部分复用的能力

以下能力可以复用一部分，但需要根据自定义算子的语义重新裁剪：

- `ge.graph` / `ge.es` wrapper：如果后续自定义算子Python化需要表达schema、infer逻辑、图级replacement或eager入图过程，这些graph wrapper仍然有价值；但它们不能替代算子schema自身的建模。
- 插件路径与安装路径管理：当前仓已经存在custom opp路径和plugin manager能力，例如`graph_metadef/base/common/plugin/plugin_manager.cc`中的`custom_op_lib_path_`相关逻辑，这说明“插件发现”和“自定义交付件安装路径”有现成基础；但Python custom op还需要定义更明确的Python包和OPP包之间的映射规则。
- 错误传播与降级策略：当前pass里的“单插件失败不拖垮主链路”思路仍然适用，但custom op往往比pass更靠近编译主流程，哪些错误允许降级、哪些错误必须hard fail，需要重新分级。
- ATC参数入口：本方案里为`--py_pass_path`、`--py_pass_module`预留的参数接入方式，后续可扩展到Python custom op；但参数命名、优先级和与现有custom opp目录配置的关系还需要再定。

这一层更像“机制复用”，不是“实现直接拷贝”。复用比例大致在`20%~30%`。

### 16.3 需要为自定义算子新增的能力

以下部分基本不能直接从Python pass方案里获得，后续需要单独设计：

- 自定义算子的schema/OpDef注册模型：包括输入输出、属性、约束、默认值、版本、命名空间。
- shape/type inference的Python注册与执行模型。
- kernel交付链路：例如AscendC/Triton/TBE/host侧实现、编译产物、二进制布局、版本校验。
- OPP包布局与安装协议：Python包、op proto、kernel二进制、tiling文件、配置文件之间的目录组织关系。
- 编译/运行时识别逻辑：ATC和在线编译阶段如何发现、校验、下沉Python custom op。
- 自定义算子与framework adaptor的衔接：例如当前`examples/custom_op`中展示的PyTorch/TensorFlow入图方式，这部分明显超出pass的职责范围。

如果后续目标只做到“Python写schema + infer + 注册”，那么当前pass方案带来的复用会更高；如果目标还包括“Python侧完整驱动kernel开发、打包、发布”，新增工作量会明显增大。

### 16.4 复用结论

建议把“Python pass化”和“Python custom op化”看成两个阶段：

- 第一阶段先完成Python pass，把Python插件接入、生命周期、内存管理、锁/GIL、打包与fallback这些通用底座做稳定。
- 第二阶段在这个底座上补自定义算子专属模型，包括schema、infer、kernel交付和OPP布局。

从项目维度粗估：

- 基础设施层复用：`60%~70%`
- 项目整体复用：`40%~50%`

这个比例已经足够说明，当前Python pass设计不是一次性方案，而是在提前建设后续Python custom op可以持续复用的公共底座。
