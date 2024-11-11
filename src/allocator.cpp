#include "allocator.h"
#include "memory.h"

#include <cstdint>
#include <memory>


// template <typename T, class allocator> class stl_adaptor {
//   public:
//     typedef T value_type;
//     allocator& m_allocator;
//
//     stl_adaptor(allocator& allocator) noexcept : m_allocator(allocator) {}
//
//     template <typename U> STLAdaptor(const STLAdaptor<U, Alloc>& other) noexcept : m_allocator(other.m_allocator) {}
//
//     constexpr T* allocate(std::size_t n) {
//         return reinterpret_cast<T*>(m_allocator.Allocate(n * sizeof(T), alignof(T)));
//     }
//
//     constexpr void deallocate(T* p, [[maybe_unused]] std::size_t n) noexcept {
//         m_allocator.Free(p);
//     }
//
//     std::size_t MaxAllocationSize() const noexcept {
//         return m_allocator.GetSize();
//     }
//
//     // bool operator==(const STLAdaptor<T,Alloc>& rhs) const noexcept
//     // {
//     //     if constexpr(std::is_base_of_v<FixedAllocator,Alloc>)
//     //     {
//     //         return m_allocator.GetStart() == rhs.m_allocator.GetStart();
//     //     }
//     //     else
//     //     {
//     //         DynamicAllocator::BlockDesc* a =
//     //             reinterpret_cast<DynamicAllocator*>
//     //             (&m_allocator)->m_currentBlock;
//     //
//     //         while(a->prevBlock != nullptr)
//     //         {
//     //             a = a->prevBlock;
//     //         }
//     //
//     //         DynamicAllocator::BlockDesc* b =
//     //             reinterpret_cast<DynamicAllocator*>
//     //             (&rhs.m_allocator)->m_currentBlock;
//     //
//     //         while(b->prevBlock != nullptr)
//     //         {
//     //             b = b->prevBlock;
//     //         }
//     //
//     //         return a == b;
//     //     }
//     // }
//
//     bool operator!=(const STLAdaptor<T, Alloc>& rhs) const noexcept {
//         return !(*this == rhs);
//     }
// };


// template <typename T> struct fixed_vector {
//     T* data;
//     int size{};
//     int capacity{};
//
//     fixed_vector(monotonic_allocator& allocator, int size) : capacity(size) {
//         data = (T*)allocator.allocate(sizeof(T) * size, alignof(T));
//     }
//
//     void push(T element) {
//         if (size >= capacity) {
//             return;
//         }
//         data[size] = element;
//         size++;
//     }
//
//     // template<typename T>
//     //     void alloc_fixed_vector(fixed_vector<T> vector, monotonic_allocator allocator) {
//     //         vector.data = allocator.allocate(sizeof )
//     //
//     //
//     //     }
// };
