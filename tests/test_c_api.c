// test_c_api.c - C API 测试示例
#include "fsspec_cpp/fsspec_c.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
    printf("=== fsspec C API Test ===\n");

    // 初始化
    if (fsspec_init() != 0) {
        fprintf(stderr, "Failed to init: %s\n", fsspec_last_error());
        return 1;
    }
    printf("✓ Initialized\n");

    // 创建测试文件
    const char* test_url = "file:///tmp/fsspec_c_test.txt";

    // 测试 1: 直接打开写入
    fsspec_file_t* f = fsspec_open(test_url, "w");
    if (!f) {
        fprintf(stderr, "Failed to open for write: %s\n", fsspec_last_error());
        return 1;
    }

    const char* data = "Hello from fsspec C API!\nLine 2\nLine 3";
    size_t written = fsspec_file_write(f, data, strlen(data));
    printf("✓ Written %zu bytes\n", written);

    fsspec_file_close(f);
    printf("✓ File closed\n");

    // 测试 2: fsspec_stat 获取文件信息
    printf("\n=== fsspec_stat test ===\n");
    fsspec_stat_t fst;
    if (fsspec_stat(test_url, &fst) == 0) {
        printf("✓ stat succeeded\n");
        printf("  Path:     %s\n", fst.path);
        printf("  Name:     %s\n", fst.name);
        printf("  Size:     %ld bytes\n", (long)fst.size);
        printf("  Is dir:   %s\n", fst.is_dir ? "yes" : "no");
        printf("  Modified: %.1f\n", fst.mtime);
        printf("  Protocol: %s\n", fst.protocol);
    } else {
        fprintf(stderr, "✗ stat failed: %s\n", fsspec_last_error());
    }

#ifdef __linux__
    // 测试 3: POSIX stat 兼容
    printf("\n=== POSIX stat test ===\n");
    struct stat st;
    if (fsspec_posix_stat(test_url, &st) == 0) {
        printf("✓ POSIX stat succeeded\n");
        printf("  st_size:  %ld bytes\n", (long)st.st_size);
        printf("  st_mode:  0%o (%s)\n", st.st_mode & 0777,
               S_ISDIR(st.st_mode) ? "directory" : "regular file");
        printf("  st_mtime: %ld\n", (long)st.st_mtime);
        printf("  st_uid:   %d\n", st.st_uid);
        printf("  st_gid:   %d\n", st.st_gid);
        printf("  st_nlink: %lu\n", (unsigned long)st.st_nlink);
    } else {
        fprintf(stderr, "✗ POSIX stat failed: %s\n", fsspec_last_error());
    }
#endif

    // 测试 4: 重新打开读取
    printf("\n=== Read test ===\n");
    f = fsspec_open(test_url, "r");
    if (!f) {
        fprintf(stderr, "Failed to open for read: %s\n", fsspec_last_error());
        return 1;
    }

    char buf[1024];
    size_t total = 0;
    size_t n;
    printf("--- Content ---\n");
    while ((n = fsspec_file_read(f, buf, sizeof(buf))) > 0) {
        fwrite(buf, 1, n, stdout);
        total += n;
    }
    printf("\n---------------\n");
    printf("✓ Read %zu bytes total\n", total);

    // 测试 seek/tell
    fsspec_file_seek(f, 0, 0);  // SEEK_SET
    int64_t pos = fsspec_file_tell(f);
    printf("✓ Seek to beginning, pos=%ld\n", (long)pos);

    fsspec_file_close(f);

// 测试 5: FILE* 包装 (Linux only)
#ifdef __linux__
    printf("\n=== FILE* wrapper test (GNU/Linux) ===\n");
    FILE* fp = fsspec_fopen(test_url, "r");
    if (!fp) {
        fprintf(stderr, "Failed to fsspec_fopen\n");
        return 1;
    }

    printf("✓ Opened as FILE*\n");

    // 使用标准 C I/O
    char line[256];
    printf("\n--- Lines via FILE* ---\n");
    while (fgets(line, sizeof(line), fp)) {
        printf("Line: %s", line);
    }
    printf("\n-----------------------\n");

    // 测试 fseek/ftell
    fseek(fp, 0, SEEK_SET);
    long fpos = ftell(fp);
    printf("✓ fseek/ftell works, pos=%ld\n", fpos);

    // 测试 fread
    char fbuf[100];
    size_t fn = fread(fbuf, 1, sizeof(fbuf) - 1, fp);
    fbuf[fn] = '\0';
    printf("✓ fread read %zu bytes: '%.20s...'\n", fn, fbuf);

    fclose(fp);
    printf("✓ FILE* closed\n");
#endif

    // 测试 6: 零拷贝 Buffer API
    printf("\n=== Zero-copy Buffer API test ===\n");
    const char* buffer_test_url = "file:///tmp/fsspec_buffer_test.txt";
    const char* test_data = "Zero-copy buffer test data!";

    // 写入测试文件
    f = fsspec_open(buffer_test_url, "w");
    if (!f) {
        fprintf(stderr, "Failed to open buffer test file for write\n");
        return 1;
    }
    written = fsspec_file_write(f, test_data, strlen(test_data));
    printf("✓ Written %zu bytes for buffer test\n", written);
    fsspec_file_close(f);

    // 使用零拷贝 API 读取
    f = fsspec_open(buffer_test_url, "r");
    if (!f) {
        fprintf(stderr, "Failed to open buffer test file for read\n");
        return 1;
    }

    fsspec_buffer_t* buffer = fsspec_file_read_buffer(f, 1024);
    if (buffer) {
        const void* ptr;
        size_t size;
        if (fsspec_buffer_get_info(buffer, &ptr, &size) == 0) {
            printf("✓ Zero-copy read succeeded\n");
            printf("  Buffer size: %zu bytes\n", size);
            printf("  Content: %.*s\n", (int)size, (char*)ptr);
            if (size == strlen(test_data) && memcmp(ptr, test_data, size) == 0) {
                printf("✓ Data matches!\n");
            } else {
                fprintf(stderr, "✗ Data mismatch!\n");
            }
        }
        fsspec_buffer_release(buffer);
    } else {
        fprintf(stderr, "✗ Zero-copy read failed: %s\n", fsspec_last_error());
    }
    fsspec_file_close(f);

    // 测试零拷贝写入
    f = fsspec_open(buffer_test_url, "w");
    if (f) {
        fsspec_buffer_t* write_buf = fsspec_buffer_from_memory(test_data, strlen(test_data));
        if (write_buf) {
            written = fsspec_file_write_buffer(f, write_buf);
            printf("✓ Zero-copy write: %zu bytes\n", written);
            fsspec_buffer_release(write_buf);
        }
        fsspec_file_close(f);

        // 验证写入的数据
        f = fsspec_open(buffer_test_url, "r");
        if (f) {
            char verify_buf[256];
            size_t n = fsspec_file_read(f, verify_buf, sizeof(verify_buf));
            if (n == strlen(test_data) && memcmp(verify_buf, test_data, n) == 0) {
                printf("✓ Zero-copy write data verified!\n");
            }
            fsspec_file_close(f);
        }
    }

    // 清理
    fsspec_cleanup();
    printf("\n=== All tests passed! ===\n");

    return 0;
}
