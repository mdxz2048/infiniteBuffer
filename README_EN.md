[中文版本](README.md) | English Description

# Infinite Buffer Library

This is a pure C implementation of a circular buffer, which we call the InfiniteBuffer.

## Features

- Only depends on basic POSIX interfaces
- Lock-free implementation
- Supports single-producer, multi-consumer mode

## API Description

`lib_infinite_buffer` provides the following interfaces:

- `lib_infinite_buffer_create`: Create and initialize a buffer.
- `lib_infinite_buffer_destroy`: Destroy the buffer and release resources.
- `lib_infinite_buffer_write`: Write data to the buffer.
- `lib_infinite_buffer_write_wait`: Write data to the buffer, recommended for use.
- `lib_infinite_buffer_read`: Read data from the buffer.
- `lib_infinite_buffer_isEmpty`: Check if the buffer is empty.

For more information on the interfaces, please see [lib_infinite_buffer.h](include/lib_infinite_buffer.h).

## Usage Example

Ensure you have the `make` tool installed. Use the following command to build the library and examples:

```shell
make
```

This will generate the library file `libinfiniteBuffer.so` in the `lib` directory and provide the header file in the include directory `#include "lib_infinite_buffer.h"`.

## Performance

Performance test results will be provided through a Python script, so stay tuned.

## License

This project is licensed under the MIT License. See the LICENSE file for details.