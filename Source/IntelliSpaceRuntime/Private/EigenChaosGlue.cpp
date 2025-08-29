// EigenChaosGlue.cpp  — provides allocators Chaos/Eigen expect at link time.
#include "CoreMinimal.h"
#include "HAL/UnrealMemory.h"

// Use size_t — on arm64 this demangles as 'unsigned long', matching the callers.
//void* StdMalloc(size_t Size, size_t Alignment)
//{
//    return FMemory::Malloc((SIZE_T)Size, (uint32)Alignment);
//}
//
//void* StdRealloc(void* Ptr, size_t NewSize, size_t Alignment)
//{
//    return FMemory::Realloc(Ptr, (SIZE_T)NewSize, (uint32)Alignment);
//}
//
//void  StdFree(void* Ptr)
//{
//    FMemory::Free(Ptr);
//}

