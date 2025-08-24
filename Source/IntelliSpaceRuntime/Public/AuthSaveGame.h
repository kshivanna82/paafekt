#pragma once
#include "GameFramework/SaveGame.h"
#include "AuthSaveGame.generated.h"

UCLASS()
class INTELLISPACERUNTIME_API UAuthSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category="Auth")
    FString UserId;

    UPROPERTY(BlueprintReadWrite, Category="Auth")
    FString AuthToken;         // e.g., server JWT or opaque token

    UPROPERTY(BlueprintReadWrite, Category="Auth")
    bool bLoggedIn = false;

    UPROPERTY(BlueprintReadWrite, Category="Auth")
    FDateTime TokenExpiryUtc;  // optional

    UFUNCTION(BlueprintCallable, Category="Auth")
    bool IsValidSession() const
    {
        if (!bLoggedIn) return false;
        if (TokenExpiryUtc.GetTicks() == 0) return true; // no expiry used
        return FDateTime::UtcNow() < TokenExpiryUtc;
    }
};
