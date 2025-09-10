// ScannerHUD.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScannerHUD.generated.h"

UCLASS()
class INTELLISPACE_API UScannerHUD : public UUserWidget
{
    GENERATED_BODY()
    
public:
    virtual bool Initialize() override;
    virtual void NativeConstruct() override;
    
    UFUNCTION(BlueprintCallable)
    void OnStartButtonClicked();
    
    UFUNCTION(BlueprintCallable)
    void OnStopButtonClicked();
    
    UFUNCTION(BlueprintCallable)
    void OnClearButtonClicked();
    
    UFUNCTION(BlueprintCallable)
    void OnExportButtonClicked();
    
    void SetScannerActor(class AARKitRoomScanner* Scanner);
    void UpdateStatus(const FString& Status);
    
private:
    class AARKitRoomScanner* ScannerActor;
    
    UPROPERTY()
    class UButton* StartButton;
    
    UPROPERTY()
    class UButton* StopButton;
    
    UPROPERTY()
    class UButton* ClearButton;
    
    UPROPERTY()
    class UButton* ExportButton;
    
    UPROPERTY()
    class UTextBlock* StatusText;
    
    void CreateUI();
};
