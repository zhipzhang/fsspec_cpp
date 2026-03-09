#pragma once

#include "fsspec_cpp/fs.hpp"

// 前置声明 nanobind
namespace nanobind {
    class module_;
}

namespace fsspec {
namespace python {

// 获取 Python 的 fsspec 模块引用
// 返回 nanobind::module_ 供内部使用
nanobind::module_ fsspec_module();
nanobind::module_ fsspec_core();

// 从 Python 异常转换
void throw_python_error();

} // namespace python
} // namespace fsspec
