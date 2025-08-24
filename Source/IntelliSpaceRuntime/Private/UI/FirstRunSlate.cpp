#include "UI/FirstRunSlate.h" // must be first

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"

void SFirstRunSlate::Construct(const FArguments& InArgs)
{
    OnSubmit = InArgs._OnSubmit;

    ChildSlot
    [
        SNew(SBorder)
        .Padding(16)
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Welcome to IntelliSpace")))
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(8)
            [
                SNew(SButton)
                .Text(FText::FromString(TEXT("Continue")))
                .OnClicked(this, &SFirstRunSlate::HandleContinue)
            ]
        ]
    ];
}

FReply SFirstRunSlate::HandleContinue()
{
    if (OnSubmit.IsBound())
    {
        OnSubmit.Execute();
    }
    return FReply::Handled();
}
