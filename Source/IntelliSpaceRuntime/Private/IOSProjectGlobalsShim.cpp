// IOSProjectGlobalsShim.cpp
// Provides iOS link-time definitions for a couple of Core globals.

#include "CoreMinimal.h"

#if PLATFORM_IOS && !WITH_EDITOR

// Whether this executable is "game agnostic" (false for a normal game/app).
CORE_API bool GIsGameAgnosticExe = false;
// Matches CoreGlobals.h: extern CORE_API TCHAR GInternalProjectName[64];
CORE_API TCHAR GInternalProjectName[64] = TEXT("IntelliSpace");
// NOTE: Do NOT define GIsGameAgnosticExe here unless the linker
// complains it's missing. If it does, add:
//   CORE_API bool GIsGameAgnosticExe = false;
#endif // PLATFORM_IOS && !UE_EDITOR
