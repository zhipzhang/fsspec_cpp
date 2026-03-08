#include "fsspec_cpp/fs.hpp"
#include "fsspec_cpp/bridge.hpp"

namespace fsspec {
namespace python {

namespace {
    // 线程局部存储 Python 模块引用
    thread_local nb::module_ fsspec_mod;
    thread_local nb::module_ fsspec_core_mod;
}

nb::module_ fsspec_module() {
    if (!fsspec_mod) {
        fsspec_mod = nb::module_::import("fsspec");
    }
    return fsspec_mod;
}

nb::module_ fsspec_core() {
    if (!fsspec_core_mod) {
        fsspec_core_mod = nb::module_::import("fsspec.core");
    }
    return fsspec_core_mod;
}

void throw_python_error() {
    if (PyErr_Occurred()) {
        PyErr_Print();
        throw std::runtime_error("Python exception occurred");
    }
}

} // namespace python
} // namespace fsspec
