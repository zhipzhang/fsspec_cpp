// fsspec_c_pure.c - 纯 C 实现，直接使用 Python C API
#include "fsspec_cpp/fsspec_c.h"
#include <Python.h>
#include <string.h>
#include <stdlib.h>

#ifdef __linux__
#include <unistd.h>
#include <sys/stat.h>
#endif

// 线程局部存储错误信息
static __thread char g_last_error[1024] = {0};

static void set_error(const char* msg) {
    strncpy(g_last_error, msg, sizeof(g_last_error) - 1);
    g_last_error[sizeof(g_last_error) - 1] = '\0';
}

// 包装结构体
struct fsspec_file_s {
    PyObject* py_file;
    bool closed;
};

struct fsspec_fs_s {
    PyObject* py_fs;
};

// 零拷贝 Buffer 结构体
struct fsspec_buffer_s {
    PyObject* obj;       // 持有的 Python 对象（增加引用计数）
    void* ptr;           // 数据指针
    size_t size;         // 数据大小
    bool is_memoryview;  // 是否是 memoryview（需要特殊释放）
};

// 前向声明（零拷贝 API）
fsspec_buffer_t* fsspec_file_read_buffer(fsspec_file_t* file, size_t size);
size_t fsspec_file_write_buffer(fsspec_file_t* file, fsspec_buffer_t* buffer);
fsspec_buffer_t* fsspec_buffer_from_memory(const void* ptr, size_t size);
int fsspec_buffer_get_info(fsspec_buffer_t* buffer, const void** ptr, size_t* size);
void fsspec_buffer_release(fsspec_buffer_t* buffer);

// ============ 初始化 ============

int fsspec_init(void) {
    if (!Py_IsInitialized()) {
        Py_Initialize();
    }
    return 0;
}

void fsspec_cleanup(void) {
    // 不清理 Python 解释器
}

const char* fsspec_last_error(void) { return g_last_error[0] ? g_last_error : NULL; }

// ============ 协议支持 ============

int fsspec_import_protocol(const char* module_name) {
    if (!module_name) {
        set_error("Module name is NULL");
        return -1;
    }

    PyObject* module = PyImport_ImportModule(module_name);
    if (!module) {
        // 获取 Python 错误信息
        if (PyErr_Occurred()) {
            PyObject *ptype, *pvalue, *ptraceback;
            PyErr_Fetch(&ptype, &pvalue, &ptraceback);

            if (pvalue) {
                PyObject* str = PyObject_Str(pvalue);
                if (str) {
                    const char* msg = PyUnicode_AsUTF8(str);
                    snprintf(g_last_error, sizeof(g_last_error), "Failed to import '%s': %s",
                             module_name, msg);
                    Py_DECREF(str);
                } else {
                    snprintf(g_last_error, sizeof(g_last_error),
                             "Failed to import '%s': unknown error", module_name);
                }
            } else {
                snprintf(g_last_error, sizeof(g_last_error),
                         "Failed to import '%s': module not found", module_name);
            }

            Py_XDECREF(ptype);
            Py_XDECREF(pvalue);
            Py_XDECREF(ptraceback);
        } else {
            snprintf(g_last_error, sizeof(g_last_error), "Failed to import '%s': module not found",
                     module_name);
        }
        return -1;
    }

    Py_DECREF(module);
    return 0;
}

// ============ 内部辅助函数 ============

static PyObject* get_fsspec_core(void) {
    static PyObject* core = NULL;
    if (!core) {
        core = PyImport_ImportModule("fsspec.core");
        if (!core) {
            PyErr_Print();
            return NULL;
        }
    }
    return core;
}

static PyObject* get_fsspec(void) {
    static PyObject* fsspec = NULL;
    if (!fsspec) {
        fsspec = PyImport_ImportModule("fsspec");
        if (!fsspec) {
            PyErr_Print();
            return NULL;
        }
    }
    return fsspec;
}

// ============ 文件系统 ============

fsspec_fs_t* fsspec_fs_from_url(const char* url) {
    PyObject* core = get_fsspec_core();
    if (!core) {
        set_error("Failed to import fsspec.core");
        return NULL;
    }

    PyObject* result = PyObject_CallMethod(core, "url_to_fs", "s", url);
    if (!result) {
        PyErr_Print();
        set_error("url_to_fs failed");
        return NULL;
    }

    // result is (fs, path) tuple
    PyObject* fs = PyTuple_GetItem(result, 0);
    if (!fs) {
        Py_DECREF(result);
        set_error("Invalid result from url_to_fs");
        return NULL;
    }

    Py_INCREF(fs);
    Py_DECREF(result);

    fsspec_fs_t* fsspec_fs = malloc(sizeof(fsspec_fs_t));
    fsspec_fs->py_fs = fs;
    return fsspec_fs;
}

void fsspec_fs_free(fsspec_fs_t* fs) {
    if (fs) {
        Py_DECREF(fs->py_fs);
        free(fs);
    }
}

bool fsspec_fs_exists(fsspec_fs_t* fs, const char* path) {
    if (!fs) return false;
    PyObject* result = PyObject_CallMethod(fs->py_fs, "exists", "s", path);
    if (!result) {
        PyErr_Print();
        return false;
    }
    bool exists = PyObject_IsTrue(result);
    Py_DECREF(result);
    return exists;
}

int fsspec_fs_remove(fsspec_fs_t* fs, const char* path) {
    if (!fs) return -1;
    PyObject* result = PyObject_CallMethod(fs->py_fs, "rm", "s", path);
    if (!result) {
        PyErr_Print();
        return -1;
    }
    Py_DECREF(result);
    return 0;
}

int fsspec_fs_rename(fsspec_fs_t* fs, const char* src, const char* dst) {
    if (!fs) return -1;
    PyObject* result = PyObject_CallMethod(fs->py_fs, "mv", "ss", src, dst);
    if (!result) {
        PyErr_Print();
        return -1;
    }
    Py_DECREF(result);
    return 0;
}

// ============ 文件操作 ============

fsspec_file_t* fsspec_fs_open(fsspec_fs_t* fs, const char* path, fsspec_mode_t mode) {
    if (!fs) return NULL;

    // 始终使用二进制模式
    const char* mode_str = "rb";
    switch (mode) {
        case FSSPEC_MODE_READ:
            mode_str = "rb";
            break;
        case FSSPEC_MODE_WRITE:
            mode_str = "wb";
            break;
        case FSSPEC_MODE_APPEND:
            mode_str = "ab";
            break;
        case FSSPEC_MODE_READWRITE:
            mode_str = "r+b";
            break;
    }

    PyObject* result = PyObject_CallMethod(fs->py_fs, "open", "ss", path, mode_str);
    if (!result) {
        PyErr_Print();
        return NULL;
    }

    fsspec_file_t* file = malloc(sizeof(fsspec_file_t));
    file->py_file = result;
    file->closed = false;
    return file;
}

fsspec_file_t* fsspec_open(const char* url, const char* mode) {
    // 解析模式，转换为二进制模式
    fsspec_mode_t fmode = FSSPEC_MODE_READ;
    if (mode) {
        if (strchr(mode, 'w'))
            fmode = FSSPEC_MODE_WRITE;
        else if (strchr(mode, 'a'))
            fmode = FSSPEC_MODE_APPEND;
        else if (strchr(mode, '+'))
            fmode = FSSPEC_MODE_READWRITE;
    }

    fsspec_fs_t* fs = fsspec_fs_from_url(url);
    if (!fs) {
        set_error("Failed to get filesystem from URL");
        return NULL;
    }

    fsspec_file_t* file = fsspec_fs_open(fs, url, fmode);

    // 注意：文件系统对象需要保持存活，但当前设计没有处理这个问题
    // 简化起见，保持 fs 存活（内存泄漏，但功能正确）

    return file;
}

int fsspec_file_close(fsspec_file_t* file) {
    if (!file) return -1;
    if (!file->closed) {
        PyObject_CallMethod(file->py_file, "close", NULL);
        file->closed = true;
    }
    Py_DECREF(file->py_file);
    free(file);
    return 0;
}

size_t fsspec_file_read(fsspec_file_t* file, void* buffer, size_t size) {
    if (!file || file->closed) return 0;

    // 使用零拷贝 Buffer API，然后拷贝到用户 buffer
    fsspec_buffer_t* buf = fsspec_file_read_buffer(file, size);
    if (!buf) return 0;

    const void* ptr;
    size_t len;
    if (fsspec_buffer_get_info(buf, &ptr, &len) != 0) {
        fsspec_buffer_release(buf);
        return 0;
    }

    // 拷贝到用户 buffer（这是必要的，因为用户提供了目标内存）
    memcpy(buffer, ptr, len);
    fsspec_buffer_release(buf);
    return len;
}

size_t fsspec_file_write(fsspec_file_t* file, const void* buffer, size_t size) {
    if (!file || file->closed) return 0;

    // 使用零拷贝 Buffer API
    fsspec_buffer_t* buf = fsspec_buffer_from_memory(buffer, size);
    if (!buf) return 0;

    size_t written = fsspec_file_write_buffer(file, buf);
    fsspec_buffer_release(buf);
    return written;
}

int64_t fsspec_file_seek(fsspec_file_t* file, int64_t offset, int whence) {
    if (!file || file->closed) return -1;

    PyObject* result = PyObject_CallMethod(file->py_file, "seek", "Li", (long long)offset, whence);
    if (!result) {
        PyErr_Print();
        return -1;
    }

    int64_t pos = (int64_t)PyLong_AsLongLong(result);
    Py_DECREF(result);
    return pos;
}

int64_t fsspec_file_tell(fsspec_file_t* file) {
    if (!file || file->closed) return -1;

    PyObject* result = PyObject_CallMethod(file->py_file, "tell", NULL);
    if (!result) {
        PyErr_Print();
        return -1;
    }

    int64_t pos = (int64_t)PyLong_AsLongLong(result);
    Py_DECREF(result);
    return pos;
}

int fsspec_file_flush(fsspec_file_t* file) {
    // fsspec 通常自动刷新
    return 0;
}

bool fsspec_file_eof(fsspec_file_t* file) {
    if (!file || file->closed) return true;

    int64_t pos = fsspec_file_tell(file);
    char c;
    size_t n = fsspec_file_read(file, &c, 1);
    if (n == 0) return true;
    fsspec_file_seek(file, pos, 0);
    return false;
}

// ============ 文件信息 ============

int fsspec_fs_stat(fsspec_fs_t* fs, const char* path, fsspec_stat_t* st) {
    if (!fs || !st) return -1;

    PyObject* result = PyObject_CallMethod(fs->py_fs, "info", "s", path);
    if (!result) {
        PyErr_Print();
        return -1;
    }

    memset(st, 0, sizeof(*st));

    PyObject* name = PyDict_GetItemString(result, "name");
    if (name && PyUnicode_Check(name)) {
        strncpy(st->path, PyUnicode_AsUTF8(name), sizeof(st->path) - 1);
        strncpy(st->name, PyUnicode_AsUTF8(name), sizeof(st->name) - 1);
    }

    PyObject* size = PyDict_GetItemString(result, "size");
    if (size) st->size = (int64_t)PyLong_AsLongLong(size);

    PyObject* type = PyDict_GetItemString(result, "type");
    if (type && PyUnicode_Check(type)) {
        st->is_dir = strcmp(PyUnicode_AsUTF8(type), "directory") == 0;
    }

    PyObject* mtime = PyDict_GetItemString(result, "mtime");
    if (mtime) st->mtime = PyFloat_AsDouble(mtime);

    Py_DECREF(result);
    return 0;
}

int fsspec_stat(const char* url, fsspec_stat_t* st) {
    fsspec_fs_t* fs = fsspec_fs_from_url(url);
    if (!fs) return -1;
    int result = fsspec_fs_stat(fs, url, st);
    fsspec_fs_free(fs);
    return result;
}

// ============ POSIX stat 兼容 ============

#ifdef __linux__

int fsspec_stat_to_posix(const fsspec_stat_t* fst, struct stat* st) {
    if (!fst || !st) return -1;

    memset(st, 0, sizeof(*st));
    st->st_size = fst->size;
    st->st_mode = fst->is_dir ? (S_IFDIR | 0755) : (S_IFREG | 0644);
    st->st_mtime = (time_t)fst->mtime;
    st->st_atime = st->st_mtime;
    st->st_ctime = st->st_mtime;
    st->st_uid = getuid();
    st->st_gid = getgid();
    st->st_blksize = 4096;
    st->st_blocks = (fst->size + 511) / 512;
    st->st_nlink = 1;
    return 0;
}

int fsspec_posix_stat(const char* url, struct stat* st) {
    fsspec_stat_t fst;
    if (fsspec_stat(url, &fst) != 0) return -1;
    return fsspec_stat_to_posix(&fst, st);
}

#endif

// ============ FILE* 包装 ============

#ifdef __linux__

static ssize_t cookie_read(void* cookie, char* buf, size_t size) {
    return (ssize_t)fsspec_file_read((fsspec_file_t*)cookie, buf, size);
}

static ssize_t cookie_write(void* cookie, const char* buf, size_t size) {
    return (ssize_t)fsspec_file_write((fsspec_file_t*)cookie, buf, size);
}

static int cookie_seek(void* cookie, off64_t* offset, int whence) {
    int64_t pos = fsspec_file_seek((fsspec_file_t*)cookie, *offset, whence);
    if (pos < 0) {
        errno = ESPIPE;
        return -1;
    }
    *offset = pos;
    return 0;
}

static int cookie_close(void* cookie) { return fsspec_file_close((fsspec_file_t*)cookie); }

FILE* fsspec_file_to_fileptr(fsspec_file_t* file, const char* mode) {
    if (!file) return NULL;

    cookie_io_functions_t funcs = {
        .read = cookie_read, .write = cookie_write, .seek = cookie_seek, .close = cookie_close};

    return fopencookie(file, mode, funcs);
}

FILE* fsspec_fopen(const char* url, const char* mode) {
    if (!url) {
        errno = EINVAL;
        return NULL;
    }

    fsspec_file_t* file = fsspec_open(url, mode);
    if (!file) {
        errno = ENOENT;
        return NULL;
    }

    FILE* fp = fsspec_file_to_fileptr(file, mode ? mode : "rb");
    if (!fp) {
        fsspec_file_close(file);
    }
    return fp;
}

#endif

// ============ 零拷贝 Buffer API ============

fsspec_buffer_t* fsspec_file_read_buffer(fsspec_file_t* file, size_t size) {
    if (!file || file->closed) return NULL;

    // 读取数据
    PyObject* result = PyObject_CallMethod(file->py_file, "read", "n", (Py_ssize_t)size);
    if (!result) {
        PyErr_Print();
        set_error("read() failed");
        return NULL;
    }

    // 创建 memoryview（零拷贝视图）
    PyObject* mv = PyMemoryView_FromObject(result);
    if (!mv) {
        Py_DECREF(result);
        PyErr_Print();
        set_error("Failed to create memoryview");
        return NULL;
    }

    // 获取 buffer 信息
    Py_buffer* buf = PyMemoryView_GET_BUFFER(mv);
    if (!buf) {
        Py_DECREF(mv);
        Py_DECREF(result);
        set_error("Failed to get buffer info");
        return NULL;
    }

    fsspec_buffer_t* buffer = malloc(sizeof(fsspec_buffer_t));
    buffer->obj = result;          // 持有原始 bytes 对象
    buffer->ptr = buf->buf;        // 直接指向内存
    buffer->size = buf->len;       // 数据大小
    buffer->is_memoryview = true;  // 标记类型

    Py_DECREF(mv);  // memoryview 可以释放了，但底层 buffer 还在
    return buffer;
}

size_t fsspec_file_write_buffer(fsspec_file_t* file, fsspec_buffer_t* buffer) {
    if (!file || file->closed || !buffer) return 0;

    // 创建 memoryview 包装（零拷贝）
    PyObject* mv = PyMemoryView_FromMemory((char*)buffer->ptr, buffer->size, PyBUF_READ);
    if (!mv) {
        PyErr_Print();
        return 0;
    }

    PyObject* result = PyObject_CallMethod(file->py_file, "write", "O", mv);
    Py_DECREF(mv);

    if (!result) {
        PyErr_Print();
        return 0;
    }

    size_t written = (size_t)PyLong_AsSize_t(result);
    Py_DECREF(result);
    return written;
}

int fsspec_buffer_get_info(fsspec_buffer_t* buffer, const void** ptr, size_t* size) {
    if (!buffer) return -1;
    if (ptr) *ptr = buffer->ptr;
    if (size) *size = buffer->size;
    return 0;
}

void fsspec_buffer_release(fsspec_buffer_t* buffer) {
    if (!buffer) return;
    if (buffer->obj) {
        Py_DECREF(buffer->obj);  // 释放持有的 Python 对象
    }
    free(buffer);
}

fsspec_buffer_t* fsspec_buffer_from_memory(const void* ptr, size_t size) {
    if (!ptr || size == 0) return NULL;

    fsspec_buffer_t* buffer = malloc(sizeof(fsspec_buffer_t));
    buffer->obj = NULL;        // 不持有 Python 对象
    buffer->ptr = (void*)ptr;  // 用户内存
    buffer->size = size;
    buffer->is_memoryview = false;  // 标记类型

    return buffer;
}
