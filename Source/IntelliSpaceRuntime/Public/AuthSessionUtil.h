#pragma once
#include "Kismet/GameplayStatics.h"
#include "AuthSaveGame.h"

// Small namespace with static helper functions
namespace AuthSession
{
    static const FString Slot = TEXT("AuthSlot"); // save slot name
    static const int32 UserIndex = 0;             // usually 0

    // Load session if it exists
    inline UAuthSaveGame* Load()
    {
        if (UGameplayStatics::DoesSaveGameExist(Slot, UserIndex))
        {
            return Cast<UAuthSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, UserIndex));
        }
        return nullptr;
    }

    // Save session object
    inline bool Save(UAuthSaveGame* Obj)
    {
        return Obj && UGameplayStatics::SaveGameToSlot(Obj, Slot, UserIndex);
    }

    // Delete session (logout)
    inline void Clear()
    {
        if (UGameplayStatics::DoesSaveGameExist(Slot, UserIndex))
        {
            UGameplayStatics::DeleteGameInSlot(Slot, UserIndex);
        }
    }
}
