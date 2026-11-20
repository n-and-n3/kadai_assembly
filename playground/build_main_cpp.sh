#!/usr/bin/env bash
set -euo pipefail

if (( $# < 1 || $# > 2 )); then
    echo "Usage: $0 SOURCE_FILE [OUTPUT_CPP]" >&2
    exit 2
fi

source_file=$1
if [[ ! -f $source_file ]]; then
    echo "Source file not found: $source_file" >&2
    exit 1
fi

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_dir=$(dirname -- "$script_dir")
output_cpp=${2:-"$repo_dir/Main.cpp"}
temp_dir=$(mktemp -d)
trap 'rm -rf -- "$temp_dir"' EXIT

gcc -std=c11 -Wall -Wextra -Werror \
    "$repo_dir/25B31026/calc4.c" -o "$temp_dir/calc4"

source_text=$(cat -- "$source_file")
"$temp_dir/calc4" "$source_text" > "$temp_dir/generated.s"

python3 "$repo_dir/25B31026/codes/gas_to_cpp.py" "$temp_dir/generated.s" \
    --source-file "$source_file" -o "$output_cpp"

echo "Created $output_cpp"
