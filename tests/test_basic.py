import fsspec_cpp as fsc

def test_open_local_file():
    """测试打开本地文件"""
    # 创建测试文件
    with open("/tmp/test_fsspec_cpp.txt", "w") as f:
        f.write("Hello fsspec-cpp!")
    
    # 通过 fsspec_cpp 读取
    fs = fsc.filesystem_from_url("file:///tmp/")
    assert fs.exists("/tmp/test_fsspec_cpp.txt")
    
    f = fs.open("/tmp/test_fsspec_cpp.txt", fsc.OpenMode.Read)
    content = f.read_all()
    assert content == b"Hello fsspec-cpp!"
    f.close()

def test_open_url():
    """测试直接 open URL"""
    f = fsc.open("file:///tmp/test_fsspec_cpp.txt")
    content = f.read_all()
    assert content == b"Hello fsspec-cpp!"
    f.close()

def test_write_file():
    """测试写入文件"""
    fs = fsc.filesystem_from_url("file:///tmp/")
    f = fs.open("/tmp/test_write.txt", fsc.OpenMode.Write)
    f.write_all(b"Test write content")
    f.close()
    
    # 验证
    f = fs.open("/tmp/test_write.txt", fsc.OpenMode.Read)
    assert f.read_all() == b"Test write content"
    f.close()

if __name__ == "__main__":
    test_open_local_file()
    test_open_url()
    test_write_file()
    print("All tests passed!")
