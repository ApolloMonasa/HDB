# ============================================================================
# tools/CMakeLists.txt —— 配套调试工具
#
#   shell/               hsql           交互式 SQL 终端（Lab4 起可用）
#   b_plus_tree_printer/ b_plus_tree_printer B+ 树 DOT 图导出（Lab3 调试用）
#   sqllogictest/        hdb-sqllogictest    .slt 端到端测试 runner（Lab4 评分）
#
# TODO[维护者]：随骨架推进逐个启用
# ============================================================================

add_subdirectory(shell)
add_subdirectory(b_plus_tree_printer)
add_subdirectory(sqllogictest)
add_subdirectory(benchmark)
