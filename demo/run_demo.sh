#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET_FILE="$ROOT_DIR/demo/text.txt"

printf '%s\n' 'bat bat bat' > "$TARGET_FILE"

echo "Demo file content before load:"
cat "$TARGET_FILE"

echo

echo "Build the module with:"
echo "  make"
echo "Then load it with:"
echo "  sudo insmod $ROOT_DIR/bat_to_cat.ko"
echo "Finally run:"
echo "  cat $TARGET_FILE"
echo "Expected output: cat cat cat"
