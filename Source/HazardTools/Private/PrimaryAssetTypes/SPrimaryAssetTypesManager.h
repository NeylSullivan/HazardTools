// Copyright Neyl Sullivan 2026

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "Widgets/SCompoundWidget.h"

class FStructOnScope;
class IStructureDetailsView;
struct FPropertyChangedEvent;

enum class EHazardPrimaryAssetTypeValidationSeverity : uint8
{
	None,
	Info,
	Warning,
	Error
};

struct FHazardPrimaryAssetTypeRow
{
	int32 Index = INDEX_NONE;
	FPrimaryAssetTypeInfo TypeInfo;
	EHazardPrimaryAssetTypeValidationSeverity ValidationSeverity = EHazardPrimaryAssetTypeValidationSeverity::None;
	FText ValidationText;
};

using FHazardPrimaryAssetTypeRowPtr = TSharedPtr<FHazardPrimaryAssetTypeRow>;

namespace HazardTools
{
class SPrimaryAssetTypesManager : public SCompoundWidget
{
	using ThisClass = SPrimaryAssetTypesManager;

public:
	SLATE_BEGIN_ARGS(SPrimaryAssetTypesManager)
		{
		}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	static const FName Column_ID_Index;
	static const FName Column_ID_PrimaryAssetType;
	static const FName Column_ID_AssetBaseClass;
	static const FName Column_ID_HasBlueprintClasses;
	static const FName Column_ID_IsEditorOnly;
	static const FName Column_ID_DirectoriesCount;
	static const FName Column_ID_SpecificAssetsCount;
	static const FName Column_ID_Rules;
	static const FName Column_ID_Validation;

	static FText GetCookRuleText(EPrimaryAssetCookRule CookRule);
	static FString GetRulesSummary(const FPrimaryAssetRules& Rules);
	static FString GetClassDisplayName(const FPrimaryAssetTypeInfo& TypeInfo);
	static FSlateColor GetSeverityColor(EHazardPrimaryAssetTypeValidationSeverity Severity);

private:
	TSharedRef<SWidget> MakeToolbar();
	TSharedRef<SWidget> MakeTable();
	TSharedRef<SWidget> MakeDetails();
	TSharedRef<ITableRow> HandleGenerateRow(FHazardPrimaryAssetTypeRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable);

	void LoadFromSettings();
	void RefreshRows();
	void RefreshTable();
	void RefreshDetails();
	void ApplyFilter();
	void SelectRowByIndex(int32 Index) const;
	void MarkDirty();
	void HandleFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	void HandleSearchTextChanged(const FText& InSearchText);

	FReply AddEntry();
	FReply DuplicateEntry();
	FReply DeleteEntry();
	FReply MoveSelectedEntry(int32 Direction);
	FReply ValidateAndNotify();
	FReply SaveChanges();
	FReply RevertChanges();
	FReply OpenAssetManagerSettings() const;

	bool CanEditSelectedRow() const;
	FHazardPrimaryAssetTypeRowPtr GetSelectedRow() const;
	FPrimaryAssetTypeInfo* GetSelectedTypeInfo();
	const FPrimaryAssetTypeInfo* GetSelectedTypeInfo() const;

	FName MakeUniquePrimaryAssetTypeName(const FName& BaseName, int32 IgnoredIndex = INDEX_NONE) const;
	void ValidateRows(bool bShowDialog);
	void ValidateRow(FHazardPrimaryAssetTypeRow& Row, const TMap<FName, int32>& TypeCounts, const TMap<FString, TArray<FName>>& DirectoryUsers) const;
	bool DoesRowMatchFilter(const FHazardPrimaryAssetTypeRowPtr& Row) const;

	FText GetSearchHighlightText() const;
	FText GetSelectedValidationText() const;

	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;

	TArray<FPrimaryAssetTypeInfo> WorkingTypes;
	TArray<FHazardPrimaryAssetTypeRowPtr> Rows;
	TArray<FHazardPrimaryAssetTypeRowPtr> FilteredRows;
	TSharedPtr<SListView<FHazardPrimaryAssetTypeRowPtr>> ListView;
	TSharedPtr<class SSearchBox> SearchBox;
	TSharedPtr<SBox> DetailsBox;
	TSharedPtr<FStructOnScope> SelectedTypeStructData;
	TSharedPtr<IStructureDetailsView> StructureDetailsView;
	FText SearchText;
	bool bHasUnsavedChanges = false;
	int32 LastErrorCount = 0;
	int32 LastWarningCount = 0;
};
}
