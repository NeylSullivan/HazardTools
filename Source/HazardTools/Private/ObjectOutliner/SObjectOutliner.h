// Copyright Neyl Sullivan 2022

#pragma once

#include "ObjectOutlinerFwd.h"
#include "Misc/TextFilter.h"
#include "Widgets/SCompoundWidget.h"

namespace HazardTools
{
class SObjectOutliner : public SCompoundWidget
{
	using ThisClass = SObjectOutliner;
	// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
	using SettingsClass = UHazardToolsObjectOutlinerSettings;
public:
	virtual ~SObjectOutliner() override;

	SLATE_BEGIN_ARGS(SObjectOutliner)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** @return Returns a string to use for highlighting results in the outliner list */
	TAttribute<FText> GetTextFilterHighlightText() const;
	static bool IsTreeViewMode();

	static const FName Column_ID_Name;
	static const FName Column_ID_Class;
	static const FName Column_ID_Memory;

private:
	TSharedRef<SHorizontalBox> MakeToolbar();
	TSharedRef<SWidget> MakeTreeView();
	TSharedRef<::IDetailsView> MakePropertyEditor();
	TSharedRef<SHorizontalBox> MakeImperativeFilterButtons();
	TSharedRef<SWidget> GetDropDownFiltersButtonContent();
	void HandleToggleAllDropDownFilters();

	TSharedRef<ITableRow> HandleListGenerateRow(FObjectOutlinerItemPtr ObjectPtr, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedPtr<SWidget> GetContextMenuContent() const;


	void Populate() const;

	void HandleListSelectionChanged(FObjectOutlinerItemPtr InItem, ESelectInfo::Type SelectInfo) const;

	static void OnGetChildrenForOutlinerTree(FObjectOutlinerItemPtr InParent, TArray<FObjectOutlinerItemPtr>& OutChildren);

	/** Handler for recursively expanding/collapsing items */
	void SetItemExpansionRecursive(FObjectOutlinerItemPtr ItemPtr, bool bInExpansionState);

	/** Called by the editable text control when the filter text is changed by the user */
	void OnFilterTextChanged(const FText& InFilterText) const;

	/** Populates OutSearchStrings with the strings associated with TreeItem that should be used in searching */
	void PopulateSearchStrings(const UObject& TreeItem, TArray<FString>& OutSearchStrings) const;

	FReply OnRefreshClicked() const;

	/** @return	Returns the filter status text */
	FText GetFilterStatusText() const;

	/** @return Returns color for the filter status text message, based on success of search filter */
	FSlateColor GetFilterStatusTextColor() const;

	/** @return	Returns true if the text filter is currently active */
	bool IsTextFilterActive() const;

	/** Handles column sorting mode change */
	void OnColumnSortModeChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode) const;

	/** Sort the specified array of items based on the current sort column */
	static void SortItems(TArray<FObjectOutlinerItemPtr>& Items);

	bool ShouldItemPassFilter(const UObject* ObjectPtr) const;
	bool ShouldItemPassTextFilter(const UObject* ObjectPtr) const;

	/** @return Returns the current sort mode of the specified column */
	static EColumnSortMode::Type GetColumnSortMode(const FName ColumnId);

	TSharedPtr<SHeaderRow> HeaderRowWidget; // The header row of the scene outliner
	TSharedPtr<STreeView<FObjectOutlinerItemPtr>> TreeView;
	TSharedPtr<::IDetailsView> PropertyEditor;
	TSharedPtr<SSearchBox> FilterTextBoxWidget;      // Widget containing the filtering text box
	TSharedPtr<SComboButton> ViewOptionsComboButton; // The button that displays view options

	TArray<TSharedPtr<FObjectOutlinerFilter>> ImperativeFilters;
	TArray<TSharedPtr<FObjectOutlinerFilter>> DropDownFilters;
	//bool bShowOnlyCheckedObjects = false;
	TSharedPtr<TTextFilter<const UObject&>> SearchBoxFilter;

	TSharedPtr<FObjectOutlinerModel> Model;
};
};
