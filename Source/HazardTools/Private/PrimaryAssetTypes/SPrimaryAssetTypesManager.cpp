// Copyright Neyl Sullivan 2026

#include "PrimaryAssetTypes/SPrimaryAssetTypesManager.h"

#include "DetailsViewArgs.h"
#include "Engine/AssetManagerSettings.h"
#include "Engine/DataAsset.h"
#include "Framework/Notifications/NotificationManager.h"
#include "ISettingsModule.h"
#include "IStructureDetailsView.h"
#include "PropertyEditorModule.h"
#include "SlateOptMacros.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/SListView.h"

namespace HazardTools
{
const FName SPrimaryAssetTypesManager::Column_ID_Index = "Index";
const FName SPrimaryAssetTypesManager::Column_ID_PrimaryAssetType = "PrimaryAssetType";
const FName SPrimaryAssetTypesManager::Column_ID_AssetBaseClass = "AssetBaseClass";
const FName SPrimaryAssetTypesManager::Column_ID_HasBlueprintClasses = "HasBlueprintClasses";
const FName SPrimaryAssetTypesManager::Column_ID_IsEditorOnly = "IsEditorOnly";
const FName SPrimaryAssetTypesManager::Column_ID_DirectoriesCount = "DirectoriesCount";
const FName SPrimaryAssetTypesManager::Column_ID_SpecificAssetsCount = "SpecificAssetsCount";
const FName SPrimaryAssetTypesManager::Column_ID_Rules = "Rules";
const FName SPrimaryAssetTypesManager::Column_ID_Validation = "Validation";

// Compact table row used by the manager list. The row mirrors WorkingTypes entries and never owns config data.
class SPrimaryAssetTypeTableRow : public SMultiColumnTableRow<FHazardPrimaryAssetTypeRowPtr>
{
public:
	SLATE_BEGIN_ARGS(SPrimaryAssetTypeTableRow)
		{
		}
		SLATE_ARGUMENT(FHazardPrimaryAssetTypeRowPtr, Row)
		SLATE_ATTRIBUTE(FText, HighlightText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
	{
		Row = InArgs._Row;
		HighlightText = InArgs._HighlightText;
		SMultiColumnTableRow<FHazardPrimaryAssetTypeRowPtr>::Construct(FSuperRowType::FArguments(), OwnerTable);
		SetToolTipText(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SPrimaryAssetTypeTableRow::GetRowToolTipText)));
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (!Row.IsValid())
		{
			return SNullWidget::NullWidget;
		}

		const FPrimaryAssetTypeInfo& TypeInfo = Row->TypeInfo;
		FText Text;

		if (ColumnName == SPrimaryAssetTypesManager::Column_ID_Index)
		{
			Text = FText::AsNumber(Row->Index);
		}
		else if (ColumnName == SPrimaryAssetTypesManager::Column_ID_PrimaryAssetType)
		{
			Text = FText::FromName(TypeInfo.PrimaryAssetType);
		}
		else if (ColumnName == SPrimaryAssetTypesManager::Column_ID_AssetBaseClass)
		{
			Text = FText::FromString(SPrimaryAssetTypesManager::GetClassDisplayName(TypeInfo));
		}
		else if (ColumnName == SPrimaryAssetTypesManager::Column_ID_HasBlueprintClasses)
		{
			Text = TypeInfo.bHasBlueprintClasses ? INVTEXT("True") : INVTEXT("False");
		}
		else if (ColumnName == SPrimaryAssetTypesManager::Column_ID_IsEditorOnly)
		{
			Text = TypeInfo.bIsEditorOnly ? INVTEXT("True") : INVTEXT("False");
		}
		else if (ColumnName == SPrimaryAssetTypesManager::Column_ID_DirectoriesCount)
		{
			Text = FText::AsNumber(TypeInfo.GetDirectories().Num());
		}
		else if (ColumnName == SPrimaryAssetTypesManager::Column_ID_SpecificAssetsCount)
		{
			Text = FText::AsNumber(TypeInfo.GetSpecificAssets().Num());
		}
		else if (ColumnName == SPrimaryAssetTypesManager::Column_ID_Rules)
		{
			Text = FText::FromString(SPrimaryAssetTypesManager::GetRulesSummary(TypeInfo.Rules));
		}
		else if (ColumnName == SPrimaryAssetTypesManager::Column_ID_Validation)
		{
			Text = Row->ValidationText.IsEmpty() ? INVTEXT("OK") : Row->ValidationText;
		}

		const bool bCenterAlignedColumn =
			ColumnName == SPrimaryAssetTypesManager::Column_ID_HasBlueprintClasses
			|| ColumnName == SPrimaryAssetTypesManager::Column_ID_IsEditorOnly
			|| ColumnName == SPrimaryAssetTypesManager::Column_ID_DirectoriesCount
			|| ColumnName == SPrimaryAssetTypesManager::Column_ID_SpecificAssetsCount
			|| ColumnName == SPrimaryAssetTypesManager::Column_ID_Validation;

		const TSharedRef<STextBlock> TextBlock =
			SNew(STextBlock)
			.Text(Text)
			.HighlightText(HighlightText)
			.Justification(bCenterAlignedColumn ? ETextJustify::Center : ETextJustify::Left)
			.ColorAndOpacity(ColumnName == SPrimaryAssetTypesManager::Column_ID_Validation ? SPrimaryAssetTypesManager::GetSeverityColor(Row->ValidationSeverity) : FSlateColor::UseForeground());

		if (bCenterAlignedColumn)
		{
			return SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					TextBlock
				];
		}

		return TextBlock;
	}

private:
	// Keep row hover details richer than the visible columns without widening the table.
	FText GetRowToolTipText() const
	{
		if (!Row.IsValid())
		{
			return FText::GetEmpty();
		}

		const FPrimaryAssetTypeInfo& TypeInfo = Row->TypeInfo;
		TArray<FString> Lines;
		Lines.Add(FString::Printf(TEXT("Index: %d"), Row->Index));
		Lines.Add(FString::Printf(TEXT("Primary Asset Type: %s"), *TypeInfo.PrimaryAssetType.ToString()));
		Lines.Add(FString::Printf(TEXT("Asset Base Class: %s"), *SPrimaryAssetTypesManager::GetClassDisplayName(TypeInfo)));
		Lines.Add(FString::Printf(TEXT("Has Blueprint Classes: %s"), TypeInfo.bHasBlueprintClasses ? TEXT("True") : TEXT("False")));
		Lines.Add(FString::Printf(TEXT("Editor Only: %s"), TypeInfo.bIsEditorOnly ? TEXT("True") : TEXT("False")));
		Lines.Add(FString::Printf(TEXT("Directories: %d"), TypeInfo.GetDirectories().Num()));
		AppendDirectoryLines(TypeInfo, Lines);
		Lines.Add(FString::Printf(TEXT("Specific Assets: %d"), TypeInfo.GetSpecificAssets().Num()));
		AppendSpecificAssetLines(TypeInfo, Lines);
		Lines.Add(FString::Printf(TEXT("Rules: %s"), *SPrimaryAssetTypesManager::GetRulesSummary(TypeInfo.Rules)));
		Lines.Add(FString::Printf(TEXT("Validation: %s"), *Row->ValidationText.ToString()));

		return FText::FromString(FString::Join(Lines, TEXT("\n")));
	}

	static void AppendDirectoryLines(const FPrimaryAssetTypeInfo& TypeInfo, TArray<FString>& Lines)
	{
		for (const FDirectoryPath& Directory : TypeInfo.GetDirectories())
		{
			Lines.Add(FString::Printf(TEXT("  - %s"), *Directory.Path));
		}
	}

	static void AppendSpecificAssetLines(const FPrimaryAssetTypeInfo& TypeInfo, TArray<FString>& Lines)
	{
		for (const FSoftObjectPath& AssetPath : TypeInfo.GetSpecificAssets())
		{
			Lines.Add(FString::Printf(TEXT("  - %s"), *AssetPath.ToString()));
		}
	}

	FHazardPrimaryAssetTypeRowPtr Row;
	TAttribute<FText> HighlightText;
};

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SPrimaryAssetTypesManager::Construct(const FArguments& InArgs)
{
	LoadFromSettings();

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 8.0f, 8.0f, 4.0f)
		[
			MakeToolbar()
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)

			+ SSplitter::Slot()
			.Value(0.68f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.Padding(FMargin(3.0f))
				[
					MakeTable()
				]
			]

			+ SSplitter::Slot()
			.Value(0.32f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.Padding(FMargin(8.0f))
				[
					SAssignNew(DetailsBox, SBox)
					[
						MakeDetails()
					]
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Header"))
			.Padding(FMargin(14.0f, 8.0f))
			[
				SNew(STextBlock)
				.Text(this, &ThisClass::GetStatusText)
				.ColorAndOpacity(this, &ThisClass::GetStatusColor)
			]
		]
	];
}

TSharedRef<SWidget> SPrimaryAssetTypesManager::MakeToolbar()
{
	// Use built-in AppStyle icons so the tool visually matches other Unreal editor panels.
	auto MakeToolbarButton = [](const FText& Label, const FName IconName, const FText& ToolTip, const FOnClicked& OnClicked, const TAttribute<bool>& IsEnabled = TAttribute<bool>(true))
	{
		return SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ContentPadding(FMargin(6.0f, 3.0f))
			.ToolTipText(ToolTip)
			.IsEnabled(IsEnabled)
			.OnClicked(OnClicked)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush(IconName))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(Label)
				]
			];
	};

	const TAttribute<bool> HasSelectedRow = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &ThisClass::CanEditSelectedRow));

	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			MakeToolbarButton(INVTEXT("Add"), "Icons.PlusCircle", INVTEXT("Add a new Primary Asset Type entry"), FOnClicked::CreateSP(this, &ThisClass::AddEntry))
		]

		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			MakeToolbarButton(INVTEXT("Duplicate"), "Icons.Duplicate", INVTEXT("Duplicate the selected Primary Asset Type entry"), FOnClicked::CreateSP(this, &ThisClass::DuplicateEntry), HasSelectedRow)
		]

		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			MakeToolbarButton(INVTEXT("Delete"), "Icons.Delete", INVTEXT("Delete the selected Primary Asset Type entry"), FOnClicked::CreateSP(this, &ThisClass::DeleteEntry), HasSelectedRow)
		]

		+ SHorizontalBox::Slot().AutoWidth().Padding(10.0f, 0.0f, 4.0f, 0.0f)
		[
			MakeToolbarButton(INVTEXT("Move Up"), "Icons.ArrowUp", INVTEXT("Move the selected entry up"), FOnClicked::CreateSP(this, &ThisClass::MoveSelectedEntry, -1), HasSelectedRow)
		]

		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			MakeToolbarButton(INVTEXT("Move Down"), "Icons.ArrowDown", INVTEXT("Move the selected entry down"), FOnClicked::CreateSP(this, &ThisClass::MoveSelectedEntry, 1), HasSelectedRow)
		]

		+ SHorizontalBox::Slot().AutoWidth().Padding(10.0f, 0.0f, 4.0f, 0.0f)
		[
			MakeToolbarButton(INVTEXT("Validate"), "Icons.Check", INVTEXT("Validate all Primary Asset Type entries"), FOnClicked::CreateSP(this, &ThisClass::ValidateAndNotify))
		]

		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			MakeToolbarButton(INVTEXT("Save"), "Icons.Save", INVTEXT("Save changes to Asset Manager settings"), FOnClicked::CreateSP(this, &ThisClass::SaveChanges))
		]

		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
		[
			MakeToolbarButton(INVTEXT("Revert"), "Icons.Refresh", INVTEXT("Reload values from Asset Manager settings"), FOnClicked::CreateSP(this, &ThisClass::RevertChanges))
		]

		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f, 8.0f, 0.0f)
		[
			SAssignNew(SearchBox, SSearchBox)
			.HintText(INVTEXT("Search Primary Asset Types..."))
			.OnTextChanged(this, &ThisClass::HandleSearchTextChanged)
		]

		+ SHorizontalBox::Slot().AutoWidth()
		[
			MakeToolbarButton(INVTEXT("Settings"), "Icons.Settings", INVTEXT("Open the original Asset Manager project settings page"), FOnClicked::CreateSP(this, &ThisClass::OpenAssetManagerSettings))
		];
}

TSharedRef<SWidget> SPrimaryAssetTypesManager::MakeTable()
{
	const TSharedPtr<SHeaderRow> HeaderRow =
		SNew(SHeaderRow)
		+ SHeaderRow::Column(Column_ID_Index).DefaultLabel(INVTEXT("#")).FixedWidth(42.0f)
		+ SHeaderRow::Column(Column_ID_PrimaryAssetType).DefaultLabel(INVTEXT("Primary Asset Type")).FillWidth(0.18f)
		+ SHeaderRow::Column(Column_ID_AssetBaseClass).DefaultLabel(INVTEXT("Asset Base Class")).FillWidth(0.20f)
		+ SHeaderRow::Column(Column_ID_HasBlueprintClasses).DefaultLabel(INVTEXT("BP Classes")).FixedWidth(82.0f)
		+ SHeaderRow::Column(Column_ID_IsEditorOnly).DefaultLabel(INVTEXT("Editor Only")).FixedWidth(82.0f)
		+ SHeaderRow::Column(Column_ID_DirectoriesCount).DefaultLabel(INVTEXT("Dirs")).FixedWidth(55.0f)
		+ SHeaderRow::Column(Column_ID_SpecificAssetsCount).DefaultLabel(INVTEXT("Assets")).FixedWidth(62.0f)
		+ SHeaderRow::Column(Column_ID_Rules).DefaultLabel(INVTEXT("Cook Rule / Chunk")).ManualWidth(260.0f)
		+ SHeaderRow::Column(Column_ID_Validation).DefaultLabel(INVTEXT("Validation")).FillWidth(0.15f);

	SAssignNew(ListView, SListView<FHazardPrimaryAssetTypeRowPtr>)
		.ListItemsSource(&FilteredRows)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &ThisClass::HandleGenerateRow)
		.OnSelectionChanged_Lambda([this](FHazardPrimaryAssetTypeRowPtr, ESelectInfo::Type)
		{
			RefreshDetails();
		})
		.HeaderRow(HeaderRow);

	return ListView.ToSharedRef();
}

TSharedRef<SWidget> SPrimaryAssetTypesManager::MakeDetails()
{
	if (!CanEditSelectedRow())
	{
		StructureDetailsView.Reset();
		SelectedTypeStructData.Reset();
		return SNew(STextBlock)
			.Text(INVTEXT("Select a Primary Asset type to edit its details."));
	}

	FPrimaryAssetTypeInfo* TypeInfo = GetSelectedTypeInfo();
	if (!TypeInfo)
	{
		return SNullWidget::NullWidget;
	}

	// Bind the standard details view directly to the selected WorkingTypes entry.
	// This keeps Unreal's native USTRUCT customizations for paths, soft classes, and nested rules.
	SelectedTypeStructData = MakeShared<FStructOnScope>(FPrimaryAssetTypeInfo::StaticStruct(), reinterpret_cast<uint8*>(TypeInfo));

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.bShowObjectLabel = false;
	DetailsViewArgs.bAllowFavoriteSystem = false;
	DetailsViewArgs.ColumnWidth = 0.45f;

	FStructureDetailsViewArgs StructureViewArgs;
	StructureViewArgs.bShowObjects = false;
	StructureViewArgs.bShowAssets = true;
	StructureViewArgs.bShowClasses = true;
	StructureViewArgs.bShowInterfaces = false;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	StructureDetailsView = PropertyEditorModule.CreateStructureDetailView(DetailsViewArgs, StructureViewArgs, SelectedTypeStructData, INVTEXT("Primary Asset Type"));
	StructureDetailsView->GetOnFinishedChangingPropertiesDelegate().AddSP(this, &ThisClass::HandleFinishedChangingProperties);

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			StructureDetailsView->GetWidget().ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.ColorAndOpacity_Lambda([this]()
			{
				const FHazardPrimaryAssetTypeRowPtr Row = GetSelectedRow();
				return Row.IsValid() ? GetSeverityColor(Row->ValidationSeverity) : FSlateColor::UseForeground();
			})
			.Text(this, &ThisClass::GetSelectedValidationText)
		];
}

TSharedRef<ITableRow> SPrimaryAssetTypesManager::HandleGenerateRow(const FHazardPrimaryAssetTypeRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SPrimaryAssetTypeTableRow, OwnerTable)
		.Row(Row)
		.HighlightText(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &ThisClass::GetSearchHighlightText)));
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SPrimaryAssetTypesManager::LoadFromSettings()
{
	const UAssetManagerSettings* Settings = GetDefault<UAssetManagerSettings>();
	WorkingTypes = Settings->PrimaryAssetTypesToScan;
	bHasUnsavedChanges = false;
	RefreshRows();
}

void SPrimaryAssetTypesManager::RefreshRows()
{
	Rows.Reset();
	for (int32 Index = 0; Index < WorkingTypes.Num(); ++Index)
	{
		FHazardPrimaryAssetTypeRowPtr Row = MakeShared<FHazardPrimaryAssetTypeRow>();
		Row->Index = Index;
		Row->TypeInfo = WorkingTypes[Index];
		Rows.Add(Row);
	}
	ValidateRows(false);
	ApplyFilter();
}

void SPrimaryAssetTypesManager::RefreshTable()
{
	RefreshRows();
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
	if (DetailsBox.IsValid())
	{
		DetailsBox->SetContent(MakeDetails());
	}
}

void SPrimaryAssetTypesManager::RefreshDetails()
{
	if (DetailsBox.IsValid())
	{
		DetailsBox->SetContent(MakeDetails());
	}
}

void SPrimaryAssetTypesManager::SelectRowByIndex(const int32 Index) const
{
	if (ListView.IsValid() && Rows.IsValidIndex(Index))
	{
		for (const FHazardPrimaryAssetTypeRowPtr& Row : FilteredRows)
		{
			if (Row.IsValid() && Row->Index == Index)
			{
				ListView->SetSelection(Row);
				ListView->RequestScrollIntoView(Row);
				break;
			}
		}
	}
}

void SPrimaryAssetTypesManager::MarkDirty()
{
	bHasUnsavedChanges = true;
	RefreshTable();
}

void SPrimaryAssetTypesManager::HandleFinishedChangingProperties(const FPropertyChangedEvent&)
{
	const FHazardPrimaryAssetTypeRowPtr SelectedRow = GetSelectedRow();
	if (!SelectedRow.IsValid() || !WorkingTypes.IsValidIndex(SelectedRow->Index))
	{
		return;
	}

	// The details view edits WorkingTypes directly; copy the changed value back into the visible row mirror.
	SelectedRow->TypeInfo = WorkingTypes[SelectedRow->Index];
	bHasUnsavedChanges = true;
	ValidateRows(false);
	ApplyFilter();

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

void SPrimaryAssetTypesManager::HandleSearchTextChanged(const FText& InSearchText)
{
	SearchText = InSearchText;
	ApplyFilter();

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

FReply SPrimaryAssetTypesManager::AddEntry()
{
	FPrimaryAssetTypeInfo NewType;
	NewType.PrimaryAssetType = MakeUniquePrimaryAssetTypeName(TEXT("NewPrimaryAssetType"));
	NewType.SetAssetBaseClass(UPrimaryDataAsset::StaticClass());
	NewType.AssetBaseClassLoaded = UPrimaryDataAsset::StaticClass();
	NewType.bHasBlueprintClasses = false;
	NewType.bIsEditorOnly = false;
	WorkingTypes.Add(NewType);
	MarkDirty();
	SelectRowByIndex(WorkingTypes.Num() - 1);
	return FReply::Handled();
}

FReply SPrimaryAssetTypesManager::DuplicateEntry()
{
	const FHazardPrimaryAssetTypeRowPtr SelectedRow = GetSelectedRow();
	if (!SelectedRow.IsValid())
	{
		return FReply::Unhandled();
	}

	FPrimaryAssetTypeInfo DuplicatedType = SelectedRow->TypeInfo;
	DuplicatedType.PrimaryAssetType = MakeUniquePrimaryAssetTypeName(FName(*(DuplicatedType.PrimaryAssetType.ToString() + TEXT("_Copy"))));
	const int32 InsertIndex = SelectedRow->Index + 1;
	WorkingTypes.Insert(DuplicatedType, InsertIndex);
	MarkDirty();
	SelectRowByIndex(InsertIndex);
	return FReply::Handled();
}

FReply SPrimaryAssetTypesManager::DeleteEntry()
{
	const FHazardPrimaryAssetTypeRowPtr SelectedRow = GetSelectedRow();
	if (!SelectedRow.IsValid())
	{
		return FReply::Unhandled();
	}

	const EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(INVTEXT("Delete Primary Asset type '{0}'?"), FText::FromName(SelectedRow->TypeInfo.PrimaryAssetType)));
	if (Result != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	const int32 DeletedIndex = SelectedRow->Index;
	WorkingTypes.RemoveAt(DeletedIndex);
	MarkDirty();
	SelectRowByIndex(FMath::Clamp(DeletedIndex, 0, WorkingTypes.Num() - 1));
	return FReply::Handled();
}

FReply SPrimaryAssetTypesManager::MoveSelectedEntry(const int32 Direction)
{
	const FHazardPrimaryAssetTypeRowPtr SelectedRow = GetSelectedRow();
	if (!SelectedRow.IsValid())
	{
		return FReply::Unhandled();
	}

	const int32 SourceIndex = SelectedRow->Index;
	const int32 TargetIndex = SourceIndex + Direction;
	if (!WorkingTypes.IsValidIndex(TargetIndex))
	{
		return FReply::Handled();
	}

	WorkingTypes.Swap(SourceIndex, TargetIndex);
	MarkDirty();
	SelectRowByIndex(TargetIndex);
	return FReply::Handled();
}

FReply SPrimaryAssetTypesManager::ValidateAndNotify()
{
	ValidateRows(true);
	RefreshTable();
	return FReply::Handled();
}

FReply SPrimaryAssetTypesManager::SaveChanges()
{
	ValidateRows(false);

	UAssetManagerSettings* Settings = GetMutableDefault<UAssetManagerSettings>();
	Settings->Modify();
	Settings->PrimaryAssetTypesToScan = WorkingTypes;
	Settings->SaveConfig();
	Settings->TryUpdateDefaultConfigFile();
	bHasUnsavedChanges = false;

	FNotificationInfo Info(INVTEXT("Primary Asset Types saved to Asset Manager settings."));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
	RefreshTable();
	return FReply::Handled();
}

FReply SPrimaryAssetTypesManager::RevertChanges()
{
	LoadFromSettings();
	RefreshTable();
	return FReply::Handled();
}

FReply SPrimaryAssetTypesManager::OpenAssetManagerSettings() const
{
	ISettingsModule& SettingsModule = FModuleManager::LoadModuleChecked<ISettingsModule>("Settings");
	SettingsModule.ShowViewer("Project", NAME_Game, UAssetManagerSettings::StaticClass()->GetFName());
	return FReply::Handled();
}

bool SPrimaryAssetTypesManager::CanEditSelectedRow() const
{
	return GetSelectedRow().IsValid();
}

FHazardPrimaryAssetTypeRowPtr SPrimaryAssetTypesManager::GetSelectedRow() const
{
	if (!ListView.IsValid())
	{
		return nullptr;
	}

	const TArray<FHazardPrimaryAssetTypeRowPtr> SelectedRows = ListView->GetSelectedItems();
	return SelectedRows.Num() > 0 ? SelectedRows[0] : nullptr;
}

FPrimaryAssetTypeInfo* SPrimaryAssetTypesManager::GetSelectedTypeInfo()
{
	const FHazardPrimaryAssetTypeRowPtr SelectedRow = GetSelectedRow();
	return SelectedRow.IsValid() && WorkingTypes.IsValidIndex(SelectedRow->Index) ? &WorkingTypes[SelectedRow->Index] : nullptr;
}

const FPrimaryAssetTypeInfo* SPrimaryAssetTypesManager::GetSelectedTypeInfo() const
{
	const FHazardPrimaryAssetTypeRowPtr SelectedRow = GetSelectedRow();
	return SelectedRow.IsValid() && WorkingTypes.IsValidIndex(SelectedRow->Index) ? &WorkingTypes[SelectedRow->Index] : nullptr;
}

FName SPrimaryAssetTypesManager::MakeUniquePrimaryAssetTypeName(const FName& BaseName, const int32 IgnoredIndex) const
{
	FString Candidate = BaseName.IsNone() ? TEXT("NewPrimaryAssetType") : BaseName.ToString();
	int32 Suffix = 0;

	while (true)
	{
		const FName CandidateName(*Candidate);
		bool bAlreadyUsed = false;
		for (int32 Index = 0; Index < WorkingTypes.Num(); ++Index)
		{
			if (Index != IgnoredIndex && WorkingTypes[Index].PrimaryAssetType == CandidateName)
			{
				bAlreadyUsed = true;
				break;
			}
		}

		if (!bAlreadyUsed)
		{
			return CandidateName;
		}

		Candidate = FString::Printf(TEXT("%s_%d"), *BaseName.ToString(), ++Suffix);
	}
}

void SPrimaryAssetTypesManager::ValidateRows(const bool bShowDialog)
{
	TMap<FName, int32> TypeCounts;
	TMap<FString, TArray<FName>> DirectoryUsers;

	for (const FPrimaryAssetTypeInfo& TypeInfo : WorkingTypes)
	{
		if (!TypeInfo.PrimaryAssetType.IsNone())
		{
			TypeCounts.FindOrAdd(TypeInfo.PrimaryAssetType)++;
		}

		for (const FDirectoryPath& Directory : TypeInfo.GetDirectories())
		{
			if (!Directory.Path.IsEmpty())
			{
				DirectoryUsers.FindOrAdd(Directory.Path).Add(TypeInfo.PrimaryAssetType);
			}
		}
	}

	LastErrorCount = 0;
	LastWarningCount = 0;
	for (FHazardPrimaryAssetTypeRowPtr& Row : Rows)
	{
		if (Row.IsValid())
		{
			ValidateRow(*Row, TypeCounts, DirectoryUsers);
			if (Row->ValidationSeverity == EHazardPrimaryAssetTypeValidationSeverity::Error)
			{
				++LastErrorCount;
			}
			else if (Row->ValidationSeverity == EHazardPrimaryAssetTypeValidationSeverity::Warning)
			{
				++LastWarningCount;
			}
		}
	}

	if (bShowDialog)
	{
		const FText Message = LastErrorCount == 0 && LastWarningCount == 0
			? INVTEXT("Validation completed. No obvious problems found.")
			: FText::Format(INVTEXT("Validation completed with {0} error(s) and {1} warning(s). See the Validation column for details."), FText::AsNumber(LastErrorCount), FText::AsNumber(LastWarningCount));
		FMessageDialog::Open(EAppMsgType::Ok, Message);
	}
}

void SPrimaryAssetTypesManager::ApplyFilter()
{
	FilteredRows.Reset();
	for (const FHazardPrimaryAssetTypeRowPtr& Row : Rows)
	{
		if (DoesRowMatchFilter(Row))
		{
			FilteredRows.Add(Row);
		}
	}
}

void SPrimaryAssetTypesManager::ValidateRow(FHazardPrimaryAssetTypeRow& Row, const TMap<FName, int32>& TypeCounts, const TMap<FString, TArray<FName>>& DirectoryUsers) const
{
	TArray<FText> Errors;
	TArray<FText> Warnings;
	const FPrimaryAssetTypeInfo& TypeInfo = Row.TypeInfo;

	if (TypeInfo.PrimaryAssetType.IsNone())
	{
		Errors.Add(INVTEXT("Empty PrimaryAssetType"));
	}
	else if (const int32* Count = TypeCounts.Find(TypeInfo.PrimaryAssetType); Count && *Count > 1)
	{
		Errors.Add(INVTEXT("Duplicate PrimaryAssetType"));
	}

	const TSoftClassPtr<UObject>& AssetBaseClass = TypeInfo.GetAssetBaseClass();
	const UClass* LoadedClass = AssetBaseClass.Get();
	if (AssetBaseClass.IsNull() && LoadedClass == nullptr)
	{
		Errors.Add(INVTEXT("Missing AssetBaseClass"));
	}

	const FString AssetBaseClassPath = AssetBaseClass.ToSoftObjectPath().ToString();
	const bool bIsGameFeatureDataType =
		TypeInfo.PrimaryAssetType == TEXT("GameFeatureData")
		|| (LoadedClass && LoadedClass->GetName() == TEXT("GameFeatureData"))
		|| AssetBaseClassPath.Contains(TEXT("GameFeatureData"));

	// GameFeatureData is discovered by the Game Features system and commonly has no explicit scan paths here.
	if (!bIsGameFeatureDataType && TypeInfo.GetDirectories().IsEmpty() && TypeInfo.GetSpecificAssets().IsEmpty())
	{
		Errors.Add(INVTEXT("No directories or specific assets"));
	}

	for (const FDirectoryPath& Directory : TypeInfo.GetDirectories())
	{
		if (Directory.Path.IsEmpty() || Directory.Path.Contains(TEXT("\\")) || Directory.Path.EndsWith(TEXT("/")) || !FPackageName::IsValidLongPackageName(Directory.Path))
		{
			Errors.Add(FText::Format(INVTEXT("Invalid directory path '{0}'"), FText::FromString(Directory.Path)));
		}

		if (const TArray<FName>* Users = DirectoryUsers.Find(Directory.Path); Users && Users->Num() > 1)
		{
			Warnings.Add(FText::Format(INVTEXT("Directory also used by other asset types: {0}"), FText::FromString(Directory.Path)));
		}
	}

	for (const FSoftObjectPath& AssetPath : TypeInfo.GetSpecificAssets())
	{
		const FString PathString = AssetPath.ToString();
		if (PathString.IsEmpty() || !FPackageName::IsValidObjectPath(PathString))
		{
			Errors.Add(FText::Format(INVTEXT("Invalid specific asset path '{0}'"), FText::FromString(PathString)));
		}
	}

	if (TypeInfo.bHasBlueprintClasses && LoadedClass && LoadedClass->HasMetaDataHierarchical(TEXT("IsBlueprintBase")) && !LoadedClass->GetBoolMetaDataHierarchical(TEXT("IsBlueprintBase")))
	{
		Warnings.Add(INVTEXT("Blueprint class scanning is enabled, but the base class does not look blueprint-friendly"));
	}

	if (TypeInfo.bIsEditorOnly && (TypeInfo.Rules.CookRule == EPrimaryAssetCookRule::AlwaysCook || TypeInfo.Rules.CookRule == EPrimaryAssetCookRule::DevelopmentAlwaysProductionUnknownCook || TypeInfo.Rules.CookRule == EPrimaryAssetCookRule::DevelopmentAlwaysProductionNeverCook))
	{
		Warnings.Add(INVTEXT("Editor-only type has cook rules that imply cooking"));
	}

	if (!Errors.IsEmpty())
	{
		Row.ValidationSeverity = EHazardPrimaryAssetTypeValidationSeverity::Error;
		Row.ValidationText = FText::Join(INVTEXT("; "), Errors);
	}
	else if (!Warnings.IsEmpty())
	{
		Row.ValidationSeverity = EHazardPrimaryAssetTypeValidationSeverity::Warning;
		Row.ValidationText = FText::Join(INVTEXT("; "), Warnings);
	}
	else
	{
		Row.ValidationSeverity = EHazardPrimaryAssetTypeValidationSeverity::None;
		Row.ValidationText = INVTEXT("OK");
	}
}

bool SPrimaryAssetTypesManager::DoesRowMatchFilter(const FHazardPrimaryAssetTypeRowPtr& Row) const
{
	if (!Row.IsValid())
	{
		return false;
	}

	const FString FilterString = SearchText.ToString().TrimStartAndEnd();
	if (FilterString.IsEmpty())
	{
		return true;
	}

	const FPrimaryAssetTypeInfo& TypeInfo = Row->TypeInfo;
	TArray<FString> SearchStrings;
	// Search uses the same high-signal fields that are visible in the table or row tooltip.
	SearchStrings.Add(TypeInfo.PrimaryAssetType.ToString());
	SearchStrings.Add(GetClassDisplayName(TypeInfo));
	SearchStrings.Add(GetRulesSummary(TypeInfo.Rules));
	SearchStrings.Add(Row->ValidationText.ToString());

	for (const FDirectoryPath& Directory : TypeInfo.GetDirectories())
	{
		SearchStrings.Add(Directory.Path);
	}

	for (const FSoftObjectPath& AssetPath : TypeInfo.GetSpecificAssets())
	{
		SearchStrings.Add(AssetPath.ToString());
	}

	for (const FString& SearchString : SearchStrings)
	{
		if (SearchString.Contains(FilterString, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

FText SPrimaryAssetTypesManager::GetSearchHighlightText() const
{
	return SearchText;
}

FText SPrimaryAssetTypesManager::GetSelectedValidationText() const
{
	const FHazardPrimaryAssetTypeRowPtr Row = GetSelectedRow();
	return Row.IsValid() ? Row->ValidationText : FText::GetEmpty();
}

FText SPrimaryAssetTypesManager::GetStatusText() const
{
	const FText DirtyText = bHasUnsavedChanges ? INVTEXT("Unsaved changes") : INVTEXT("No unsaved changes");
	if (!SearchText.IsEmptyOrWhitespace())
	{
		return FText::Format(INVTEXT("{0} of {1} Primary Asset type(s) shown. {2}. Validation: {3} error(s), {4} warning(s)."), FText::AsNumber(FilteredRows.Num()), FText::AsNumber(WorkingTypes.Num()), DirtyText, FText::AsNumber(LastErrorCount), FText::AsNumber(LastWarningCount));
	}

	return FText::Format(INVTEXT("{0} Primary Asset type(s). {1}. Validation: {2} error(s), {3} warning(s)."), FText::AsNumber(WorkingTypes.Num()), DirtyText, FText::AsNumber(LastErrorCount), FText::AsNumber(LastWarningCount));
}

FSlateColor SPrimaryAssetTypesManager::GetStatusColor() const
{
	if (LastErrorCount > 0)
	{
		return FAppStyle::Get().GetSlateColor("Colors.AccentRed");
	}
	if (LastWarningCount > 0)
	{
		return FAppStyle::Get().GetSlateColor("Colors.AccentYellow");
	}
	return FSlateColor::UseForeground();
}

FText SPrimaryAssetTypesManager::GetCookRuleText(const EPrimaryAssetCookRule CookRule)
{
	switch (CookRule)
	{
	case EPrimaryAssetCookRule::NeverCook:
		return INVTEXT("NeverCook");
	case EPrimaryAssetCookRule::ProductionNeverCook:
		return INVTEXT("ProductionNeverCook");
	case EPrimaryAssetCookRule::DevelopmentAlwaysProductionNeverCook:
		return INVTEXT("DevelopmentAlwaysProductionNeverCook");
	case EPrimaryAssetCookRule::DevelopmentAlwaysProductionUnknownCook:
		return INVTEXT("DevelopmentAlwaysProductionUnknownCook");
	case EPrimaryAssetCookRule::AlwaysCook:
		return INVTEXT("AlwaysCook");
	case EPrimaryAssetCookRule::Unknown:
	default:
		return INVTEXT("Unknown");
	}
}

FString SPrimaryAssetTypesManager::GetRulesSummary(const FPrimaryAssetRules& Rules)
{
	return FString::Printf(TEXT("%s, Chunk %d, Priority %d%s"), *GetCookRuleText(Rules.CookRule).ToString(), Rules.ChunkId, Rules.Priority, Rules.bApplyRecursively ? TEXT(", Recursive") : TEXT(""));
}

FString SPrimaryAssetTypesManager::GetClassDisplayName(const FPrimaryAssetTypeInfo& TypeInfo)
{
	const TSoftClassPtr<UObject>& AssetBaseClass = TypeInfo.GetAssetBaseClass();
	if (const UClass* LoadedClass = AssetBaseClass.Get())
	{
		return LoadedClass->GetName();
	}

	const FString Path = AssetBaseClass.ToSoftObjectPath().ToString();
	return Path.IsEmpty() ? TEXT("None") : Path;
}

FSlateColor SPrimaryAssetTypesManager::GetSeverityColor(const EHazardPrimaryAssetTypeValidationSeverity Severity)
{
	switch (Severity)
	{
	case EHazardPrimaryAssetTypeValidationSeverity::Error:
		return FAppStyle::Get().GetSlateColor("Colors.AccentRed");
	case EHazardPrimaryAssetTypeValidationSeverity::Warning:
		return FAppStyle::Get().GetSlateColor("Colors.AccentYellow");
	case EHazardPrimaryAssetTypeValidationSeverity::Info:
		return FAppStyle::Get().GetSlateColor("Colors.AccentBlue");
	case EHazardPrimaryAssetTypeValidationSeverity::None:
	default:
		return FAppStyle::Get().GetSlateColor("Colors.AccentGreen");
	}
}
}
