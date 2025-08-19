//#pragma once
//#include "CoreMinimal.h"
//
//// C bridge used by tests; implemented in a .cpp/.mm in this module.
//extern "C" {
//    INTELLISPACERUNTIME_API void* CreateU2NetRunner();
//    INTELLISPACERUNTIME_API void  DestroyU2NetRunner(void* Runner);
//}
#pragma once
extern "C" {
    void* CreateU2NetRunner();
    void  DestroyU2NetRunner(void* runner);
}
