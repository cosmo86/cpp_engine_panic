#progma once 
#include <chrono>

inline uint64_t CurrentTime_nanoseconds()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>
              (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}  