#pragma once
#include "CoreMinimal.h"

// HTTP types used in this header:
#include "Http.h"                 // FHttpRequestPtr, FHttpResponsePtr
// (optional) only if you call FHttpModule::Get() in the header:
#include "HttpModule.h"
#include "Blueprint/UserWidget.h"

#include "LoginWidget.generated.h"

// Forward declarations
class UButton;
class UEditableTextBox;
class UTextBlock;
class UVerticalBox;
class UImage;
class UBorder;

/**
 * Login Widget for IntelliSpace
 * Handles phone number authentication with OTP
 */
UCLASS()
class INTELLISPACE_API ULoginWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    ULoginWidget(const FObjectInitializer& ObjectInitializer);

    // Widget components - bind these in Blueprint
    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* PhoneBox;

    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* OtpBox;

    UPROPERTY(meta = (BindWidget))
    class UButton* SendOtpButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* VerifyButton;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* TitleText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* StatusText;

    UPROPERTY(meta = (BindWidgetOptional))
    class UVerticalBox* MainContainer;

    UPROPERTY(meta = (BindWidgetOptional))
    class UImage* LogoImage;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* PhoneLabel;

    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* OtpLabel;

protected:
    virtual void NativeConstruct() override;

    // Button click handlers
    UFUNCTION()
    void OnSendOtpClicked();

    UFUNCTION()
    void OnVerifyClicked();

    // Text change handlers
    UFUNCTION()
    void OnPhoneTextChanged(const FText& Text);

    UFUNCTION()
    void OnOtpTextChanged(const FText& Text);

    // HTTP Response handlers
    void OnOtpSent(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    
    void OnOtpVerified(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    // Helper functions
    UFUNCTION()
    void SetupUIElements();
    
    UFUNCTION()
    bool ValidatePhoneNumber(const FString& PhoneNumber);
    
    UFUNCTION()
    void SendOtpRequest(const FString& PhoneNumber);
    
    UFUNCTION()
    void VerifyOtpRequest(const FString& PhoneNumber, const FString& OtpCode);
    
    UFUNCTION()
    void ShowMessage(const FString& Message, const FLinearColor& Color);
    
    UFUNCTION()
    void StoreAuthToken(const FString& Token);
    
//    UFUNCTION()
//    void OnOtpExpired();

private:
    // OTP state management
    bool bIsOtpSent;
    FTimerHandle OtpExpiryTimer;
    float OtpExpiryTime;

    // User data
    FString CurrentPhoneNumber;
    FString CurrentSessionToken;

public:
    // API Endpoints (can be configured in Blueprint)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "API")
    FString SendOtpEndpoint = "https://api.intellispace.com/auth/send-otp";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "API")
    FString VerifyOtpEndpoint = "https://api.intellispace.com/auth/verify-otp";

    // UI Configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    FLinearColor PrimaryColor = FLinearColor(0.0f, 0.52f, 1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    FLinearColor SecondaryColor = FLinearColor(0.16f, 0.65f, 0.27f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    FLinearColor BackgroundColor = FLinearColor(0.15f, 0.15f, 0.2f, 1.0f);

    // Events that can be implemented in Blueprint
    UFUNCTION(BlueprintImplementableEvent, Category = "Login")
    void OnLoginSuccess(const FString& AuthToken);

    UFUNCTION(BlueprintImplementableEvent, Category = "Login")
    void OnLoginFailed(const FString& ErrorMessage);

    // Blueprint callable functions
    UFUNCTION(BlueprintCallable, Category = "Login")
    void ResetLoginForm();

    UFUNCTION(BlueprintCallable, Category = "Login")
    void ShowOtpStep();

    UFUNCTION(BlueprintCallable, Category = "Login")
    FString GetPhoneNumber() const { return CurrentPhoneNumber; }

    UFUNCTION(BlueprintCallable, Category = "Login")
    bool IsOtpSent() const { return bIsOtpSent; }
};
