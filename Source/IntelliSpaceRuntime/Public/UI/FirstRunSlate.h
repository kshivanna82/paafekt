#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

// Simple submit delegate (no params)
DECLARE_DELEGATE(FOnSubmit)

/** Minimal first-run panel */
class SFirstRunSlate : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFirstRunSlate) {}
        SLATE_EVENT(FOnSubmit, OnSubmit)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FOnSubmit OnSubmit;
    FReply HandleContinue();
};
