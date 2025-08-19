#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

DECLARE_DELEGATE_TwoParams(FOnFirstRunSubmitted, const FString& /*Name*/, const FString& /*Phone*/);

/**
 * Simple first-run form (pure Slate).
 * Renders centrally with two text boxes and a submit button.
 */
class SFirstRunSlate : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFirstRunSlate) {}
		SLATE_EVENT(FOnFirstRunSubmitted, OnSubmitted)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	bool Validate(FString& OutError) const;
	FReply OnClickSubmit();

private:
	FOnFirstRunSubmitted OnSubmitted;

	TSharedPtr<class SEditableTextBox> NameBox;
	TSharedPtr<class SEditableTextBox> PhoneBox;
	TSharedPtr<class STextBlock> ErrorLabel;
};
