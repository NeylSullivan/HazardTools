// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"
#include "SlateOptMacros.h"
#include "StyleBrowserFwd.h"
#include "StyleBrowserItem.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

namespace HazardTools
{
class SStyleBrowserTableRow : public SMultiColumnTableRow<FStyleBrowserItemRef>
{
public:
	SLATE_BEGIN_ARGS(SStyleBrowserTableRow)
		{
		}

	SLATE_END_ARGS()

public:
	SStyleBrowserTableRow()
	{
	}

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	/**
	 * Constructs the widget.
	 *
	 * @param InOwnerTableView
	 * @param InItem
	 * @param InOwnerStyleBrowser
	 */
	void Construct(const FArguments& /*InArgs*/, const TSharedRef<STableViewBase>& InOwnerTableView, const FStyleBrowserItemRef InItem, const TSharedRef<SStyleBrowser> InOwnerStyleBrowser)
	{
		Item = InItem;
		StyleBrowserWeakPtr = InOwnerStyleBrowser;

		SMultiColumnTableRow<FStyleBrowserItemRef>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (const auto ObjectBrowser = StyleBrowserWeakPtr.Pin())
		{
			HighlightText = ObjectBrowser->GetTextFilterHighlightText();
		}

		if (Item.IsValid())
		{
			if (ColumnName == SStyleBrowser::Column_ID_Name)
			{
				return
					SNew(STextBlock)
					.Text(FText::FromName(Item->Name))
					.HighlightText(HighlightText);
			}

			// Only draw style set name for items added from parent style set
			if (ColumnName == SStyleBrowser::Column_ID_Set && Item->bAddedFromParentStyle)
			{
				return
					SNew(STextBlock)
					.Text(FText::FromName(Item->StyleSetName))
					.HighlightText(HighlightText);
			}

			if (ColumnName == SStyleBrowser::Column_ID_Type)
			{
				return
					SNew(STextBlock)
					.Text(FText::FromName(Item->GetTypeName()))
					.HighlightText(HighlightText);
			}

			if (ColumnName == SStyleBrowser::Column_ID_Preview)
			{
				TSharedRef<SWidget> PreviewWidget = Item->GetPreviewColumnWidget();
				// If displaying simple text block widget inject text highlighting
				if (PreviewWidget->GetWidgetClass().GetWidgetType() == STextBlock::StaticWidgetClass().GetWidgetType())
				{
					const TSharedRef<STextBlock> PreviewWidgetAsTextBlock = StaticCastSharedRef<STextBlock>(PreviewWidget);
					PreviewWidgetAsTextBlock->SetHighlightText(HighlightText);
				}

				return PreviewWidget;
			}
		}

		return SNullWidget::NullWidget;
	}

	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

private:
	FStyleBrowserItemPtr Item;
	TWeakPtr<SStyleBrowser> StyleBrowserWeakPtr;
	TAttribute<FText> HighlightText;
};
}
