# Normalization Configuration

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-16T09:10:43.417Z pushedAt=2026-10-19T12:37:33.436Z -->

Normalization is a processing method that scales data to a specific range, which facilitates subsequent data processing and analysis. The normalization function supported by AIPP is implemented through two steps: mean subtraction and coefficient multiplication. This processing method is not only applicable to conventional normalization requirements, but also flexibly supports conversion between multiple data formats, improving the efficiency and compatibility of data processing.

For example, when converting from uint8 to fp16, the conversion can be expressed as the following formula. In the formula, mean\_chn\_i represents the mean of each channel, min\_chn\_i represents the minimum value of each channel, and var\_reci\_chn\_i represents the reciprocal of the variance of each channel. These three values of each channel are parameters that need to be configured.

```text
pixel_out_chx(i)=[pixel_in_chx(i)-mean_chn_i-min_chn_i]*var_reci_chn_i
```
