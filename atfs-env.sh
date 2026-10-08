#!/usr/bin/env sh

# Runtime defaults for Advantest FutureSuite ATFS GPIB.
# Override any value before sourcing this file if the installation differs.

: "${ATFSROOT:=/opt/ATFS}"
: "${ATFSARCH:=i386}"
: "${ATFSOS:=linux}"
: "${ATFSSYS:=ATFSsys-4.05F1}"
: "${ATFSSYSTEM:=${ATFSROOT}/${ATFSARCH}/${ATFSOS}}"
: "${ATFSVAROPT:=/var/opt}"
ATFS_LIB_DIR="${ATFSSYSTEM}/${ATFSSYS}/lib"
: "${GPIB_LIBRARY_PATH:=${ATFS_LIB_DIR}/libatfsgpib.so}"
: "${ATFSGPIBNAME:=127.0.0.1:9910,127.0.0.1:9911}"

export ATFSROOT ATFSARCH ATFSOS ATFSSYS ATFSSYSTEM ATFSVAROPT ATFSGPIBNAME
export ATFS_LIB_DIR GPIB_LIBRARY_PATH

case ":${LD_LIBRARY_PATH:-}:" in
  *":${ATFS_LIB_DIR}:"*) ;;
  *) LD_LIBRARY_PATH="${ATFS_LIB_DIR}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" ;;
esac
export LD_LIBRARY_PATH

atfs_print_environment() {
  printf 'ATFSROOT=%s\n' "$ATFSROOT"
  printf 'ATFSARCH=%s\n' "$ATFSARCH"
  printf 'ATFSOS=%s\n' "$ATFSOS"
  printf 'ATFSSYS=%s\n' "$ATFSSYS"
  printf 'ATFSSYSTEM=%s\n' "$ATFSSYSTEM"
  printf 'ATFSVAROPT=%s\n' "$ATFSVAROPT"
  printf 'ATFSGPIBNAME=%s\n' "$ATFSGPIBNAME"
  printf 'ATFS_LIB_DIR=%s\n' "$ATFS_LIB_DIR"
  printf 'GPIB_LIBRARY_PATH=%s\n' "$GPIB_LIBRARY_PATH"
  printf 'LD_LIBRARY_PATH=%s\n' "$LD_LIBRARY_PATH"
}

atfs_require_path() {
  if [ ! -e "$1" ]; then
    printf 'missing required ATFS path: %s\n' "$1" >&2
    return 1
  fi
  return 0
}

atfs_check_runtime_paths() {
  failed=0
  atfs_require_path "${GPIB_LIBRARY_PATH}" || failed=1
  atfs_require_path "${ATFSSYSTEM}/${ATFSSYS}/etc/sysconfig/gpib_system.conf" || failed=1
  atfs_require_path "${ATFSVAROPT}/ATFS/${ATFSSYS}/gpib.conf" || failed=1
  return "$failed"
}

atfs_check_service_paths() {
  failed=0
  atfs_require_path "${ATFSSYSTEM}/${ATFSSYS}/bin/fsservice" || failed=1
  atfs_require_path "${ATFSSYSTEM}/${ATFSSYS}/etc/sysconfig/utsc_gpib.rc" || failed=1
  atfs_require_path "${ATFSSYSTEM}/${ATFSSYS}/etc/sysconfig/service.conf" || failed=1
  atfs_require_path "${ATFSVAROPT}/ATFS/${ATFSSYS}/rc" || failed=1
  return "$failed"
}

atfs_start_gpib_service() {
  "${ATFSSYSTEM}/${ATFSSYS}/bin/fsservice" --list
  "${ATFSSYSTEM}/${ATFSSYS}/bin/fsservice" utsc_gpib.rc start
}
