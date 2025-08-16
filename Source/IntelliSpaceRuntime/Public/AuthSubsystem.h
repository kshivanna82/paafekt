#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AuthSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FISUserProfile {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString UserId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Name;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Phone;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Token;
};

UCLASS()
class UAuthSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Coll) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable) bool IsLoggedIn() const { return !Profile.Token.IsEmpty(); }
    UFUNCTION(BlueprintCallable) const FISUserProfile& GetProfile() const { return Profile; }

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOtpStarted, bool, bOk);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOtpVerified, bool, bOk);

    UPROPERTY(BlueprintAssignable) FOtpStarted OnOtpStarted;
    UPROPERTY(BlueprintAssignable) FOtpVerified OnOtpVerified;

    UFUNCTION(BlueprintCallable) void StartOtp(const FString& Phone, const FString& Name);
    UFUNCTION(BlueprintCallable) void VerifyOtp(const FString& Code);
    UFUNCTION(BlueprintCallable) void Logout();

    void Save(); void Load();

#if WITH_DEV_AUTOMATION_TESTS
    void __Test_SetJsonPath(const FString& InPath) { JsonPath = InPath; }
    void __Test_ClearProfile() { Profile = {}; }
#endif

private:
    FString JsonPath;
    FString PendingRequestId;
    FISUserProfile Profile;
};
