// fsspec_c.h - C API 头文件
#ifndef FSSPEC_C_H
#define FSSPEC_C_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fsspec_file_s fsspec_file_t;
typedef struct fsspec_fs_s fsspec_fs_t;

// 文件打开模式
typedef enum {
    FSSPEC_MODE_READ = 0,
    FSSPEC_MODE_WRITE,
    FSSPEC_MODE_APPEND,
    FSSPEC_MODE_READWRITE
} fsspec_mode_t;

// 初始化 Python 解释器
// 返回值: 0 成功, -1 失败
int fsspec_init(void);

// 清理（程序退出时调用）
void fsspec_cleanup(void);

// 获取上次错误信息
const char* fsspec_last_error(void);

// ============ 文件系统操作 ============

// 从 URL 获取文件系统
// 返回 NULL 表示失败，用 fsspec_last_error() 获取错误
fsspec_fs_t* fsspec_fs_from_url(const char* url);

// 释放文件系统（通常不需要手动调用，但提供以防万一）
void fsspec_fs_free(fsspec_fs_t* fs);

// 检查文件是否存在
bool fsspec_fs_exists(fsspec_fs_t* fs, const char* path);

// 删除文件/目录
// 返回值: 0 成功, -1 失败
int fsspec_fs_remove(fsspec_fs_t* fs, const char* path);

// 重命名
int fsspec_fs_rename(fsspec_fs_t* fs, const char* src, const char* dst);

// ============ 文件操作 ============

// 通过文件系统打开文件
fsspec_file_t* fsspec_fs_open(fsspec_fs_t* fs, const char* path, fsspec_mode_t mode);

// 便捷函数：直接通过 URL 打开文件
fsspec_file_t* fsspec_open(const char* url, const char* mode);

// 关闭文件
int fsspec_file_close(fsspec_file_t* file);

// 读取数据，返回实际读取的字节数
size_t fsspec_file_read(fsspec_file_t* file, void* buffer, size_t size);

// 写入数据，返回实际写入的字节数
size_t fsspec_file_write(fsspec_file_t* file, const void* buffer, size_t size);

// 定位文件指针
// whence: 0=SEEK_SET, 1=SEEK_CUR, 2=SEEK_END
int64_t fsspec_file_seek(fsspec_file_t* file, int64_t offset, int whence);

// 获取当前位置
int64_t fsspec_file_tell(fsspec_file_t* file);

// 刷新缓冲区
int fsspec_file_flush(fsspec_file_t* file);

// 检查是否已到文件末尾
bool fsspec_file_eof(fsspec_file_t* file);

// ============ GNU/Linux FILE* 包装 ============

#ifdef __linux__
// 将 fsspec_file 包装成 FILE*
// 返回的 FILE* 可以正常使用 fread/fwrite/fseek/ftell/fclose
// 注意：这是 GNU 扩展 fopencookie 实现，只在 Linux 有效
FILE* fsspec_file_to_fileptr(fsspec_file_t* file, const char* mode);

// 便捷函数：直接打开 URL 返回 FILE*
FILE* fsspec_fopen(const char* url, const char* mode);
#endif

#ifdef __cplusplus
}
#endif

#endif // FSSPEC_C_H
