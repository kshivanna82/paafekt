#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoginUIBoot.generated.h"

class UButton;
class UTextBlock;
class UEditableTextBox;

UCLASS()
class INTELLISPACERUNTIME_API ULoginUIBoot : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

protected:
    // Bind these to your UMG widget via Designer (same names) or create in C++
    UPROPERTY(meta=(BindWidget)) UEditableTextBox* PhoneTextBox = nullptr;
    UPROPERTY(meta=(BindWidget)) UEditableTextBox* OtpTextBox   = nullptr;
    UPROPERTY(meta=(BindWidget)) UButton*          VerifyButton = nullptr;
    UPROPERTY(meta=(BindWidget)) UTextBlock*       VerifyLabel  = nullptr;

private:
    UFUNCTION() void OnVerifyClicked();

    // helpers
    void StyleVerifyButton();
    void StyleTextBoxes();
};
