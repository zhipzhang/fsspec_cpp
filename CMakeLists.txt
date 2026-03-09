cmake_minimum_required(VERSION 3.18)
project(fsspec_cpp VERSION 0.1.0 LANGUAGES CXX C)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 查找 Python 和 nanobind
find_package(Python 3.8 COMPONENTS Interpreter Development.Module REQUIRED)
find_package(nanobind CONFIG REQUIRED)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ============ C++ 核心库 ============
add_library(fsspec_cpp_core STATIC
    src/filesystem.cpp
    src/file.cpp
    src/bridge.cpp
)
target_link_libraries(fsspec_cpp_core PRIVATE Python::Module)

# ============ Python 模块 ============
nanobind_add_module(_fsspec_cpp
    src/bindings.cpp
)
target_link_libraries(_fsspec_cpp PRIVATE fsspec_cpp_core)

# ============ C API 共享库 ============
add_library(fsspec_c SHARED
    src/fsspec_c.cpp
    src/fsspec_fileptr.cpp
)
target_link_libraries(fsspec_c PRIVATE 
    fsspec_cpp_core
    Python::Module
)
set_target_properties(fsspec_c PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    PUBLIC_HEADER "include/fsspec_cpp/fsspec_c.h"
)

# Linux: 使用 GNU 扩展支持 fopencookie
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(fsspec_c PRIVATE _GNU_SOURCE)
endif()

# ============ C API 测试 ============
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    enable_testing()
    
    add_executable(test_c_api tests/test_c_api.c)
    target_link_libraries(test_c_api PRIVATE fsspec_c)
    # 需要链接到 Python
    target_link_libraries(test_c_api PRIVATE ${Python_LIBRARIES})
    
    add_test(NAME c_api_test COMMAND test_c_api)
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
