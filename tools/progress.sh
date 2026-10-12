#!/usr/bin/env bash
# ============================================================================
# progress.sh —— 实验进度自查
#
# 跑一遍全部单元测试，按 Lab 汇总用例通过率，一眼看到"离完成还有多远"。
#
# 用法：./tools/progress.sh [build 目录，默认 build]
# ============================================================================
set -uo pipefail

BUILD="${1:-build}"
TESTDIR="${BUILD}/test"

if [ ! -d "${TESTDIR}" ]; then
    echo "未找到 ${TESTDIR}——先构建：cmake -B build && cmake --build build --parallel" >&2
    exit 1
fi

# 测试可执行文件 -> Lab 归属
lab_of() {
    case "$1" in
        trie_test | trie_store_test)                echo 0 ;;
        disk_manager_test | lru_k_replacer_test | disk_scheduler_test | \
        buffer_pool_manager_test | page_guard_test) echo 1 ;;
        tuple_test | table_page_test | table_heap_test) echo 2 ;;
        b_plus_tree_*_test)                         echo 3 ;;
        parser_test | executor_test)                echo 4 ;;
        lock_manager_test | transaction_test)       echo 5 ;;
        mvcc_test)                                  echo 6 ;;
        *)                                          echo "?" ;;
    esac
}

lab_title=(["0"]="Lab0 Trie" ["1"]="Lab1 缓冲池" ["2"]="Lab2 记录存储" ["3"]="Lab3 B+树"
           ["4"]="Lab4 查询执行" ["5"]="Lab5 事务" ["6"]="Lab6 MVCC(选做)")

declare -A LAB_PASS LAB_TOTAL
TOTAL_PASS=0; TOTAL_CASES=0

printf "%-16s %-28s %8s\n" "Lab" "测试" "通过"
printf '%s\n' "-----------------------------------------------------------"

for bin in "${TESTDIR}"/*_test; do
    name=$(basename "${bin}")
    [ -x "${bin}" ] || continue
    lab=$(lab_of "${name}")
    # timeout 防死锁用例挂起；gtest 崩溃时 Running 行已先打印
    out=$(timeout 30 "${bin}" 2>/dev/null; true)
    total=$(sed -n 's/.*Running \([0-9]*\) tests.*/\1/p' <<<"${out}" | head -1)
    pass=$(sed -n 's/.*\[  PASSED  \] \([0-9]*\) test.*/\1/p' <<<"${out}" | tail -1)
    total=${total:-0}; pass=${pass:-0}
    if [ "${lab}" != "?" ]; then
        LAB_PASS[$lab]=$(( ${LAB_PASS[$lab]:-0} + pass ))
        LAB_TOTAL[$lab]=$(( ${LAB_TOTAL[$lab]:-0} + total ))
    fi
    TOTAL_PASS=$((TOTAL_PASS + pass))
    TOTAL_CASES=$((TOTAL_CASES + total))
    mark=""; [ "${pass}" -eq "${total}" ] && [ "${total}" -gt 0 ] && mark="  ✓"
    printf "%-16s %-28s %4s/%-4s%s\n" "${lab_title[$lab]:-其他}" "${name}" "${pass}" "${total}" "${mark}"
done

printf '%s\n' "-----------------------------------------------------------"
for i in 0 1 2 3 4 5 6; do
    [ "${LAB_TOTAL[$i]:-0}" -eq 0 ] && continue
    pct=$(( LAB_PASS[$i] * 100 / LAB_TOTAL[$i] ))
    bar=$(printf '%*s' $((pct / 5)) '' | tr ' ' '#')
    printf "%-16s %3d/%-3d %3d%% [%-20s]\n" "${lab_title[$i]}" \
        "${LAB_PASS[$i]}" "${LAB_TOTAL[$i]}" "${pct}" "${bar}"
done
printf '%s\n' "-----------------------------------------------------------"
echo "总进度: ${TOTAL_PASS}/${TOTAL_CASES} 用例通过"
