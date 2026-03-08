cmake_minimum_required(VERSION 3.18)
project(fsspec_cpp VERSION 0.1.0 LANGUAGES CXX)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 查找 Python 和 nanobind
find_package(Python 3.8 COMPONENTS Interpreter Development.Module REQUIRED)
find_package(nanobind CONFIG REQUIRED)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 主库
add_library(fsspec_cpp_core STATIC
    src/filesystem.cpp
    src/file.cpp
    src/bridge.cpp
)
target_link_libraries(fsspec_cpp_core PRIVATE Python::Module)

# Python 模块
nanobind_add_module(_fsspec_cpp
    src/bindings.cpp
)
target_link_libraries(_fsspec_cpp PRIVATE fsspec_cpp_core)

# 安装
install(TARGETS _fsspec_cpp DESTINATION fsspec_cpp)
