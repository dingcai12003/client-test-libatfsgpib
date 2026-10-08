#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

. "$SCRIPT_DIR/atfs-env.sh"

GPIB_TEST_CONFIG="${GPIB_TEST_CONFIG:-$SCRIPT_DIR/config/gpib-test.conf}"

if [ "${ATFS_PRINT_ENV:-0}" = "1" ]; then
  atfs_print_environment
fi

if [ "${ATFS_SKIP_CHECKS:-0}" != "1" ]; then
  atfs_check_runtime_paths
fi

if [ "${ATFS_SKIP_SYMBOL_CHECK:-0}" != "1" ] && command -v nm >/dev/null 2>&1; then
  if ! nm -D "$SCRIPT_DIR/gpib-test" 2>/dev/null | grep -q 'UTHN_Handle_FindArea'; then
    cat >&2 <<'EOF'
gpib-test does not export UTHN_Handle_FindArea.
Rebuild it on the target machine so libatfshn.a is linked into the executable:

  make clean
  make
  make verify-atfs-symbols

Set ATFS_SKIP_SYMBOL_CHECK=1 only for temporary diagnostics.
EOF
    exit 2
  fi
fi

if [ "${ATFS_START_GPIB:-0}" = "1" ]; then
  atfs_check_service_paths
  atfs_start_gpib_service
fi

exec "$SCRIPT_DIR/gpib-test" --config "$GPIB_TEST_CONFIG" "$@"
