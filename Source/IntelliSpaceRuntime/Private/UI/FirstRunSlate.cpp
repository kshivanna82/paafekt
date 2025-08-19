#include "UI/FirstRunSlate.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "FirstRunSlate"

void SFirstRunSlate::Construct(const FArguments& InArgs)
{
    OnSubmitted = InArgs._OnSubmitted;

    ChildSlot
    [
        SNew(SBorder)
        .Padding(24.f)
        [
            SNew(SBox)
            .WidthOverride(520.f)
            [
                SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 0.f, 0.f, 12.f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("Title", "Welcome!"))
                    .Font(FSlateFontInfo("Verdana", 20))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 0.f, 0.f, 6.f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("NameLabel", "Your Name"))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 0.f, 0.f, 12.f)
                [
                    SAssignNew(NameBox, SEditableTextBox)
                    .HintText(LOCTEXT("NameHint", "e.g. Alex Li"))
                    .MinDesiredWidth(480.f)
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 0.f, 0.f, 6.f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("PhoneLabel", "Phone Number"))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.f, 0.f, 0.f, 16.f)
                [
                    SAssignNew(PhoneBox, SEditableTextBox)
                    .HintText(LOCTEXT("PhoneHint", "+1 555 123 4567"))
                    .MinDesiredWidth(480.f)
                    // NOTE: Avoided KeyboardType to keep 5.4 compatibility
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SButton)
                    .Text(LOCTEXT("Continue", "Continue"))
                    .IsEnabled(this, &SFirstRunSlate::IsSubmitEnabled)
                    .OnClicked(this, &SFirstRunSlate::HandleSubmit)
                ]
            ]
        ]
    ];
}

bool SFirstRunSlate::IsSubmitEnabled() const
{
    const bool bHasName  = NameBox.IsValid()  && !NameBox->GetText().ToString().TrimStartAndEnd().IsEmpty();
    const bool bHasPhone = PhoneBox.IsValid() && !PhoneBox->GetText().ToString().TrimStartAndEnd().IsEmpty();
    return bHasName && bHasPhone;
}

FReply SFirstRunSlate::HandleSubmit()
{
    const FString Name  = NameBox.IsValid()  ? NameBox->GetText().ToString().TrimStartAndEnd()  : FString();
    const FString Phone = PhoneBox.IsValid() ? PhoneBox->GetText().ToString().TrimStartAndEnd() : FString();

    if (OnSubmitted.IsBound())
    {
        OnSubmitted.Execute(Name, Phone);
    }
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
