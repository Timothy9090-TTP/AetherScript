#!/usr/bin/env bash
# Universal C++ build.
#   - находит все *.cpp в проекте (кроме build/, out/, .git/, .vscode/)
#   - добавляет -I для каждой папки include/ и inc/
#   - кладёт бинарь в корень проекта
# Usage: build.sh [run]
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CXX="${CXX:-g++}"
STD="${STD:-c++23}"
OUT="${OUT:-aether}"

mapfile -t SOURCES < <(
    find . -type f -name '*.cpp' \
        -not -path './build/*' -not -path './out/*' \
        -not -path './.git/*'  -not -path './.vscode/*' \
        | sort
)

if [ "${#SOURCES[@]}" -eq 0 ]; then
    echo "error: no .cpp files found under $ROOT" >&2
    exit 1
fi

mapfile -t INCLUDES < <(
    find . -type d \( -name include -o -name inc \) \
        -not -path './build/*' -not -path './out/*' \
        -not -path './.git/*'  -not -path './.vscode/*' \
        | sed 's/^/-I/' | sort -u
)

echo ">> $CXX -std=$STD -g -Wall -Wextra ${INCLUDES[*]} <${#SOURCES[@]} sources> -o $OUT"
"$CXX" -std="$STD" -g -Wall -Wextra "${INCLUDES[@]}" "${SOURCES[@]}" -o "$OUT"

if [ "${1:-}" = "run" ]; then
    exec "./$OUT"
fi