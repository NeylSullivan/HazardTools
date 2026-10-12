// Copyright Neyl Sullivan 2026

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UWidgetBlueprint;
class SMultiLineEditableTextBox;

namespace HazardTools
{
	class SHazardUMGToSlate : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SHazardUMGToSlate) {}
			SLATE_ARGUMENT(UWidgetBlueprint*, Blueprint)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		void SetBlueprint(UWidgetBlueprint* Blueprint);
		static void Open(UWidgetBlueprint* Blueprint);
		static void RegisterMenus();
		static const FName TabName;

	private:
		FReply Generate();
		FReply CopyCode() const;
		TWeakObjectPtr<UWidgetBlueprint> Source;
		TSharedPtr<SMultiLineEditableTextBox> CodeView;
		FString Code;
		FText Status;
		FText WarningText;
		bool bOnlyNonDefaultValues = true;
	};
}