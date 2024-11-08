#pragma once

#include <cstddef>
#include <cstdlib>
#include <memory>

constexpr std::size_t operator""_KiB(unsigned long long int x) {
    return 1024ULL * x;
}

constexpr std::size_t operator""_MiB(unsigned long long int x) {
    return 1024_KiB * x;
}

constexpr std::size_t operator""_GiB(unsigned long long int x) {
    return 1024_MiB * x;
}

constexpr std::size_t operator""_TiB(unsigned long long int x) {
    return 1024_GiB * x;
}

constexpr std::size_t operator""_PiB(unsigned long long int x) {
    return 1024_TiB * x;
}

struct Buffer {
    void* data;
    std::size_t size;

    // Buffer(size_t size) : data(malloc(size)), size(size) {}

    ~Buffer() {
        free(data);
    }
};

inline void allocate_buffer(Buffer& buffer, std::size_t size) {
    buffer.data = std::malloc(size);
    buffer.size = size;
}

struct Memory {
    Buffer ui;
};

inline void allocate_all_memory(Memory& memory) {
    allocate_buffer(memory.ui, 5_MiB);
}
