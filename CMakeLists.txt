cmake_minimum_required(VERSION 3.18)
project(fsspec_cpp VERSION 0.1.0 LANGUAGES C CXX)

# C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 查找 Python
find_package(Python 3.8 COMPONENTS Interpreter Development Development.Module REQUIRED)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ============ C API 共享库 ============
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
    
    # doctest C++ 测试
    add_executable(test_cpp_api tests/test_cpp_api.cpp)
    target_link_libraries(test_cpp_api PRIVATE fsspec_c ${Python_LIBRARIES})
    target_include_directories(test_cpp_api PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party
        ${Python_INCLUDE_DIRS}
    )
    
    add_test(NAME c_api_test COMMAND test_c_api)
    add_test(NAME cpp_api_test COMMAND test_cpp_api)
endif()

# ============ 安装 ============
install(TARGETS fsspec_c 
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/fsspec_cpp
)
