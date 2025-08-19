//#include "U2NetRunnerBridge.h"
//
//extern "C" {
//
//void* CreateU2NetRunner()
//{
//    // Stub instance; tests just need the symbol (and usually don't deref).
//    static int64 Dummy;
//    return &Dummy;
//}
//
//void DestroyU2NetRunner(void* /*Runner*/)
//{
//    // Nothing to do in the stub.
//}
//
//} // extern "C"
#include "U2NetRunnerBridge.h"

#if !PLATFORM_IOS
// Non-iOS fallback (or your other-platform implementation)
void* CreateU2NetRunner()          { return nullptr; }
void  DestroyU2NetRunner(void*)    {}
#endif
