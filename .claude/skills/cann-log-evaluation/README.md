# CANN 日志质量评测 Skill（cann-log-evaluation）

面向 CANN 源码的日志打印质量评测：识别用户指定文件或目录中的全部运行时日志/打印语句，
依据 CANN 日志规范输出问题清单、量化指标得分和 10 分制日志质量总评分。
Skill 只发现问题并给出修复建议，不修改被检仓库。

评测对象是**源码中的日志语句本身**。所有入口都使用完整源码协议：先识别扫描范围内的
全部日志，再对每条日志执行规则审查；每个完整源码文件只送审一次，超长文件直接失败
记录，不截断、不分段。

最终交付物是 Markdown 报告（含 10 分制总评分与 27 项量化指标）；Excel 仅在显式需要时
作为可选附件生成，不再是交付前提。

## 核心能力

- 完整源码覆盖：扫描范围内的全部日志语句均纳入评测，注释、初始化、数学 `log` 和普通字符串不算日志。
- 规则与 LLM 双判定：敏感信息、刷屏、级别敏感词等可机械验证的规则用确定性规则判定；错误描述完备性、调用链覆盖等语义类问题由大模型判定。
- 断点续跑：成功请求写入进度文件，重新运行同一命令自动继续未完成批次。
- 部分评估告警：即使存在失败请求，也会生成已成功处理部分的报告，并在元数据和验证结果中标记完整性告警。
- 在 Markdown 主报告中输出 10 分制总评分、每项量化指标得分、带规则大类/严重级/代码位置/修复建议的问题明细和分队整改清单。

## 评分口径

- **总评分（10 分制）**为全部量化评估指标得分的平均值；敏感信息明文违规一票否决（命中即 0 分）。请求失败时仍使用已完成部分计算，并标记为部分评估。
- 量化指标共 27 项，分六个族：字段完整性（LOG-F）、错误描述（LOG-E）、调用链与流程（LOG-C）、泛滥抑制（LOG-L）、级别语义（LOG-V）、安全与语言合规（LOG-S），按 L1-L3 优先级分层，每项独立给出 10 分制得分。
- 规范规则归入可定位性、可读性、有效性、安全合规四个规则大类，分类统计表和 Markdown 报告会展示该字段。
- 完整口径见 [references/quantitative-metrics.md](references/quantitative-metrics.md)（指标定义、量化公式、打分规则）和 [references/log-rules.md](references/log-rules.md)（扫描范围、规则、判定尺度、严重级和黑名单的唯一事实源）。

## 安装

### 从仓库安装

在仓库根目录执行：

```bash
python3 scripts/install.py \
  --skill cann-log-evaluation \
  --target codex \
  --overwrite
```

`--target` 可选 `codex`、`claude`、`opencode` 或 `all`。安装后可验证：

```bash
python3 scripts/install.py \
  --skill cann-log-evaluation \
  --target codex \
  --verify-only
```

### 远程安装

```bash
curl -fsSL https://raw.gitcode.com/cann-tools/openskills/raw/main/scripts/install.sh \
  | bash -s -- --skill cann-log-evaluation --target codex
```

或使用本 skill 自带的一键引导脚本（克隆仓库后调用 `install_skill.py`）：

```bash
curl -fsSL https://raw.gitcode.com/cann-tools/openskills/raw/main/cann-log-evaluation/scripts/bootstrap.sh \
  | bash -s -- --target codex
```

### 在 skill 目录内安装

```bash
python3 cann-log-evaluation/scripts/install_skill.py --target codex
```

`--target` 可选 `codex`、`claude`、`opencode` 或 `all`，`--dry-run` 仅展示将要执行的操作。

安装后重启对应 Agent，使其重新加载 Skill。

## LLM 配置（必需）

本 Skill 完全依赖大模型完成日志识别与规范评测，运行前必须配置以下环境变量：

```bash
export LLM_API_BASE="https://api.openai.com/v1"
export LLM_API_KEY="sk-..."
export LLM_MODEL="glm-5.3-flash"  # 可省略，默认即为该模型
```

| 环境变量 | 必需 | 说明 | 默认值 |
|---|---|---|---|
| `LLM_API_BASE` | 是 | OpenAI 兼容的 API 基础地址 | 无 |
| `LLM_API_KEY` | 是 | API 密钥 | 无 |
| `LLM_MODEL` | 否 | 模型名称；也可用 `--model` 覆盖，优先级 `--model` > `LLM_MODEL` > 默认值 | `glm-5.3-flash` |
| `LLM_MAX_INPUT_CHARS` | 否 | 完整请求输入硬上限（字符），单文件超限直接失败 | `1000000` |
| `LLM_MAX_TOKENS` | 否 | 可选输出上限；未设置时不发送 `max_tokens` | 不限制 |

核心报告生成只用 Python 标准库；可选 Excel 附件需要 `openpyxl`，上传 OBS 需要 `esdk-obs-python`（两者均见 `requirements.txt`）：

```bash
pip install -r cann-log-evaluation/requirements.txt
```

## 单仓评测快速开始

以评测 https://gitcode.com/cann/runtime 仓 `src/dfx/log` 目录的日志质量为例。
以下命令均在仓库根目录执行。

1. 配置 LLM 环境变量（见上一节）。

2. 准备输入，两种方式任选：

   用脚本拉取目标仓（默认浅克隆到 skill 的 `input/` 目录，目录名为仓名；用 `--branch master` 时为 `仓名@master`）：

   ```bash
   python3 cann-log-evaluation/scripts/fetch_repo.py \
     --url https://gitcode.com/cann/runtime.git
   ```

   或跳过克隆，直接用 `--input` 指向本地已有源码目录（整仓或子目录均可）。

3. 执行单仓评测：

   ```bash
   python3 cann-log-evaluation/scripts/run_local.py \
     --input cann-log-evaluation/input/runtime/src/dfx/log \
     --name runtime-log
   ```

   `--input` 指向被评测目录，`--name` 为本次评测名称（会作为报告中的仓库名），
   输出默认写入 skill 的 `output/` 目录。

4. 在 `output/` 中查看日志质量评分：

   - 打开当日唯一 Markdown 报告 `cann-log-evaluation-report-YYYYMMDD.md`：
     顶部“管理摘要”给出总评分、总 DI、按来源分类和每类问题示例（适合快速汇报）；
     “一、评测概览”的“总评分（10分制）”列即该仓日志质量评分，
     “三、量化评估指标”查看每项指标的合规率、得分、违规数与样本数，
     “四、问题明细”查看逐条问题。
   - 同目录的 `<仓名>_findings.jsonl` 是机器可读的问题明细。
   - 如需 Excel 附件，在评测命令后追加 `--excel`（需要 `openpyxl`）。

评测中断后重新运行同一命令会自动续跑；`--restart` 丢弃旧进度从头评测。

日志识别请求默认按约 90 万字符装包（`--target-input-chars` 可调）。若所用网关对单请求
有超时限制（表现为大请求返回 HTTP 504），调小该值让每次请求更快返回，例如
`--target-input-chars 250000`；已完成文件不受影响，重跑会自动续跑。

## 批量与每日评测

```bash
# input/ 下每个子目录各当一个仓分别评测（支持 `仓名@分支` 目录命名）
python3 cann-log-evaluation/scripts/run_local.py --per-subdir

# 每日全量巡检：默认只评测 master 分支目录，额外产出日差异与分队整改清单
python3 cann-log-evaluation/scripts/daily_run.py --input <input目录> --out <output目录>
```

默认总并发 64，单仓最多 8 个模型请求。`run_local.py` 用 `--agents`（仓库级独立
agent 数，默认 10）、`--max-concurrency`、`--max-workers-per-repo` 覆盖；
`daily_run.py` 用 `--concurrency` 和 `--max-workers-per-repo` 覆盖。
`run_local.py --daily` 为每日巡检预设（master-only + 完整文件送审）。

## 使用示例

```text
使用 $cann-log-evaluation，评测 CANN runtime 仓 src/dfx/log 目录的日志打印质量：
识别该目录全部源码中的运行时日志语句，依据 CANN 日志规范输出问题清单、
每项量化指标得分和 10 分制日志质量总评分，并给出修复建议。
```

请求只需明确被评测的仓库/目录范围；Skill 会自动完成输入准备、日志识别、
规则审查与评分报告。

## 输出

`output/` 目录包含（`YYYYMMDD` 为评测日期）：

| 交付物 | 说明 |
|---|---|
| **`cann-log-evaluation-report-YYYYMMDD.md`** | **唯一主交付报告**：顶部管理摘要（总评分/总 DI/按来源分类/每类问题示例）、评测概览、分类统计、27 项量化评估指标、完整问题明细（代码位置/源码行/原因/建议/发现时间/分队）、完整性告警 |
| `issue-tracking.json` | 问题跟踪 JSON（`report_url` 指向 OBS 上的 Markdown 主报告） |
| `cann-log-evaluation-report-YYYYMMDD.xlsx` | 可选附件（`--excel`）：概览、分类统计、问题明细、量化评估指标 |
| `<仓名>_findings.jsonl` | 每仓问题明细（规则、严重级、代码位置、建议） |
| `daily-team-assignment.md` | 分队整改清单（每日入口） |
| 日差异 | 与上一次评测日的问题增量（每日入口） |

规则变化后重新运行会使用新的评测指纹，不复用旧结果。

## 上传 OBS 桶

评测完成并生成 `output/issue-tracking.json` 和对应的
`output/cann-log-evaluation-report-YYYYMMDD.md` 后，可使用上传脚本将交付物同步到 OBS：

```bash
pip install -r requirements.txt

export OBS_ACCESS_KEY_ID="<华为云 Access Key>"
export OBS_SECRET_ACCESS_KEY="<华为云 Secret Key>"
# 使用临时凭证时可选：
export OBS_SECURITY_TOKEN="<Security Token>"

python3 scripts/upload_issue_json_obs.py
```

默认配置如下：

- 桶：`opencann-obs`
- 服务地址：`https://obs.cn-north-4.myhuaweicloud.com`
- 上传内容：Markdown 主报告和 `issue-tracking.json`；Excel 附件仅在传入 `--excel-file` 时一并上传
- 目标目录：`cann_exp/static/prod/A2_A3/agent_component_log/default/YYYYMMDD/`、
  `ann_exp/static/prod/A5/agent_component_log/default/YYYYMMDD/`

日期默认从 JSON 中的 `report_url` 读取；历史空结果没有日期时，使用 `--date YYYYMMDD` 指定。上传前可先只校验文件并查看目标地址：

```bash
python3 scripts/upload_issue_json_obs.py --dry-run
```

如需覆盖输入文件、报告、桶或日期，可使用 `--file`、`--report-file`、`--excel-file`、`--bucket`、`--date`；`--json-only` 仅上传 JSON，适用于特殊补传场景。正式交付为 Markdown + JSON，Excel 可选。历史 JSON 中的 Excel 链接仍可补传。

## 目录结构

```text
cann-log-evaluation/
├── SKILL.md                          # Agent 执行协议：运行入口、评测要求与输出
├── README.md                         # 本文档
├── requirements.txt                  # 可选依赖：openpyxl（Excel 附件）、esdk-obs-python（OBS 上传）
├── references/
│   ├── log-rules.md                  # 规范规则唯一事实源（扫描范围、规则、严重级、黑名单）
│   ├── quantitative-metrics.md       # 27 项量化指标、量化公式与 10 分制打分规则
│   ├── repo-component-map.json       # 仓名-组件映射（每日入口与默认拉取清单来源）
│   └── repo-team-map.json            # 仓名-分队映射（分队整改清单来源）
├── scripts/
│   ├── run_local.py                  # 本地一键入口（单仓 / 批量）
│   ├── daily_run.py                  # 每日全量巡检入口
│   ├── fetch_repo.py                 # 拉取仓库到 input/（支持 --url / --list / 组件映射）
│   ├── generate_all_reports.py       # 生成交付物（Markdown 主报告 / issue-tracking；Excel 可选）
│   ├── gen_md_report.py              # 唯一 Markdown 主报告（管理摘要 + 评分 + 指标 + 明细）
│   ├── report_metrics.py             # 27 项指标、10 分制评分、DI、分队（MD/Excel 共用）
│   ├── gen_excel_report.py           # 可选 Excel 附件（含 10 分制评分）
│   ├── run_rule_scenarios.py         # 规则场景拦截测试运行器（pipeline / live）
│   ├── export_issue_json.py          # 问题跟踪 JSON 导出
│   ├── llm_evaluate.py               # 评测引擎：两阶段日志识别 + 规则审查
│   ├── install_skill.py              # skill 安装器
│   ├── bootstrap.sh                  # 远程一键引导安装
│   ├── upload_issue_json_obs.py      # OBS 上传
│   └── ...                           # 校准、探测、恢复等辅助脚本
├── tests/                            # 单元测试（pytest）
│   └── scenarios/                    # 27 条规则 + 4 个控制场景的本地用例
└── docs/                             # 内部方案记录
```

### 核心文件说明

- [SKILL.md](SKILL.md)：核心执行流程、评测要求（findings 必须引用真实文件、行号、规则、严重级、原因和建议）与输出契约。
- [log-rules.md](references/log-rules.md)：扫描范围、规则、判定尺度、严重级和黑名单的唯一来源，规则分可定位性、可读性、有效性、安全合规四类。
- [quantitative-metrics.md](references/quantitative-metrics.md)：27 项量化指标的定义、量化公式与 10 分制打分规则，是总评分的依据。
- [run_local.py](scripts/run_local.py)：本地一键入口，扫描 input/ → 大模型评测 → output/ 生成 Markdown 主报告与问题 JSON（`--excel` 可选附件）。
- [llm_evaluate.py](scripts/llm_evaluate.py)：评测引擎，完成完整源码覆盖的模型请求；模型请求由它执行，拼写审查先核对仓库内真实标识符，不确定时不报。
- [report_metrics.py](scripts/report_metrics.py)：27 项量化指标与 10 分制评分的唯一实现，Markdown 与 Excel 报告共用，保证两种格式数值一致。

## 测试

```bash
cd cann-log-evaluation && python3 -m pytest tests/ -q

# 规则场景拦截测试（离线，不调用模型）
python3 scripts/run_rule_scenarios.py --scenarios tests/scenarios   --mode pipeline --out output/scenarios

# 规则场景拦截测试（真实模型，需要 LLM 凭证）
python3 scripts/run_rule_scenarios.py --scenarios tests/scenarios   --mode live --jobs 4 --min-recall 0.8 --min-precision 0.9   --out output/scenarios
```

`tests/scenarios/` 按 `references/log-rules.md` 的 27 条规则各构造一个正例仓和负例控制，
`--mode live` 的计分卡输出到 `output/scenarios/scenario-report.md`（逐规则拦截/漏报/误报）。

## 安全边界

只发现问题并给出建议，不修改被检仓库；评测产物只写入 skill 自己的 `output/` 目录，
被评测源码只读。OBS 上传需要用户显式提供华为云凭证，不上传评测输入源码，只上传
报告交付物。
