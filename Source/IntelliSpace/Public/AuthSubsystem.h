// AuthSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AuthSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOtpStarted, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOtpVerified, bool, bSuccess);

USTRUCT(BlueprintType)
struct FAuthProfile
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite)
    FString UserId;
    
    UPROPERTY(BlueprintReadWrite)
    FString Name;
    
    UPROPERTY(BlueprintReadWrite)
    FString Phone;
    
    UPROPERTY(BlueprintReadWrite)
    FString Token;
};

UCLASS(BlueprintType)
class INTELLISPACE_API UAuthSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="Auth")
    void StartOtp(const FString& PhoneNumber, const FString& CountryCode);

    UFUNCTION(BlueprintCallable, Category="Auth")
    void VerifyOtp(const FString& OtpCode);

    UFUNCTION(BlueprintCallable, Category="Auth")
    void Logout();

    UFUNCTION(BlueprintPure, Category="Auth")
    bool IsLoggedIn() const { return !Profile.Token.IsEmpty(); }

    const FAuthProfile& GetProfile() const { return Profile; }

    UPROPERTY(BlueprintAssignable, Category="Auth")
    FOnOtpStarted OnOtpStarted;

    UPROPERTY(BlueprintAssignable, Category="Auth")
    FOnOtpVerified OnOtpVerified;

private:
    void LoadFromDisk();
    void SaveToDisk() const;
    static FString GetSavePath();

    FAuthProfile Profile;
};
