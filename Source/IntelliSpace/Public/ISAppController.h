// ISAppController.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ISAppController.generated.h"

UENUM(BlueprintType)
enum class EAppState : uint8
{
    Login,
    Segmentation,
    MainMenu
};

UCLASS()
class INTELLISPACE_API AISAppController : public AActor
{
    GENERATED_BODY()

public:
    AISAppController();

    UFUNCTION(BlueprintCallable, Category = "AppController")
    void ChangeAppState(EAppState NewState);
    
    UFUNCTION(BlueprintPure, Category = "AppController")
    EAppState GetCurrentState() const { return CurrentState; }
    
    UFUNCTION(BlueprintCallable, Category = "AppController", meta = (WorldContext = "WorldContextObject"))
    static AISAppController* GetInstance(UObject* WorldContextObject);

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    EAppState CurrentState;
    
    UPROPERTY()
    class ALoginActor* LoginActor;
    
    UPROPERTY()
    class AFurniLife* FurniLifeActor;
    
    static AISAppController* Instance;
};
