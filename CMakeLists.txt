# ============================================================================
# HDB —— 杭电《数据库原理》实验内核框架
#
# 教学思路参考 CMU 15-445 的 BusTub（https://github.com/cmu-db/bustub），
# 构建方式与其保持一致：
#   cmake -B build
#   cmake --build build -j$(nproc)
# 详细用法见 README.md。
# ============================================================================

cmake_minimum_required(VERSION 3.16)

project(hdb LANGUAGES CXX)

# 基线：C++17，GCC 9+ / Clang 10+ 均可编译（实验室环境友好）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认 Debug 构建，方便学生用 gdb / sanitizer 排查问题
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# 防呆：不要在仓库根目录下直接 cmake .（BusTub 同款检查），
# 否则会在根目录生成 CMakeCache.txt / CMakeFiles 污染源码树。
if(EXISTS "${CMAKE_BINARY_DIR}/CMakeLists.txt")
  message(FATAL_ERROR "Run CMake in a build subdirectory! \"cmake -B build\" \
Some junk files were created in this folder (CMakeCache.txt, CMakeFiles); you should delete those.")
endif()

# ----------------------------------------------------------------------------
# 用户可配置选项（可用 -DXXX=ON/OFF 覆盖）
# ----------------------------------------------------------------------------
option(HDB_BUILD_TESTS "构建单元测试（依赖系统 GTest）" ON)
option(HDB_BUILD_TOOLS "构建 hsql / b_plus_tree_printer 等工具" ON)
option(HDB_WERROR "把编译警告当作错误（默认关闭，交由课程自行决定是否开启）" OFF)
# Sanitizer 选择（Debug 构建生效）：address = 内存错误（默认）；thread = 数据竞争（Lab3/5 并发调试）
set(HDB_SANITIZER "address" CACHE STRING "address | thread | none")

# ----------------------------------------------------------------------------
# 全局编译选项
# ----------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra)
  if(HDB_WERROR)
    add_compile_options(-Werror)
  endif()
endif()

# Debug + Sanitizer：内存错误（address）或数据竞争（thread）在测试运行时直接暴露。
#   cmake -B build -DHDB_SANITIZER=thread   # Lab3 蟹行 / Lab5 锁实验推荐
# 注意：address 与 thread 互斥，同一构建目录只开一个。
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT HDB_SANITIZER STREQUAL "none")
  add_compile_options(-fsanitize=${HDB_SANITIZER} -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=${HDB_SANITIZER})
endif()

# 产物目录（BusTub 约定）：可执行文件进 build/bin，测试进 build/test，
# 静态库进 build/lib。
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 头文件搜索路径（目录级，供各模块 OBJECT 库直接使用，BusTub 同款）
include_directories(${PROJECT_SOURCE_DIR}/src/include)

# ----------------------------------------------------------------------------
# 代码风格：cmake --build build --target format（未装 clang-format 时给提示）
# ----------------------------------------------------------------------------
find_program(CLANG_FORMAT_BIN clang-format)
if(CLANG_FORMAT_BIN)
  add_custom_target(format
    COMMAND ${CLANG_FORMAT_BIN} -i
            ${CMAKE_CURRENT_SOURCE_DIR}/src/include/hdb/*.h
            ${CMAKE_CURRENT_SOURCE_DIR}/src/include/hdb/*/*.h
            ${CMAKE_CURRENT_SOURCE_DIR}/src/include/hdb/*/*/*.h
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/test/*/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/*/*.cpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running clang-format (style: .clang-format)")
else()
  add_custom_target(format
    COMMAND ${CMAKE_COMMAND} -E echo
            "clang-format not found. Install: Fedora 'sudo dnf install clang-tools-extra' or Ubuntu 'sudo apt install clang-format'")
endif()

# ----------------------------------------------------------------------------
# 子模块
# ----------------------------------------------------------------------------
enable_testing()

add_subdirectory(src)

if(HDB_BUILD_TESTS)
  add_subdirectory(test)
endif()

if(HDB_BUILD_TOOLS)
  add_subdirectory(tools)
endif()

# 构建摘要，帮助学生确认自己的配置是否符合预期
message(STATUS "")
message(STATUS "[HDB] build type       : ${CMAKE_BUILD_TYPE}")
message(STATUS "[HDB] compiler         : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "[HDB] C++ standard     : ${CMAKE_CXX_STANDARD}")
message(STATUS "[HDB] unit tests       : ${HDB_BUILD_TESTS}")
message(STATUS "[HDB] tools            : ${HDB_BUILD_TOOLS}")
message(STATUS "[HDB] sanitizer(Debug) : ${HDB_SANITIZER}")
message(STATUS "")
