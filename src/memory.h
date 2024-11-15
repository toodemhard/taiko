#pragma once

#include "allocator.h"

struct MemoryAllocators {
    linear_allocator ui_allocator;
};

template<typename T, typename... Args>
inline void reinitialize(T* object, Args&&... args) {
    object->~T();
    new (object) T(std::forward<Args>(args)...);
}
