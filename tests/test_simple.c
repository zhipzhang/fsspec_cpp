#include "fsspec_cpp/fsspec_c.h"
#include <stdio.h>
#include <string.h>

int main() {
    printf("=== fsspec C API Test ===\n");
    
    // 初始化
    if (fsspec_init() != 0) {
        fprintf(stderr, "Failed to init: %s\n", fsspec_last_error());
        return 1;
    }
    printf("✓ Initialized\n");
    fflush(stdout);
    
    // 测试: 通过 fs_from_url 获取文件系统
    const char* test_url = "file:///tmp/";
    printf("Getting fs for: %s\n", test_url);
    fflush(stdout);
    
    fsspec_fs_t* fs = fsspec_fs_from_url(test_url);
    if (!fs) {
        fprintf(stderr, "Failed to get fs: %s\n", fsspec_last_error());
        return 1;
    }
    printf("✓ Got filesystem\n");
    fflush(stdout);
    
    // 检查文件是否存在
    const char* test_file = "/tmp/fsspec_c_test.txt";
    printf("Checking exists: %s\n", test_file);
    fflush(stdout);
    
    bool exists = fsspec_fs_exists(fs, test_file);
    printf("Exists: %s\n", exists ? "yes" : "no");
    fflush(stdout);
    
    // 打开文件
    printf("Opening file\n");
    fflush(stdout);
    fsspec_file_t* f = fsspec_fs_open(fs, test_file, FSSPEC_MODE_WRITE);
    if (!f) {
        fprintf(stderr, "Failed to open: %s\n", fsspec_last_error());
        fsspec_fs_free(fs);
        return 1;
    }
    printf("✓ Opened file\n");
    fflush(stdout);
    
    // 写入
    const char* data = "Hello!\n";
    size_t written = fsspec_file_write(f, data, strlen(data));
    printf("✓ Written %zu bytes\n", written);
    fflush(stdout);
    
    fsspec_file_close(f);
    printf("✓ File closed\n");
    fflush(stdout);
    
    fsspec_fs_free(fs);
    printf("✓ FS freed\n");
    fflush(stdout);
    
    // 清理
    fsspec_cleanup();
    printf("\n=== Test passed! ===\n");
    
    return 0;
}
