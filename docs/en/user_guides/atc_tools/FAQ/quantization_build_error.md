# When quantization is enabled, model conversion reports "build_main build graph[infer_graph_info] failed"?

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:23:53.876Z pushedAt=2026-10-28T08:50:48.640Z nl=2 -->

## Symptoms

During model conversion, the quantization feature is configured through the [--compression\_optimize\_conf](../CLI_options/--compression_optimize_conf.md) parameter, but the model conversion fails. The following error message is displayed:

![Figure](../figures/quant_build_error.png)

## Cause Analysis

This issue may be caused by a failure to compile the original model.

## Solution

You can try disabling quantization and performing model conversion again to identify the cause of the error. After the original model is successfully converted without quantization, try enabling quantization again.
