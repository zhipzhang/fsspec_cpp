#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>
#include "fsspec_cpp/fs.hpp"

namespace nb = nanobind;

using namespace fsspec;

NB_MODULE(_fsspec_cpp, m) {
    m.doc() = "fsspec C++ bindings via nanobind";

    // OpenMode 枚举
    nb::enum_<OpenMode>(m, "OpenMode")
        .value("Read", OpenMode::Read)
        .value("Write", OpenMode::Write)
        .value("Append", OpenMode::Append)
        .value("ReadWrite", OpenMode::ReadWrite);

    // FileInfo 结构体
    nb::class_<FileInfo>(m, "FileInfo")
        .def_ro("path", &FileInfo::path)
        .def_ro("name", &FileInfo::name)
        .def_ro("size", &FileInfo::size)
        .def_ro("is_dir", &FileInfo::is_dir)
        .def_ro("mtime", &FileInfo::mtime)
        .def_ro("protocol", &FileInfo::protocol);

    // File 类（抽象基类）
    nb::class_<File>(m, "File")
        .def(
            "read",
            [](File& self, size_t size) {
                std::string buffer(size, '\0');
                size_t n = self.read(buffer.data(), size);
                buffer.resize(n);
                return buffer;
            },
            nb::arg("size"))
        .def("read_all", &File::read_all)
        .def(
            "write",
            [](File& self, const std::string& data) {
                return self.write(data.data(), data.size());
            },
            nb::arg("data"))
        .def("write_all", &File::write_all)
        .def("seek", &File::seek, nb::arg("pos"), nb::arg("whence") = 0)
        .def("tell", &File::tell)
        .def("close", &File::close)
        .def("closed", &File::closed);

    // FileSystem 类（抽象基类）
    nb::class_<FileSystem>(m, "FileSystem")
        .def("open", &FileSystem::open, nb::arg("path"), nb::arg("mode") = OpenMode::Read)
        .def("exists", &FileSystem::exists)
        .def("remove", &FileSystem::remove)
        .def("rename", &FileSystem::rename)
        .def("ls", &FileSystem::ls)
        .def("mkdir", &FileSystem::mkdir, nb::arg("path"), nb::arg("create_parents") = true)
        .def("rmdir", &FileSystem::rmdir)
        .def("info", &FileSystem::info)
        .def("glob", &FileSystem::glob);

    // 便捷函数 - 使用 lambda 避免函数指针歧义
    m.def("filesystem_from_url", [](const std::string& url) { return filesystem_from_url(url); });

    m.def(
        "open",
        [](const std::string& url, const std::string& mode) { return fsspec::open(url, mode); },
        nb::arg("url"), nb::arg("mode") = "r");

    m.def("exists", [](const std::string& url) { return fsspec::exists(url); });

    m.def("remove", [](const std::string& url) { fsspec::remove(url); });

    m.def("ls", [](const std::string& url) { return fsspec::ls(url); });
}
