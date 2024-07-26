#ifdef DEBUG
#define DEBUG_LOG(string) std::cerr << string;
#define DEBUG_PANIC(string) {std::cerr << string; exit(1);}
#else
#define DEBUG_LOG(string)
#define DEBUG_PANIC(string) 
#endif
