// fsspec_c.cpp - C API 实现
#include "fsspec_cpp/fsspec_c.h"
#include "fsspec_cpp/fs.hpp"
#include "fsspec_cpp/bridge.hpp"

#include <nanobind/nanobind.h>
#include <cstring>
#include <string>

namespace nb = nanobind;

// 线程局部存储错误信息
thread_local char g_last_error[1024] = {0};

static void set_error(const char* msg) {
    strncpy(g_last_error, msg, sizeof(g_last_error) - 1);
    g_last_error[sizeof(g_last_error) - 1] = '\0';
}

static void set_exception_error() {
    try {
        throw;
    } catch (const std::exception& e) {
        set_error(e.what());
    } catch (...) {
        set_error("Unknown error");
    }
}

// 包装结构体 - 持有 C++ 对象
struct fsspec_file_s {
    std::unique_ptr<fsspec::File> cpp_file;
    bool closed = false;
};

struct fsspec_fs_s {
    std::unique_ptr<fsspec::FileSystem> cpp_fs;
};

// ============ 初始化 ============

int fsspec_init(void) {
    try {
        if (!Py_IsInitialized()) {
            Py_Initialize();
        }
        // 预热 fsspec 模块
        nb::gil_scoped_acquire acquire;
        fsspec::python::fsspec_module();
        return 0;
    } catch (...) {
        set_exception_error();
        return -1;
    }
}

void fsspec_cleanup(void) {
    // Python 解释器由进程生命周期管理，通常不需要显式清理
    // 如果需要：Py_Finalize();
}

const char* fsspec_last_error(void) {
    return g_last_error[0] ? g_last_error : NULL;
}

// ============ 文件系统 ============

fsspec_fs_t* fsspec_fs_from_url(const char* url) {
    try {
        nb::gil_scoped_acquire acquire;
        auto fs = fsspec::filesystem_from_url(url);
        fsspec_fs_t* result = new fsspec_fs_t;
        result->cpp_fs = std::move(fs);
        return result;
    } catch (...) {
        set_exception_error();
        return NULL;
    }
}

void fsspec_fs_free(fsspec_fs_t* fs) {
    delete fs;
}

bool fsspec_fs_exists(fsspec_fs_t* fs, const char* path) {
    if (!fs) return false;
    try {
        nb::gil_scoped_acquire acquire;
        return fs->cpp_fs->exists(path);
    } catch (...) {
        set_exception_error();
        return false;
    }
}

int fsspec_fs_remove(fsspec_fs_t* fs, const char* path) {
    if (!fs) return -1;
    try {
        nb::gil_scoped_acquire acquire;
        fs->cpp_fs->remove(path);
        return 0;
    } catch (...) {
        set_exception_error();
        return -1;
    }
}

int fsspec_fs_rename(fsspec_fs_t* fs, const char* src, const char* dst) {
    if (!fs) return -1;
    try {
        nb::gil_scoped_acquire acquire;
        fs->cpp_fs->rename(src, dst);
        return 0;
    } catch (...) {
        set_exception_error();
        return -1;
    }
}

// ============ 文件操作 ============

fsspec_file_t* fsspec_fs_open(fsspec_fs_t* fs, const char* path, fsspec_mode_t mode) {
    if (!fs) return NULL;
    try {
        nb::gil_scoped_acquire acquire;
        fsspec::OpenMode cpp_mode;
        switch (mode) {
            case FSSPEC_MODE_READ: cpp_mode = fsspec::OpenMode::Read; break;
            case FSSPEC_MODE_WRITE: cpp_mode = fsspec::OpenMode::Write; break;
            case FSSPEC_MODE_APPEND: cpp_mode = fsspec::OpenMode::Append; break;
            case FSSPEC_MODE_READWRITE: cpp_mode = fsspec::OpenMode::ReadWrite; break;
            default: cpp_mode = fsspec::OpenMode::Read;
        }
        
        auto file = fs->cpp_fs->open(path, cpp_mode);
        fsspec_file_t* result = new fsspec_file_t;
        result->cpp_file = std::move(file);
        return result;
    } catch (...) {
        set_exception_error();
        return NULL;
    }
}

fsspec_file_t* fsspec_open(const char* url, const char* mode) {
    try {
        nb::gil_scoped_acquire acquire;
        auto file = fsspec::open(url, mode ? mode : "r");
        fsspec_file_t* result = new fsspec_file_t;
        result->cpp_file = std::move(file);
        return result;
    } catch (...) {
        set_exception_error();
        return NULL;
    }
}

int fsspec_file_close(fsspec_file_t* file) {
    if (!file) return -1;
    try {
        if (!file->closed) {
            nb::gil_scoped_acquire acquire;
            file->cpp_file->close();
            file->closed = true;
        }
        delete file;
        return 0;
    } catch (...) {
        set_exception_error();
        delete file;
        return -1;
    }
}

size_t fsspec_file_read(fsspec_file_t* file, void* buffer, size_t size) {
    if (!file || file->closed) return 0;
    try {
        nb::gil_scoped_acquire acquire;
        return file->cpp_file->read(buffer, size);
    } catch (...) {
        set_exception_error();
        return 0;
    }
}

size_t fsspec_file_write(fsspec_file_t* file, const void* buffer, size_t size) {
    if (!file || file->closed) return 0;
    try {
        nb::gil_scoped_acquire acquire;
        return file->cpp_file->write(buffer, size);
    } catch (...) {
        set_exception_error();
        return 0;
    }
}

int64_t fsspec_file_seek(fsspec_file_t* file, int64_t offset, int whence) {
    if (!file || file->closed) return -1;
    try {
        nb::gil_scoped_acquire acquire;
        file->cpp_file->seek(offset, whence);
        return file->cpp_file->tell();
    } catch (...) {
        set_exception_error();
        return -1;
    }
}

int64_t fsspec_file_tell(fsspec_file_t* file) {
    if (!file || file->closed) return -1;
    try {
        nb::gil_scoped_acquire acquire;
        return file->cpp_file->tell();
    } catch (...) {
        set_exception_error();
        return -1;
    }
}

int fsspec_file_flush(fsspec_file_t* file) {
    // fsspec 通常自动刷新，这里暂时返回成功
    return 0;
}

bool fsspec_file_eof(fsspec_file_t* file) {
    if (!file || file->closed) return true;
    try {
        nb::gil_scoped_acquire acquire;
        int64_t pos = file->cpp_file->tell();
        // 尝试读取 1 字节来判断 EOF
        char c;
        size_t n = file->cpp_file->read(&c, 1);
        if (n == 0) return true;
        // 读到了，seek 回去
        file->cpp_file->seek(pos, 0);
        return false;
    } catch (...) {
        return true;
    }
}
