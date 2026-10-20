#!/bin/bash
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------


if [[ -z "$1" ]]; then
    echo -e "[ERROR] No source dir provided"
    exit 1
fi

if [[ -z "$2" ]]; then
    echo -e "[ERROR] No destination dir provided"
    exit 1
fi

src="$1"
dest_dir="$2"

if [[ ! -d "$src" ]]; then
    echo -e "[ERROR] Source dir does not exist: $src"
    exit 1
fi

if ! mkdir -p "$dest_dir"; then
    echo -e "[ERROR] Failed to create destination dir: $dest_dir"
    exit 1
fi

dest_file="$dest_dir/npu_supported_ops.json"

if [ -f "$dest_file" ];then
    chmod u+w "$dest_file"
fi

echo "$*"

find_op_types() {
  cur_dir="$1"
  find "$cur_dir" \( -name "*.cpp" -o -name "*.cc" -o -name "*.h" \) -print0 \
  | xargs -0 -r awk '
          /REG_AUTO_MAPPING_OP/ {
              match($0, /REG_AUTO_MAPPING_OP[[:space:]]*\(([^,)]+)/, a)
              gsub(/^[[:space:]]+|[[:space:]]+$/, "", a[1])
              print a[1]
          }
          '| sort -u
}
add_ops() {
    name=$1
    isHeavy=$2
    file=$3
    grep -w "\"$name\"" "$file" >/dev/null
    if [ $?  == 0 ];then
        return
    fi
    echo "  \"${name}\": {" >> "${file}"
    echo "    \"isGray\": false," >> "${file}"
    echo "    \"isHeavy\": ${isHeavy}" >> "${file}"
    echo "  }," >> "${file}"
}

echo "{" > "$dest_file" || exit 1
find_op_types "$src" | while IFS= read -r op; do
    echo "op = $op"
    add_ops "$op" "false" "$dest_file"
done
echo "}" >> "$dest_file" || exit 1
file_count=$(wc -l < "$dest_file")
line=$(($file_count-1))
sed -i "${line}{s/,//g}" "$dest_file" || exit 1

chmod 640 "${dest_file}"
echo "[INFO] Succeed generated ${dest_file}"

exit 0
