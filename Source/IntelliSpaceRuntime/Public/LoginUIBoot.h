#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoginUIBoot.generated.h"

class UButton;
class UTextBlock;
class UEditableTextBox;
class ASegDecorActor;

class ULoginWidget;
/**
 * Login UI that collects phone + OTP and navigates to SegDecorActor when Verify is clicked.
 * Drop-in replacement focusing on Verify OTP flow and robust label rendering.
 */
UCLASS()
class INTELLISPACERUNTIME_API ULoginUIBoot : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    
    UFUNCTION(BlueprintCallable, Category="Login")
    void Init(UWorld* World);
    
    /** EXACTLY matches ULoginWidget::FOnVerifyOtp(const FString&, const FString&) */
    UFUNCTION()
    void OnVerifyClicked(FString Phone, FString Code);

    /** Optional: matches ULoginWidget::FOnSendOtp(const FString&) */
    UFUNCTION()
    void OnSendClicked(FString Phone);

    /** Switch view/camera and close the login UI. */
    UFUNCTION(BlueprintCallable, Category="Login")
    void NavigateToSegDecor();

    /** Assign WBP_Login (parent class MUST be ULoginWidget) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Login")
    TSubclassOf<ULoginWidget> LoginWidgetClass = nullptr;

protected:
    /** Verify button from the designer (optional so we can still spawn a label if missing). */
    UPROPERTY(meta=(BindWidgetOptional))
    UButton* VerifyButton = nullptr;

    /** Optional label inside the Verify button. If not present, code will create one. */
    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* VerifyLabel = nullptr;

    /** Optional inputs, kept here to preserve serialized layout; not required by this patch. */
    UPROPERTY(meta=(BindWidgetOptional))
    UEditableTextBox* PhoneText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UEditableTextBox* OtpText = nullptr;

    /** Called when Verify is clicked. */
//    UFUNCTION()
//    void OnVerifyClicked();

    /** Makes sure the Verify button displays a visible "Verify OTP" label. */
    void EnsureVerifyLabel();

//    /** Performs the actual navigation/spawn logic to SegDecorActor and restores game input. */
//    void NavigateToSegDecor();

    /** Allow overriding the exact class to navigate to (settable in BP if needed). */
    UPROPERTY(EditDefaultsOnly, Category="Navigation")
    TSubclassOf<AActor> SegDecorActorOverrideClass = nullptr;

    /** Spawn transform used if SegDecorActor is not already present. */
    UPROPERTY(EditDefaultsOnly, Category="Navigation")
    FTransform SegDecorSpawnTransform = FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::OneVector);
    
private:
    /** Live instance kept here so it isn't GC'd */
    UPROPERTY()
    ULoginWidget* LoginWidget = nullptr;
};
