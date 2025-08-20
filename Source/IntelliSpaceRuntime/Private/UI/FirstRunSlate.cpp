#include "UI/FirstRunSlate.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "FirstRun"

void SFirstRunSlate::Construct(const FArguments& InArgs)
{
    OnSubmitted = InArgs._OnSubmitted;

    ChildSlot
    [
        SNew(SBorder)
        .Padding(24.f)
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
            [
                SNew(STextBlock).Text(LOCTEXT("Welcome", "Welcome! Please enter your details"))
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
            [
                SNew(STextBlock).Text(LOCTEXT("NameLbl", "Name"))
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
            [
                SAssignNew(NameBox, SEditableTextBox)
                .MinDesiredWidth(420.f)
                .HintText(LOCTEXT("NameHint", "Jane Doe"))
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
            [
                SNew(STextBlock).Text(LOCTEXT("PhoneLbl", "Phone"))
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,16)
            [
                SAssignNew(PhoneBox, SEditableTextBox)
                .MinDesiredWidth(420.f)
                .HintText(LOCTEXT("PhoneHint", "+1 555 123 4567"))
            ]

            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
            [
                SNew(SButton)
                .Text(LOCTEXT("Submit", "Submit"))
                .OnClicked(this, &SFirstRunSlate::OnSubmitClicked)
            ]
        ]
    ];
}

FReply SFirstRunSlate::OnSubmitClicked()
{
    if (OnSubmitted.IsBound())
    {
        const FString Name  = NameBox.IsValid()  ? NameBox->GetText().ToString()  : FString();
        const FString Phone = PhoneBox.IsValid() ? PhoneBox->GetText().ToString() : FString();
        OnSubmitted.Execute(Name, Phone);
    }
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
