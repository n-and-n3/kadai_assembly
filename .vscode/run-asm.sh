#!/usr/bin/env bash
set -e

source_file=$1
source_dir=$(dirname -- "$source_file")
source_name=$(basename -- "$source_file")
program="$source_dir/${source_name%.*}.out"

gcc -g -O0 "$source_file" -o "$program"
"$program"
