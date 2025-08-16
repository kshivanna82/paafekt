#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "LoginActor.generated.h"

UCLASS()
class ALoginActor : public AActor
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, Category="UI") TSubclassOf<UUserWidget> LoginWidgetClass;
    UPROPERTY(EditAnywhere, Category="Flow") FName MainLevelName = "Main";

private:
    UFUNCTION() void OnSendClicked();
    UFUNCTION() void OnVerifyClicked();
    UFUNCTION() void OnLogoutClicked();
    void ProceedToMain();
    UUserWidget* LoginWidget = nullptr;
};
