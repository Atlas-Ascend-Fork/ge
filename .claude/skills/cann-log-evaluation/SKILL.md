---
name: cann-log-evaluation
description: "评测 CANN 代码的日志打印质量。依据 references/log-rules.md，识别完整源码中的日志语句并输出带规则大类、严重级、代码位置和修复建议的报告。"
license: MIT
metadata:
  author: CANN-Tools
  version: "4.2.1"
---

# CANN 日志规范评测

评测用户指定文件或目录中的全部运行时日志/打印语句，只发现问题并给建议，不修改被检仓库。

## 规范来源

先读取 [`references/log-rules.md`](references/log-rules.md)。它是扫描范围、规则、判定尺度、严重级和黑名单的唯一来源。

## 运行入口

```bash
export LLM_API_BASE="https://api.openai.com/v1"
export LLM_API_KEY="sk-..."
export LLM_MODEL="glm-5.3-flash"  # 可省略，默认即为该模型

python3 scripts/run_local.py --input <代码路径> --name <名称>
python3 scripts/run_local.py --per-subdir
python3 scripts/daily_run.py --input <input目录> --out <output目录>
```

Markdown 是默认且唯一的正式交付格式；`--excel` 可额外生成可选的 Excel 附件（需要
`openpyxl`），不影响报告链路是否成功。

所有入口都使用完整源码协议：先识别扫描范围内的全部日志，再执行规则审查。每日入口默认只处理组件映射中的 `master` 目录，默认总并发 64，单仓最多 8 个请求；可用 `--model`、`--concurrency` 和 `--max-workers-per-repo` 覆盖。

模型选择顺序为 `--model` > `LLM_MODEL` > `glm-5.3-flash`。成功请求会写入进度文件，重新运行会继续未完成批次。完整文件超过输入硬上限时记录失败，不截断源码。即使存在失败请求，也会生成已成功处理部分的报告，并在元数据和验证结果中标记完整性告警。

## 评测要求

- 注释、初始化、数学 `log` 和普通字符串不算日志。
- 自定义宏、包装函数、回调和事件上报要沿调用链判断真实输出。
- 拼写审查先核对仓库内的真实标识符；不确定时不报。
- findings 必须引用真实文件、行号、规则、严重级、原因和建议。

## 输出

最终交付是 Markdown 报告：`output/cann-log-evaluation-report-YYYYMMDD.md`（唯一报告）。
CI 或外部流水线需要固定文件名时，可加 `--report-name`：

```bash
python3 scripts/run_local.py --input <代码路径> \
  --out ./operators/pr-{pr_number} \
  --report-name {pr_number}_log_check_summary.md
```

报告路径为 `./operators/pr-{pr_number}/{pr_number}_log_check_summary.md`。
顶部为管理摘要（10 分制总评分、总 DI、按来源分类、每类问题示例），其后为评测概览
（严重级分布、完整性告警）、27 项量化评估指标和完整问题明细
（代码位置、源码行、原因、建议、发现时间、所属分队）。

- 最终回复必须给出 Markdown 报告路径，并直接汇报总评分、严重级分布、问题总数和
  Top 问题；不得只给出 Excel 或只给文件清单。
- `issue-tracking.json` 的 `report_url` 指向 OBS 上的 Markdown 主报告；同目录还有
  每仓 JSONL、日差异和 `daily-team-assignment.md`（不再单独生成汇总报告，摘要已并入
  主报告顶部）。
- Excel（`--excel`）是可选附件，不参与交付判定；规则变化后重新运行会使用新的评测
  指纹，不复用旧结果。
