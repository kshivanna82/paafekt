#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AuthSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOtpStarted, bool, bOk);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOtpVerified, bool, bOk);

USTRUCT(BlueprintType)
struct INTELLISPACERUNTIME_API FAuthProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString UserId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Phone;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Token;
};

/**
 * Simple OTP auth + JSON persistence used by tests.
 */
UCLASS(BlueprintType)
class INTELLISPACERUNTIME_API UAuthSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem lifecycle
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Public API used by game & tests
    UFUNCTION(BlueprintCallable, Category="Auth")
    void StartOtp(const FString& PhoneNumber, const FString& CountryCode);

    UFUNCTION(BlueprintCallable, Category="Auth")
    void VerifyOtp(const FString& OtpCode);

    UFUNCTION(BlueprintCallable, Category="Auth")
    void Logout();

    UFUNCTION(BlueprintPure, Category="Auth")
    bool IsLoggedIn() const { return !Profile.Token.IsEmpty(); }

    // Test helpers (called directly by C++ tests)
    void __Test_SetJsonPath(const FString& NewPath); // overrides default save path
    void __Test_ClearProfile();                       // clears profile + saves

    // Signals
    UPROPERTY(BlueprintAssignable, Category="Auth")
    FOnOtpStarted OnOtpStarted;

    UPROPERTY(BlueprintAssignable, Category="Auth")
    FOnOtpVerified OnOtpVerified;

    // Optional getter for verification in tests or BP
    const FAuthProfile& GetProfile() const { return Profile; }

private:
    void LoadFromDisk();
    void SaveToDisk() const;
    static FString DefaultJsonPath();

private:
    FAuthProfile Profile;
    FString JsonPath;
};
