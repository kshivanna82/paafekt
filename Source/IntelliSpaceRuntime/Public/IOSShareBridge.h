#pragma once
#include "CoreMinimal.h"
#if PLATFORM_IOS
extern void IOS_ShowShareSheetForFile(const TCHAR* InPath);
#else
inline void IOS_ShowShareSheetForFile(const TCHAR*) {}
#endif
