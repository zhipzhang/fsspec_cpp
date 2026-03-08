#pragma once

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include "fsspec_cpp/fs.hpp"

namespace nb = nanobind;

namespace fsspec {
namespace python {

// Python fsspec 对象的 C++ 包装
class PyFileSystem;
class PyFile;

// 获取 Python 的 fsspec 模块引用
nb::module_ fsspec_module();
nb::module_ fsspec_core();

// 从 Python 异常转换
void throw_python_error();

} // namespace python
} // namespace fsspec
