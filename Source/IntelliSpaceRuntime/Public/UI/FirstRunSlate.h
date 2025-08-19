#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

DECLARE_DELEGATE_TwoParams(FOnFirstRunSubmitted, const FString& /*Name*/, const FString& /*Phone*/);

class INTELLISPACERUNTIME_API SFirstRunSlate : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFirstRunSlate) {}
        SLATE_EVENT(FOnFirstRunSubmitted, OnSubmitted)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FReply HandleSubmit();
    bool   IsSubmitEnabled() const;

private:
    // UI state
    TSharedPtr<class SEditableTextBox> NameBox;
    TSharedPtr<class SEditableTextBox> PhoneBox;

    // Callback
    FOnFirstRunSubmitted OnSubmitted;
};
