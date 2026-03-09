cmake_minimum_required(VERSION 3.18)
project(fsspec_cpp VERSION 0.1.0 LANGUAGES CXX C)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 查找 Python
find_package(Python 3.8 COMPONENTS Interpreter Development Development.Module REQUIRED)

# 用 Python 查找 nanobind 路径
execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import nanobind; print(nanobind.include_dir())"
    OUTPUT_VARIABLE NANOBIND_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import nanobind; import os; print(os.path.dirname(nanobind.include_dir()))"
    OUTPUT_VARIABLE NANOBIND_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT NANOBIND_INCLUDE_DIR)
    message(FATAL_ERROR "nanobind not found. Please install: pip install nanobind")
endif()

message(STATUS "nanobind include: ${NANOBIND_INCLUDE_DIR}")
message(STATUS "nanobind dir: ${NANOBIND_DIR}")

# 手动设置 NB_DIR 供 nanobind_add_module 使用
set(NB_DIR ${NANOBIND_DIR} CACHE INTERNAL "nanobind directory")

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ============ C++ 核心库 ============
add_library(fsspec_cpp_core STATIC
    src/filesystem.cpp
    src/file.cpp
    src/bridge.cpp
)
target_link_libraries(fsspec_cpp_core PRIVATE Python::Module)
target_include_directories(fsspec_cpp_core PRIVATE ${NANOBIND_INCLUDE_DIR})
# 链接 nanobind 静态库
find_library(NANOBIND_LIB nanobind-static PATHS ${NANOBIND_DIR} ${NANOBIND_DIR}/lib NO_DEFAULT_PATH)
if(NANOBIND_LIB)
    target_link_libraries(fsspec_cpp_core PRIVATE ${NANOBIND_LIB})
else()
    target_link_libraries(fsspec_cpp_core PRIVATE nanobind-static)
endif()

# ============ Python 模块 ============
# 包含 nanobind cmake 模块
list(APPEND CMAKE_PREFIX_PATH ${NANOBIND_DIR}/cmake)
include(${NANOBIND_DIR}/cmake/nanobind-config.cmake)

nanobind_add_module(_fsspec_cpp
    src/bindings.cpp
)
target_link_libraries(_fsspec_cpp PRIVATE fsspec_cpp_core)

# ============ C API 共享库 (纯 C 实现) ============
add_library(fsspec_c SHARED
    src/fsspec_c_pure.c
)
target_link_libraries(fsspec_c PRIVATE 
    ${Python_LIBRARIES}
)
target_include_directories(fsspec_c PRIVATE ${Python_INCLUDE_DIRS})
set_target_properties(fsspec_c PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    PUBLIC_HEADER "include/fsspec_cpp/fsspec_c.h"
)

# Linux: 使用 GNU 扩展支持 fopencookie
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(fsspec_c PRIVATE _GNU_SOURCE)
endif()

# ============ 测试 ============
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    
    # C API 测试
    add_executable(test_c_api tests/test_c_api.c)
    target_link_libraries(test_c_api PRIVATE fsspec_c ${Python_LIBRARIES})
    target_include_directories(test_c_api PRIVATE ${Python_INCLUDE_DIRS})
    
    # 简单 Python 测试
    add_executable(test_simple tests/test_simple.c)
    target_link_libraries(test_simple PRIVATE fsspec_c ${Python_LIBRARIES})
    target_include_directories(test_simple PRIVATE ${Python_INCLUDE_DIRS})
    
    # doctest C++ 测试
    add_executable(test_cpp_api tests/test_cpp_api.cpp)
    target_link_libraries(test_cpp_api PRIVATE fsspec_cpp_core ${Python_LIBRARIES})
    target_include_directories(test_cpp_api PRIVATE 
        ${CMAKE_SOURCE_DIR}/third_party
        ${NANOBIND_INCLUDE_DIR}
        ${Python_INCLUDE_DIRS}
    )
    
    add_test(NAME c_api_test COMMAND test_c_api)
    add_test(NAME cpp_api_test COMMAND test_cpp_api)
endif()

# ============ 安装 ============
install(TARGETS _fsspec_cpp DESTINATION fsspec_cpp)
install(TARGETS fsspec_c 
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/fsspec_cpp
)
install(FILES 
    include/fsspec_cpp/fs.hpp
    include/fsspec_cpp/bridge.hpp
    DESTINATION include/fsspec_cpp
)
