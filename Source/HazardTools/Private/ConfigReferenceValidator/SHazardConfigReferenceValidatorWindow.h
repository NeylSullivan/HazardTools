// Copyright Neyl Sullivan 2026

#pragma once

#include "CoreMinimal.h"
#include "ConfigReferenceValidator/HazardConfigReferenceValidatorTypes.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

class SSearchBox;

namespace HazardTools
{
struct FHazardConfigReferenceTreeItem
{
	TWeakObjectPtr<UClass> DeclaringClass;
	FHazardConfigReferenceRowPtr Row;
	TArray<TSharedPtr<FHazardConfigReferenceTreeItem>> Children;

	bool IsGroup() const { return !Row.IsValid(); }
};

using FHazardConfigReferenceTreeItemPtr = TSharedPtr<FHazardConfigReferenceTreeItem>;

class SHazardConfigReferenceValidatorWindow : public SCompoundWidget
{
	using ThisClass = SHazardConfigReferenceValidatorWindow;

public:
	SLATE_BEGIN_ARGS(SHazardConfigReferenceValidatorWindow)
		{
		}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void ApplyFixes(const TArray<FHazardConfigReferenceRowPtr>& RowsToFix);

	static const FName Column_Status;
	static const FName Column_SettingsClass;
	static const FName Column_ConfigFile;
	static const FName Column_ConfigSection;
	static const FName Column_PropertyPath;
	static const FName Column_ReferenceType;
	static const FName Column_CurrentPath;
	static const FName Column_ResolvedObject;
	static const FName Column_Replacement;
	static const FName Column_Actions;

private:
	TSharedRef<SWidget> MakeToolbar();
	TSharedRef<SWidget> MakeResultsTable();
	TSharedRef<ITableRow> HandleGenerateRow(FHazardConfigReferenceTreeItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleGetChildren(FHazardConfigReferenceTreeItemPtr Item, TArray<FHazardConfigReferenceTreeItemPtr>& OutChildren) const;

	void Scan();
	void RefreshFilter();
	void RefreshTable();
	void HandleSearchChanged(const FText& InSearchText);
	void SetShowInvalidOnly(ECheckBoxState NewState);
	void SetShowEmpty(ECheckBoxState NewState);
	void SetShowUnsupported(ECheckBoxState NewState);
	void SetProjectSettingsOnly(ECheckBoxState NewState);
	void SetIncludePluginSettings(ECheckBoxState NewState);
	void SetIncludeEngineSettings(ECheckBoxState NewState);

	FReply ScanClicked();
	FReply RescanClicked();
	FReply FixSelectedClicked();
	FReply FixAllFixableClicked();
	FReply ClearPendingFixesClicked();
	FReply CopyReportClicked() const;

	bool DoesRowMatchFilter(const FHazardConfigReferenceRowPtr& Row) const;
	void RebuildTree();
	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;
	FText GetSearchHighlightText() const;
	TArray<FHazardConfigReferenceRowPtr> GetSelectedRows() const;
	FString BuildReport() const;

	TArray<FHazardConfigReferenceRowPtr> Rows;
	TArray<FHazardConfigReferenceTreeItemPtr> FilteredTreeItems;
	TSharedPtr<STreeView<FHazardConfigReferenceTreeItemPtr>> TreeView;
	TSharedPtr<SSearchBox> SearchBox;
	int32 FilteredReferenceCount = 0;
	FText SearchText;
	bool bShowInvalidOnly = false;
	bool bShowEmpty = true;
	bool bShowUnsupported = true;
	bool bProjectSettingsOnly = false;
	bool bIncludePluginSettings = true;
	bool bIncludeEngineSettings = false;
	int32 LastFixedCount = 0;
	int32 LastFailedCount = 0;
};
}
