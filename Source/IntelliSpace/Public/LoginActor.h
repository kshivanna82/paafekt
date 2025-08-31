// LoginActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LoginActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoginComplete);

UCLASS()
class INTELLISPACE_API ALoginActor : public AActor
{
    GENERATED_BODY()

public:
    ALoginActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Login")
    TSubclassOf<class ULoginWidget> LoginWidgetClass;
    
    UPROPERTY(BlueprintAssignable, Category = "Login")
    FOnLoginComplete OnLoginComplete;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void ShowLoginUI();
    void HideLoginUI();
    void SpawnFurniLife();
    
    UFUNCTION()
    void OnSendOtpClicked(FString Phone);
    
    UFUNCTION()
    void OnVerifyOtpClicked(FString Phone, FString Code);
    
    UFUNCTION()
    void OnOtpStarted(bool bSuccess);
    
    UFUNCTION()
    void OnOtpVerified(bool bSuccess);

private:
    UPROPERTY()
    class UAuthSubsystem* Auth;
    
    UPROPERTY()
    class ULoginWidget* LoginWidget;
    
    FString CurrentPhone;
};
