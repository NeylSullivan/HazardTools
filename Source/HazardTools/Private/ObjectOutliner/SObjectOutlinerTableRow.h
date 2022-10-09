// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"
#include "SObjectOutliner.h"
#include "Widgets/SWidget.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"

namespace HazardTools
{
class SObjectOutlinerTableRow : public SMultiColumnTableRow<FObjectOutlinerItemRef>
{
public:
	SLATE_BEGIN_ARGS(SObjectOutlinerTableRow)
		{
		}

	SLATE_END_ARGS()

private:
	FObjectOutlinerItemPtr Item;

	// Weak ptr to the outliner widget that owns our list to get current filter(highlight) text
	TWeakPtr<SObjectOutliner> SceneOutlinerWeakPtr;

	const FSlateBrush* ClassIcon = nullptr;
	FSlateColor ContentColor;

	TAttribute<FText> HighlightText;

	FText Name;
	FText ClassName;
	FText Package;

public:
	SObjectOutlinerTableRow()
	{
	}

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The construction arguments.
	 * @param InOwnerTableView
	 * @param InItem
	 * @param InOwnerObjectOutliner
	 */
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, const FObjectOutlinerItemRef InItem, const TSharedRef<SObjectOutliner> InOwnerObjectOutliner);

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	FText GetMemoryText() const;
};
}
