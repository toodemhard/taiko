#include <exception>
#include <iostream>

#ifdef DEV
#define DEV_LOG(message) std::cerr << message;
#define DEV_PANIC(message)                                                                         \
    { std::cerr << message; throw std::runtime_error("message"); }
#else
#define DEV_LOG(string)
#define DEV_PANIC(string)
#endif
