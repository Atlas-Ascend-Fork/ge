# How Do I Determine the Correspondence Between Operators in the Original Framework Network Model and Operators Supported by the AI Processor

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:22:57.172Z pushedAt=2026-10-28T08:44:34.599Z nl=2 -->

## Symptoms

When using the precision comparison tool or performance comparison tool to analyze operator precision or performance, if a user finds that the precision or performance of some operators is problematic, they may consider using certain parameters in the ATC tool to adjust the computation precision of the operators and then perform model conversion and inference again. For example, they may configure the problematic operators to a blocklist through the [--modify\_mixlist](../CLI_options/--modify_mixlist.md) parameter. In this scenario, the parameters in ATC require that the configured value must be the OpType of the operators defined based on Ascend IR.

How can the OpType of such operators be obtained? Or, how can the OpType of the operators supported by our AI processor be obtained through the operators in the original framework network model?

## Possible Causes

When you find that the precision or performance of certain operators is problematic and need to adjust the precision of these operators, you may not know which Ascend IR operator corresponds to them.

## Solution

The following describes how to obtain the OpType of an Ascend IR operator:

- If you are using the Profiling tool for operator performance analysis, you can directly obtain the operator types supported by the AI processor. For details, see the *Performance Tuning Tool* manual:
    1. Export the "AI Core and AI CPU operator data" from the summary data. The file name is in the “op\_summary\_\*.csv” format.
    2. The "OP Type" column in the file lists the OpTypes of the operators supported by the AI processor. Find the problematic operator in this column.

- If you are using the precision comparison tool for operator precision analysis:
  - See the *Precision Debugging Tool* manual to obtain the precision comparison result file result\_\*.csv.
  - Find the name of the problematic operator based on the "NPUDump" column in the file, and then search for the corresponding OpType in the corresponding dump data file.

    The first segment of the dump data is the OpType of the operator supported by the AI processor. For example, the following figure shows the operator information highlighted in red in the dump data:

    ![Figure](../figures/faq_op_correspondence.png)
