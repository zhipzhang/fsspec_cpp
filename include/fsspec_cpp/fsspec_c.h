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

// ============ 协议支持 ============

// 导入额外的 fsspec 协议支持模块（如 fsspec_xrootd, fsspec_smb 等）
// 模块名：要导入的 Python 模块名（如 "fsspec_xrootd"）
// 返回值：0 成功, -1 失败（模块不存在或导入错误）
// 错误信息可通过 fsspec_last_error() 获取
int fsspec_import_protocol(const char* module_name);

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

// ============ 零拷贝 Buffer API (Python 3.10+) ============

// 零拷贝缓冲区句柄
typedef struct fsspec_buffer_s fsspec_buffer_t;

// 读取数据到零拷贝缓冲区
// 返回缓冲区对象，用完必须调用 fsspec_buffer_release() 释放
// 失败返回 NULL，错误信息通过 fsspec_last_error() 获取
fsspec_buffer_t* fsspec_file_read_buffer(fsspec_file_t* file, size_t size);

// 写入零拷贝缓冲区到文件
// buffer: 通过 fsspec_buffer_t 或任何支持 Python buffer protocol 的对象
// 返回实际写入的字节数
size_t fsspec_file_write_buffer(fsspec_file_t* file, fsspec_buffer_t* buffer);

// 获取缓冲区指针和大小
// ptr: 输出参数，接收数据指针
// size: 输出参数，接收数据大小
// 返回 0 成功, -1 失败
int fsspec_buffer_get_info(fsspec_buffer_t* buffer, const void** ptr, size_t* size);

// 释放缓冲区
void fsspec_buffer_release(fsspec_buffer_t* buffer);

// 从用户内存创建零拷贝缓冲区（用于写入）
// 不拷贝数据，只是包装用户指针
// 返回缓冲区对象，用完必须调用 fsspec_buffer_release() 释放
fsspec_buffer_t* fsspec_buffer_from_memory(const void* ptr, size_t size);

// ============ 文件信息 (stat) ============

typedef struct {
    char path[1024];    // 完整路径
    char name[256];     // 文件名
    int64_t size;       // 文件大小（字节）
    bool is_dir;        // 是否是目录
    double mtime;       // 修改时间（Unix时间戳，秒）
    char protocol[64];  // 协议（file, s3, gcs等）
} fsspec_stat_t;

// 获取文件/目录信息
// 返回值: 0 成功, -1 失败
int fsspec_fs_stat(fsspec_fs_t* fs, const char* path, fsspec_stat_t* st);

// 便捷函数：直接通过 URL stat
int fsspec_stat(const char* url, fsspec_stat_t* st);

// ============ POSIX stat 兼容 (Linux) ============

#ifdef __linux__
#include <sys/stat.h>

// 将 fsspec_stat_t 填充到 POSIX struct stat
// 注意：部分字段（inode, nlink, blksize等）会被设为0或默认值
// 因为云存储不一定支持这些信息
int fsspec_stat_to_posix(const fsspec_stat_t* fst, struct stat* st);

// 直接通过 URL 获取 POSIX stat（Linux 专用）
// 这是 fsspec_stat + fsspec_stat_to_posix 的便捷组合
int fsspec_posix_stat(const char* url, struct stat* st);
#endif

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

#endif  // FSSPEC_C_H
