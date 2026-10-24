
#!/usr/bin/env bash
# set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

run_ceedling_suite() {
    local suite="$1"
    echo
    echo "========== ${suite} =========="
    (
        cd "${ROOT}/test/${suite}"
        ceedling clobber
        ceedling test:all
        ceedling gcov:all
    )
}

echo "========== Unit tests =========="
run_ceedling_suite unit

echo "========== Integration tests =========="
run_ceedling_suite integration

echo "========== Static analysis =========="
"${ROOT}/test/static/run_static_checks.sh"

echo "========== UART fuzzing =========="
"${ROOT}/test/fuzz/build_and_run.sh" -max_total_time=30

echo
echo "All unit tests, integration tests, coverage tasks, static checks, and fuzz smoke test passed."
