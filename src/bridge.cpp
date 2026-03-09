#include "fsspec_cpp/fs.hpp"
#include "fsspec_cpp/bridge.hpp"

#include <nanobind/nanobind.h>

namespace nb = nanobind;

namespace fsspec {
namespace python {

namespace {
    // 使用简单的 PyObject* 指针存储，避免 C++ 构造问题
    thread_local PyObject* fsspec_mod = nullptr;
    thread_local PyObject* fsspec_core_mod = nullptr;
}

nb::module_ fsspec_module() {
    nb::gil_scoped_acquire acquire;
    if (!fsspec_mod) {
        fsspec_mod = PyImport_ImportModule("fsspec");
        if (!fsspec_mod) {
            throw_python_error();
        }
    }
    return nb::borrow<nb::module_>(fsspec_mod);
}

nb::module_ fsspec_core() {
    nb::gil_scoped_acquire acquire;
    if (!fsspec_core_mod) {
        fsspec_core_mod = PyImport_ImportModule("fsspec.core");
        if (!fsspec_core_mod) {
            throw_python_error();
        }
    }
    return nb::borrow<nb::module_>(fsspec_core_mod);
}

void throw_python_error() {
    if (PyErr_Occurred()) {
        PyErr_Print();
        throw std::runtime_error("Python exception occurred");
    }
}

} // namespace python
} // namespace fsspec
