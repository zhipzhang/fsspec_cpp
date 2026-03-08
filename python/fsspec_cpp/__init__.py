# Python 包入口
from ._fsspec_cpp import (
    FileInfo,
    File, 
    FileSystem,
    OpenMode,
    filesystem_from_url,
    open,
    exists,
    remove,
    ls,
)

__all__ = [
    "FileInfo",
    "File",
    "FileSystem", 
    "OpenMode",
    "filesystem_from_url",
    "open",
    "exists",
    "remove",
    "ls",
]

__version__ = "0.1.0"
