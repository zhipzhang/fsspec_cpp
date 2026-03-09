// fsspec_fileptr.cpp - GNU/Linux FILE* 包装 (fopencookie)
#ifdef __linux__

#include "fsspec_cpp/fsspec_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// fopencookie 需要的回调函数

static ssize_t cookie_read(void* cookie, char* buf, size_t size) {
    fsspec_file_t* file = (fsspec_file_t*)cookie;
    return (ssize_t)fsspec_file_read(file, buf, size);
}

static ssize_t cookie_write(void* cookie, const char* buf, size_t size) {
    fsspec_file_t* file = (fsspec_file_t*)cookie;
    return (ssize_t)fsspec_file_write(file, buf, size);
}

static int cookie_seek(void* cookie, off64_t* offset, int whence) {
    fsspec_file_t* file = (fsspec_file_t*)cookie;
    int64_t pos = fsspec_file_seek(file, *offset, whence);
    if (pos < 0) {
        errno = ESPIPE;
        return -1;
    }
    *offset = pos;
    return 0;
}

static int cookie_close(void* cookie) {
    fsspec_file_t* file = (fsspec_file_t*)cookie;
    return fsspec_file_close(file);
}

FILE* fsspec_file_to_fileptr(fsspec_file_t* file, const char* mode) {
    if (!file) return NULL;

    cookie_io_functions_t funcs = {
        .read = cookie_read, .write = cookie_write, .seek = cookie_seek, .close = cookie_close};

    // fopencookie: GNU 扩展
    FILE* fp = fopencookie(file, mode, funcs);
    if (!fp) {
        errno = ENOMEM;
    }
    return fp;
}

FILE* fsspec_fopen(const char* url, const char* mode) {
    if (!url) {
        errno = EINVAL;
        return NULL;
    }

    fsspec_file_t* file = fsspec_open(url, mode);
    if (!file) {
        errno = ENOENT;  // 或根据错误码映射
        return NULL;
    }

    FILE* fp = fsspec_file_to_fileptr(file, mode ? mode : "r");
    if (!fp) {
        // fopencookie 失败，清理 fsspec_file
        fsspec_file_close(file);
    }
    return fp;
}

#endif  // __linux__
