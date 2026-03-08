#include "fsspec_cpp/fs.hpp"
#include "fsspec_cpp/bridge.hpp"
#include <nanobind/nanobind.h>

namespace nb = nanobind;

namespace fsspec {

class PyFileSystem;
class PyFile;

// Python 文件系统包装
class PyFileSystem : public FileSystem {
private:
    nb::object fs_obj_;
    
public:
    explicit PyFileSystem(nb::object fs_obj) : fs_obj_(fs_obj) {}
    
    nb::object py_object() const { return fs_obj_; }
    
    std::unique_ptr<File> open(const std::string& path, OpenMode mode) override;
    bool exists(const std::string& path) override;
    void remove(const std::string& path) override;
    void rename(const std::string& src, const std::string& dst) override;
    std::vector<FileInfo> ls(const std::string& path) override;
    void mkdir(const std::string& path, bool create_parents) override;
    void rmdir(const std::string& path) override;
    FileInfo info(const std::string& path) override;
    std::vector<FileInfo> glob(const std::string& pattern) override;
};

// Python 文件包装
class PyFile : public File {
private:
    nb::object file_obj_;
    bool closed_ = false;
    
public:
    explicit PyFile(nb::object file_obj) : file_obj_(file_obj) {}
    
    size_t read(void* buffer, size_t size) override {
        nb::gil_scoped_acquire acquire;
        auto data = file_obj_.attr("read")(size);
        nb::bytes bytes = nb::cast<nb::bytes>(data);
        size_t len = bytes.size();
        memcpy(buffer, bytes.data(), len);
        return len;
    }
    
    size_t write(const void* buffer, size_t size) override {
        nb::gil_scoped_acquire acquire;
        nb::bytes data(static_cast<const char*>(buffer), size);
        auto result = file_obj_.attr("write")(data);
        return nb::cast<size_t>(result);
    }
    
    void seek(int64_t pos, int whence) override {
        nb::gil_scoped_acquire acquire;
        file_obj_.attr("seek")(pos, whence);
    }
    
    int64_t tell() override {
        nb::gil_scoped_acquire acquire;
        return nb::cast<int64_t>(file_obj_.attr("tell")());
    }
    
    void close() override {
        nb::gil_scoped_acquire acquire;
        file_obj_.attr("close")();
        closed_ = true;
    }
    
    bool closed() const override {
        return closed_;
    }
};

// PyFileSystem 实现
std::unique_ptr<File> PyFileSystem::open(const std::string& path, OpenMode mode) {
    nb::gil_scoped_acquire acquire;
    std::string mode_str = "r";
    switch(mode) {
        case OpenMode::Read: mode_str = "r"; break;
        case OpenMode::Write: mode_str = "w"; break;
        case OpenMode::Append: mode_str = "a"; break;
        case OpenMode::ReadWrite: mode_str = "r+"; break;
    }
    auto f = fs_obj_.attr("open")(path, mode_str);
    return std::make_unique<PyFile>(f);
}

bool PyFileSystem::exists(const std::string& path) {
    nb::gil_scoped_acquire acquire;
    return nb::cast<bool>(fs_obj_.attr("exists")(path));
}

void PyFileSystem::remove(const std::string& path) {
    nb::gil_scoped_acquire acquire;
    fs_obj_.attr("rm")(path);
}

void PyFileSystem::rename(const std::string& src, const std::string& dst) {
    nb::gil_scoped_acquire acquire;
    fs_obj_.attr("mv")(src, dst);
}

std::vector<FileInfo> PyFileSystem::ls(const std::string& path) {
    nb::gil_scoped_acquire acquire;
    std::vector<FileInfo> result;
    auto entries = fs_obj_.attr("ls")(path);
    for (auto item : entries) {
        FileInfo info;
        // 处理 fsspec 返回的 dict 或字符串
        if (nb::isinstance<nb::dict>(item)) {
            nb::dict d = nb::cast<nb::dict>(item);
            info.name = nb::cast<std::string>(d["name"]);
            info.size = nb::cast<int64_t>(d.value_or("size", 0));
            info.is_dir = nb::cast<bool>(d.value_or("type", "file") == nb::str("directory"));
        } else {
            info.name = nb::cast<std::string>(item);
        }
        result.push_back(info);
    }
    return result;
}

void PyFileSystem::mkdir(const std::string& path, bool create_parents) {
    nb::gil_scoped_acquire acquire;
    fs_obj_.attr("mkdir")(path, create_parents);
}

void PyFileSystem::rmdir(const std::string& path) {
    nb::gil_scoped_acquire acquire;
    fs_obj_.attr("rmdir")(path);
}

FileInfo PyFileSystem::info(const std::string& path) {
    nb::gil_scoped_acquire acquire;
    auto d = fs_obj_.attr("info")(path);
    FileInfo info;
    info.name = nb::cast<std::string>(d["name"]);
    info.size = nb::cast<int64_t>(d.value_or("size", 0));
    info.is_dir = nb::cast<bool>(d.value_or("type", "file") == nb::str("directory"));
    info.mtime = nb::cast<double>(d.value_or("mtime", 0.0));
    return info;
}

std::vector<FileInfo> PyFileSystem::glob(const std::string& pattern) {
    nb::gil_scoped_acquire acquire;
    std::vector<FileInfo> result;
    auto paths = fs_obj_.attr("glob")(pattern);
    for (auto p : paths) {
        FileInfo info;
        info.name = nb::cast<std::string>(p);
        result.push_back(info);
    }
    return result;
}

// 从 URL 获取文件系统
std::unique_ptr<FileSystem> filesystem_from_url(const std::string& url) {
    nb::gil_scoped_acquire acquire;
    auto fsspec = python::fsspec_core();
    auto fs_obj = fsspec.attr("url_to_fs")(url);
    // url_to_fs 返回 (fs, path) 元组
    auto fs = nb::cast<nb::tuple>(fs_obj)[0];
    return std::make_unique<PyFileSystem>(fs);
}

// 便捷函数
std::unique_ptr<File> open(const std::string& url, const std::string& mode) {
    nb::gil_scoped_acquire acquire;
    auto fsspec = python::fsspec_core();
    auto f = fsspec.attr("open")(url, mode);
    return std::make_unique<PyFile>(f);
}

bool exists(const std::string& url) {
    auto fs = filesystem_from_url(url);
    return fs->exists(url);
}

void remove(const std::string& url) {
    auto fs = filesystem_from_url(url);
    fs->remove(url);
}

std::vector<FileInfo> ls(const std::string& url) {
    auto fs = filesystem_from_url(url);
    return fs->ls(url);
}

} // namespace fsspec
