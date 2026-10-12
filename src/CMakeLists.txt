# ============================================================================
# src/CMakeLists.txt —— hdb 内核静态库（组织方式对标 BusTub）
#
# 目录组织：
#   src/include/hdb/**  全部头文件（接口即"题目"，学生主要阅读这里）
#   src/**              实现文件（框架给定组件与学生填空桩混排）
#
# 每个模块目录一份 CMakeLists.txt，只列自己的源文件（分散所有权），
# 通过 hdb_module() 登记到全局注册表，最终链接成单一的 libhdb.a。
# ============================================================================

# hdb_module(<模块路径> <源文件...>)
#   模块路径相对 src/（可含 "/"，如 storage/page），OBJECT 库名为
#   hdb_<路径下划线化>。
function(hdb_module module)
  string(REPLACE "/" "_" _lib "${module}")
  add_library(hdb_${_lib} OBJECT ${ARGN})
  set_property(GLOBAL APPEND PROPERTY HDB_MODULE_LIBS "hdb_${_lib}")
endfunction()

add_subdirectory(common)
add_subdirectory(primer)
add_subdirectory(type)
add_subdirectory(buffer)
add_subdirectory(storage)
add_subdirectory(catalog)
add_subdirectory(sql)
add_subdirectory(execution)
add_subdirectory(optimizer)
add_subdirectory(concurrency)

# 汇总全部模块的目标文件，链接成单一静态库 libhdb.a（BusTub 同款机制）
get_property(HDB_MODULE_LIBS GLOBAL PROPERTY HDB_MODULE_LIBS)
set(HDB_ALL_OBJECTS)
foreach(_lib ${HDB_MODULE_LIBS})
  list(APPEND HDB_ALL_OBJECTS "$<TARGET_OBJECTS:${_lib}>")
endforeach()

add_library(hdb STATIC ${HDB_ALL_OBJECTS})

find_package(Threads REQUIRED)
target_link_libraries(hdb PUBLIC Threads::Threads)
target_include_directories(hdb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
