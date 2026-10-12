# ============================================================================
# test/CMakeLists.txt —— 测试组织（对标 BusTub）
#
#   test/<模块>/*_test.cpp   每个测试文件编译成独立可执行程序（GLOB 自动
#                           收集，新增测试文件无需改本文件），产物在
#                           build/test/ 下，单独构建：
#                             cmake --build build --target lru_k_replacer_test
#   test/sql/*.slt           端到端 SQL 用例，由 hdb-sqllogictest 执行，
#                           每个文件生成一个 SQLLogicTest.<名> 的 CTest 条目
#                           和 <名>_test 构建目标。
#   全部注册进 CTest：ctest --test-dir build 一键全跑。
#
# GTest 依赖：优先 find_package(GTest)，找不到时回退 pkg-config，
# 仍找不到则给出安装指引后终止。
#   Ubuntu/Debian: sudo apt install libgtest-dev
#   Fedora:        sudo dnf install gtest-devel
# ============================================================================

find_package(GTest QUIET)

if(NOT GTest_FOUND)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(GTEST gtest)
    if(GTEST_FOUND)
      set(GTEST_INCLUDE_DIRS ${GTEST_INCLUDE_DIRS})
      set(GTEST_LIBRARIES ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES})
      include_directories(${GTEST_INCLUDE_DIRS})
      set(HDB_GTEST_LIBS ${GTEST_LIBRARIES})
    endif()
  endif()
else()
  if(TARGET GTest::gtest_main)
    set(HDB_GTEST_LIBS GTest::gtest GTest::gtest_main)
  else()
    set(HDB_GTEST_LIBS ${GTEST_LIBRARIES} ${GTEST_MAIN_LIBRARIES})
    include_directories(${GTEST_INCLUDE_DIRS})
  endif()
endif()

if(NOT GTest_FOUND AND NOT GTEST_FOUND)
  message(FATAL_ERROR
    "[HDB] 未找到 Google Test，无法构建单元测试。安装方法：\n"
    "      Ubuntu/Debian : sudo apt install libgtest-dev\n"
    "      Fedora        : sudo dnf install gtest-devel\n"
    "      或者关闭测试   : cmake -B build -DHDB_BUILD_TESTS=OFF")
endif()

# ---------------------------------------------------------------------------
# GTest 单元测试：test/<模块>/<组件>_test.cpp → 目标 <组件>_test
# ---------------------------------------------------------------------------
file(GLOB_RECURSE HDB_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*test.cpp")

foreach(_src ${HDB_TEST_SOURCES})
  get_filename_component(_name ${_src} NAME_WE)
  add_executable(${_name} ${_src})
  target_link_libraries(${_name} PRIVATE hdb ${HDB_GTEST_LIBS})
  target_include_directories(${_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  set_target_properties(${_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test")
  add_test(NAME ${_name} COMMAND ${_name})
endforeach()

# ---------------------------------------------------------------------------
# SQL 端到端用例：test/sql/*.slt → CTest 条目 + <文件名>_test 目标
# ---------------------------------------------------------------------------
file(GLOB HDB_SLT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/sql/*.slt")

foreach(_slt ${HDB_SLT_SOURCES})
  get_filename_component(_slt_name ${_slt} NAME)
  string(REPLACE ".slt" "" _slt_target ${_slt_name})
  add_test(NAME SQLLogicTest.${_slt_target} COMMAND hdb-sqllogictest ${_slt})
  add_custom_target(${_slt_target}_test COMMAND hdb-sqllogictest ${_slt})
  add_dependencies(${_slt_target}_test hdb-sqllogictest)
endforeach()
