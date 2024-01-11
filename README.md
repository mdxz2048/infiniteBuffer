[English Version](README_EN.md) | 中文说明

# Infinite Buffer Library

这是一个纯C语言写的环形缓冲区的实现，我们叫它无限缓冲区InfiniteBuffer。

## 特点

- 仅依赖基本的 POSIX 接口
- 无锁实现
- 支持单生产者多消费者模式

## 接口功能说明

`lib_infinite_buffer` 提供以下接口：

- `lib_infinite_buffer_create`：创建并初始化缓冲区。
- `lib_infinite_buffer_destroy`：销毁缓冲区并释放资源。
- `lib_infinite_buffer_write`：写入数据到缓冲区。
- `lib_infinite_buffer_write_wait`：写入数据到缓冲区，推荐使用。
- `lib_infinite_buffer_read`：从缓冲区读取数据。
- `lib_infinite_buffer_isEmpty`：检查缓冲区是否为空。

更多接口信息请查看 [lib_infinite_buffer.h](include/lib_infinite_buffer.h)。

## 使用示例

确保您已经安装了 `make` 工具。使用以下命令构建库和示例：

```shell
make
```

这将在 `lib` 目录下生成库文件`libinfiniteBuffer.so`，并在 include 目录下提供头文件`#include "lib_infinite_buffer.h`。

## 性能

性能测试结果将通过 Python 脚本提供，敬请期待。

## License

本项目采用 MIT 许可证。详情请参见 LICENSE 文件。

