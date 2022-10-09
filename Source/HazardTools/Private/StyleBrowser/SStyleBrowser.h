// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"
#include "HazardToolsStyleBrowserSettings.h"
#include "Widgets/SCompoundWidget.h"
#include "Misc/TextFilter.h"
#include "StyleBrowserFwd.h"
struct FSegmentedControlStyle;
class SRichTextBlock;
/**
 * 
 */
namespace HazardTools
{
class SStyleBrowser : public SCompoundWidget
{
	using ThisClass = SStyleBrowser;
	// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
	using SettingsClass = UHazardToolsStyleBrowserSettings;
public:
	virtual ~SStyleBrowser() override;

	SLATE_BEGIN_ARGS(SStyleBrowser)
		{
		}

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	/** @return Returns a string to use for highlighting results in the outliner list */
	TAttribute<FText> GetTextFilterHighlightText() const;

	static const FName Column_ID_Name;
	static const FName Column_ID_Set;
	static const FName Column_ID_Type;
	static const FName Column_ID_Preview;

private:
	TSharedRef<SWidget> MakeToolbar();
	TSharedRef<SWidget> MakeListView();
	TSharedRef<SWidget> MakePreviewArea();
	TSharedRef<ITableRow> HandleListGenerateRow(FStyleBrowserItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
	float GetAvailableStyleSetsDropDownButtonWidth() const;
	TSharedRef<SWidget> GetAvailableStyleSetsDropDownButtonContent() const;

	/** @return	Returns true if the text filter is currently active */
	bool IsTextFilterActive() const;

	/** @return	Returns the filter status text */
	FText GetFilterStatusText() const;

	/** @return Returns color for the filter status text message, based on success of search filter */
	FSlateColor GetFilterStatusTextColor() const;

	void HandleListSelectionChanged(FStyleBrowserItemPtr InItem, ESelectInfo::Type SelectInfo) const;

	void Populate() const;

	/** Called by the editable text control when the filter text is changed by the user */
	void OnFilterTextChanged(const FText& InFilterText) const;

	FReply OnCopyStyleNameClicked() const;
	FReply OnCopyCodeSnippedClicked() const;

	/** Populates OutSearchStrings with the strings associated with TreeItem that should be used in searching */
	void PopulateSearchStrings(const FStyleBrowserItemPtr& Item, TArray<FString>& OutSearchStrings) const;
	bool ShouldItemPassTextFilter(const FStyleBrowserItemPtr& Item) const;

	TSharedPtr<SSearchBox> FilterTextBoxWidget; // Widget containing the filtering text box
	TSharedPtr<SListView<FStyleBrowserItemPtr>> ListView;
	//TSharedPtr<SButton> PreviewAreaHeaderButton;		
	TSharedPtr<STextBlock> PreviewAreaHeaderText;
	TSharedPtr<SRichTextBlock> PreviewAreaHeaderSnippedText;
	TSharedPtr<SRichTextBlock> PreviewAreaFooterText;
	TSharedPtr<SBox> PreviewAreaBox;
	TSharedPtr<FStyleBrowserModel> Model;
	TSharedPtr<TTextFilter<const FStyleBrowserItemPtr&>> SearchBoxFilter;

	TSharedPtr<FSegmentedControlStyle> CustomSegmentedControlStyle;
};
}
