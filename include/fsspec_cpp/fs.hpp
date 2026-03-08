#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace fsspec {

class File;
class FileSystem;

// 文件打开模式
enum class OpenMode {
    Read,
    Write,
    Append,
    ReadWrite
};

// 文件信息
struct FileInfo {
    std::string path;
    std::string name;
    int64_t size;
    bool is_dir;
    double mtime;  // 修改时间
    std::string protocol;
};

// C++ 层面的文件系统接口
class FileSystem {
public:
    virtual ~FileSystem() = default;
    
    // 文件操作
    virtual std::unique_ptr<File> open(const std::string& path, OpenMode mode = OpenMode::Read) = 0;
    virtual bool exists(const std::string& path) = 0;
    virtual void remove(const std::string& path) = 0;
    virtual void rename(const std::string& src, const std::string& dst) = 0;
    
    // 目录操作
    virtual std::vector<FileInfo> ls(const std::string& path) = 0;
    virtual void mkdir(const std::string& path, bool create_parents = true) = 0;
    virtual void rmdir(const std::string& path) = 0;
    
    // 工具
    virtual FileInfo info(const std::string& path) = 0;
    virtual std::vector<FileInfo> glob(const std::string& pattern) = 0;
};

// 文件句柄
class File {
public:
    virtual ~File() = default;
    
    // 读写
    virtual size_t read(void* buffer, size_t size) = 0;
    virtual size_t write(const void* buffer, size_t size) = 0;
    virtual void seek(int64_t pos, int whence = 0) = 0;
    virtual int64_t tell() = 0;
    virtual void close() = 0;
    virtual bool closed() const = 0;
    
    // 便捷方法
    std::string read_all();
    void write_all(const std::string& data);
};

// 从 URL 获取文件系统
std::unique_ptr<FileSystem> filesystem_from_url(const std::string& url);

// 便捷函数
std::unique_ptr<File> open(const std::string& url, const std::string& mode = "r");
bool exists(const std::string& url);
void remove(const std::string& url);
std::vector<FileInfo> ls(const std::string& url);

} // namespace fsspec
