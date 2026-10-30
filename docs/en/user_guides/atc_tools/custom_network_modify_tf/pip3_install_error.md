# "Could not find a version that satisfies the requirement xxx" Is Displayed When Installing Software Using pip3 install

<!-- md-trans-meta sourceCommit=84719ef8084affb0771904e2525c615132365471 translatedAt=2026-10-27T14:20:01.055Z pushedAt=2026-10-28T08:14:26.064Z nl=2 -->

## Problem Description

When installing dependencies, using the **pip3 install xxx** command to install related software prompts that the network cannot be connected, and the message "Could not find a version that satisfies the requirement xxx" is displayed. The **apt-get update** command is used to check that the source is available. The prompt message is shown in the following figure.

**Figure 1**  Prompt message for installing software using pip3.7.5
![Illustration](../figures/pip3_7_5_install_message.png "Prompt message for installing software using pip3-7-5")

## Possible Causes

No pip source is configured.

## Solution

Configure the pip source as follows:

1. As the user who installs the ATC software package, run the following command:

    ```bash
    cd ~/.pip
    ```

    If the directory does not exist, run the following command to create it:

    ```bash
    mkdir ~/.pip
    cd ~/.pip
    ```

    Create the pip.conf file in the .pip directory by running the following command:

    ```bash
    touch pip.conf
    ```

2. Edit the pip.conf file.

    Use the **vi pip.conf** command to open the pip.conf file, write the following content, and then save the file and exit:

    ```ini
    [global]
    #Available source. Replace it based on the actual situation.
    index-url=http://xxx
    [install]
    #Trusted host. Replace it based on the actual situation.
    trusted-host=xxx
    ```
