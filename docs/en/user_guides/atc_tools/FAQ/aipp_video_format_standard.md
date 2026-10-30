# How to Determine the Format Standard of a Video Stream When Using the AIPP Color Space Conversion Model

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:21:38.533Z pushedAt=2026-10-28T08:50:21.663Z nl=2 -->

## Symptoms

When using the AIPP color gamut conversion model, the format standard of the video stream cannot be determined.

## Possible Causes

The color space conversion in AIPP requires the video stream format to be one of the following: BT-709WIDE, BT-709NARROW, BT-601NARROW, and BT-601WIDE. However, users are often unsure which format a video stream belongs to, and therefore cannot determine which color space conversion template to use.

## Solution

The following uses the third-party **ffprobe** tool as an example to demonstrate how to determine the format.

1. Download the tool and related documents from the official path: [https://www.ffmpeg.org/ffprobe-all.html#Description](https://www.ffmpeg.org/ffprobe-all.html#Description).
2. Use the **ffprobe -show\_frames   _filename_** parameter to obtain the corresponding video information.****

    **Parameter function:** Show information about each frame and subtitle contained in the input multimedia stream. The information for each single frame is printed within a dedicated section with name "FRAME" or "SUBTITLE".

3. Determine the video standard based on the following information in the result.

    - "color\_range": "tv" or "pc".
    - "color\_space": "bt709" or "bt601".

    Here, tv indicates limited, that is, narrow range. pc indicates full, that is, wide range.

    For example, if the query result is color\_range=tv and color\_space=bt709, it indicates BT-709NARROW.

    > [!NOTE] Note
    > If the command changes, refer to the official tool documentation.
