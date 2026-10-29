#!/usr/bin/env bash
set -e

source_file=$1
source_dir=$(dirname -- "$source_file")
source_name=$(basename -- "$source_file" .c)
program="$source_dir/$source_name"

gcc -Wall -Wextra -g -O0 "$source_file" -o "$program"

printf 'argv[1] に渡す式（空欄なら引数なし）: '
IFS= read -r expression

if [[ -n $expression ]]; then
    "$program" "$expression"
else
    "$program"
fi
