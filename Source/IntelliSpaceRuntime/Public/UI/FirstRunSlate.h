#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

DECLARE_DELEGATE_TwoParams(FOnFirstRunSubmitted, const FString& /*Name*/, const FString& /*Phone*/);

class SFirstRunSlate : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFirstRunSlate) {}
        SLATE_EVENT(FOnFirstRunSubmitted, OnSubmitted)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FOnFirstRunSubmitted OnSubmitted;
    TSharedPtr<class SEditableTextBox> NameBox;
    TSharedPtr<class SEditableTextBox> PhoneBox;

    FReply OnSubmitClicked();
};
