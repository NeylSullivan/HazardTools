// Copyright Neyl Sullivan 2026

#include "ConfigReferenceValidator/SHazardConfigReferenceValidatorWindow.h"

#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformProcess.h"
#include "PropertyCustomizationHelpers.h"
#include "SlateOptMacros.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STreeView.h"

namespace HazardTools
{
const FName SHazardConfigReferenceValidatorWindow::Column_Status = "Status";
const FName SHazardConfigReferenceValidatorWindow::Column_SettingsClass = "SettingsClass";
const FName SHazardConfigReferenceValidatorWindow::Column_ConfigFile = "ConfigFile";
const FName SHazardConfigReferenceValidatorWindow::Column_ConfigSection = "ConfigSection";
const FName SHazardConfigReferenceValidatorWindow::Column_PropertyPath = "PropertyPath";
const FName SHazardConfigReferenceValidatorWindow::Column_ReferenceType = "ReferenceType";
const FName SHazardConfigReferenceValidatorWindow::Column_CurrentPath = "CurrentPath";
const FName SHazardConfigReferenceValidatorWindow::Column_ResolvedObject = "ResolvedObject";
const FName SHazardConfigReferenceValidatorWindow::Column_Replacement = "Replacement";
const FName SHazardConfigReferenceValidatorWindow::Column_Actions = "Actions";

namespace ConfigReferenceValidatorUI
{
static FSlateColor GetStatusColor(const EHazardConfigReferenceStatus Status)
{
	switch (Status)
	{
	case EHazardConfigReferenceStatus::Valid:
		return FAppStyle::Get().GetSlateColor("Colors.AccentGreen");
	case EHazardConfigReferenceStatus::Invalid:
	case EHazardConfigReferenceStatus::SaveFailed:
		return FAppStyle::Get().GetSlateColor("Colors.AccentRed");
	case EHazardConfigReferenceStatus::Redirected:
	case EHazardConfigReferenceStatus::PendingFix:
		return FAppStyle::Get().GetSlateColor("Colors.AccentYellow");
	case EHazardConfigReferenceStatus::Unsupported:
		return FAppStyle::Get().GetSlateColor("Colors.Foreground");
	case EHazardConfigReferenceStatus::Empty:
	default:
		return FAppStyle::Get().GetSlateColor("Colors.Secondary");
	}
}

static TSharedRef<SWidget> MakeToolbarButton(const FText& Label, const FName IconName, const FText& ToolTip, const FOnClicked& OnClicked, const TAttribute<bool>& IsEnabled = TAttribute<bool>(true))
{
	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(FMargin(6.0f, 3.0f))
		.ToolTipText(ToolTip)
		.IsEnabled(IsEnabled)
		.OnClicked(OnClicked)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				SNew(SImage)
				.Image(FAppStyle::Get().GetBrush(IconName))
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(Label)
			]
		];
}

static TSharedRef<SWidget> MakeCheckBox(const FText& Label, const FText& ToolTip, const TAttribute<ECheckBoxState>& State, const FOnCheckStateChanged& OnChanged)
{
	return SNew(SCheckBox)
		.ToolTipText(ToolTip)
		.IsChecked(State)
		.OnCheckStateChanged(OnChanged)
		[
			SNew(STextBlock).Text(Label)
		];
}
}

class SConfigReferenceValidatorRow : public SMultiColumnTableRow<FHazardConfigReferenceTreeItemPtr>
{
public:
	SLATE_BEGIN_ARGS(SConfigReferenceValidatorRow)
		{
		}
		SLATE_ARGUMENT(FHazardConfigReferenceTreeItemPtr, Item)
		SLATE_ARGUMENT(TWeakPtr<SHazardConfigReferenceValidatorWindow>, OwnerWidget)
		SLATE_ATTRIBUTE(FText, HighlightText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
	{
		Item = InArgs._Item;
		Row = Item.IsValid() ? Item->Row : nullptr;
		OwnerWidget = InArgs._OwnerWidget;
		HighlightText = InArgs._HighlightText;
		SMultiColumnTableRow<FHazardConfigReferenceTreeItemPtr>::Construct(FSuperRowType::FArguments(), OwnerTable);
		SetToolTipText(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SConfigReferenceValidatorRow::GetRowToolTipText)));
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (Item.IsValid() && Item->IsGroup())
		{
			return GenerateGroupWidgetForColumn(ColumnName);
		}

		if (!Row.IsValid())
		{
			return SNullWidget::NullWidget;
		}

		if (ColumnName == SHazardConfigReferenceValidatorWindow::Column_Status)
		{
			return SNew(STextBlock)
				.Text(FText::FromString(Row->GetStatusString()))
				.HighlightText(HighlightText)
				.ColorAndOpacity(ConfigReferenceValidatorUI::GetStatusColor(Row->Status));
		}
		if (ColumnName == SHazardConfigReferenceValidatorWindow::Column_SettingsClass)
		{
			return MakeText(Row->GetSettingsClassName());
		}
		if (ColumnName == SHazardConfigReferenceValidatorWindow::Column_ConfigFile)
		{
			return MakeConfigFileWidget();
		}
		if (ColumnName == SHazardConfigReferenceValidatorWindow::Column_ConfigSection)
		{
			return MakeText(Row->ConfigSection);
		}
		if (ColumnName == SHazardConfigReferenceValidatorWindow::Column_PropertyPath)
		{
			return MakeText(Row->ReferencePath.ToString());
		}
		if (ColumnName == SHazardConfigReferenceValidatorWindow::Column_ReferenceType)
		{
			return MakeText(Row->GetReferenceKindString());
		}
		if (ColumnName == SHazardConfigReferenceValidatorWindow::Column_CurrentPath)
		{
			return MakeText(Row->CurrentPath);
		}
		if (ColumnName == SHazardConfigReferenceValidatorWindow::Column_ResolvedObject)
		{
			return MakeText(Row->ResolvedObjectPath);
		}
		if (ColumnName == SHazardConfigReferenceValidatorWindow::Column_Replacement)
		{
			return MakeReplacementWidget();
		}
		if (ColumnName == SHazardConfigReferenceValidatorWindow::Column_Actions)
		{
			return MakeActionsWidget();
		}

		return SNullWidget::NullWidget;
	}

private:
	TSharedRef<SWidget> GenerateGroupWidgetForColumn(const FName& ColumnName) const
	{
		const FString ClassName = GetGroupClassName();
		if (ColumnName == SHazardConfigReferenceValidatorWindow::Column_Status)
		{
			return SNew(STextBlock)
				.Text(INVTEXT("Class"))
				.Font(FAppStyle::GetFontStyle("NormalFontBold"))
				.ColorAndOpacity(FAppStyle::Get().GetSlateColor("Colors.AccentBlue"));
		}
		if (ColumnName == SHazardConfigReferenceValidatorWindow::Column_SettingsClass)
		{
			return SNew(STextBlock)
				.Text(FText::FromString(ClassName))
				.HighlightText(HighlightText)
				.Font(FAppStyle::GetFontStyle("NormalFontBold"));
		}
		if (ColumnName == SHazardConfigReferenceValidatorWindow::Column_PropertyPath)
		{
			return MakeText(FString::Printf(TEXT("%d reference(s)"), Item.IsValid() ? Item->Children.Num() : 0));
		}
		if (ColumnName == SHazardConfigReferenceValidatorWindow::Column_CurrentPath)
		{
			return MakeText(GetGroupStatusSummary());
		}
		return SNullWidget::NullWidget;
	}

	TSharedRef<SWidget> MakeText(const FString& Text) const
	{
		return SNew(STextBlock)
			.Text(FText::FromString(Text))
			.HighlightText(HighlightText);
	}

	TSharedRef<SWidget> MakeConfigFileWidget() const
	{
		if (!Row.IsValid() || Row->ConfigFile.IsEmpty())
		{
			return MakeText(FString());
		}

		const FString ConfigFile = Row->ConfigFile;
		const FString DisplayName = FPaths::GetCleanFilename(ConfigFile);
		const bool bFileExists = FPaths::FileExists(ConfigFile);
		return SNew(SHyperlink)
			.Text(FText::FromString(DisplayName))
			.ToolTipText(FText::FromString(ConfigFile))
			.IsEnabled(bFileExists)
			.OnNavigate_Lambda([ConfigFile]()
			{
				FPlatformProcess::LaunchFileInDefaultExternalApplication(*ConfigFile, nullptr, ELaunchVerb::Edit);
			});
	}

	FString GetGroupClassName() const
	{
		if (!Item.IsValid())
		{
			return TEXT("<invalid>");
		}
		const UClass* Class = Item->DeclaringClass.Get();
		return Class ? Class->GetName() : TEXT("<invalid>");
	}

	FString GetGroupStatusSummary() const
	{
		if (!Item.IsValid())
		{
			return FString();
		}

		int32 InvalidCount = 0;
		int32 RedirectedCount = 0;
		int32 PendingCount = 0;
		int32 FailedCount = 0;
		for (const FHazardConfigReferenceTreeItemPtr& Child : Item->Children)
		{
			if (!Child.IsValid() || !Child->Row.IsValid())
			{
				continue;
			}
			switch (Child->Row->Status)
			{
			case EHazardConfigReferenceStatus::Invalid:
				++InvalidCount;
				break;
			case EHazardConfigReferenceStatus::Redirected:
				++RedirectedCount;
				break;
			case EHazardConfigReferenceStatus::PendingFix:
				++PendingCount;
				break;
			case EHazardConfigReferenceStatus::SaveFailed:
				++FailedCount;
				break;
			default:
				break;
			}
		}

		return FString::Printf(TEXT("Invalid: %d, Redirected: %d, Pending: %d, Save Failed: %d"), InvalidCount, RedirectedCount, PendingCount, FailedCount);
	}

	TSharedRef<SWidget> MakeReplacementWidget()
	{
		if (!Row->IsWritable())
		{
			return SNew(STextBlock)
				.Text(INVTEXT("Read Only"))
				.ColorAndOpacity(FAppStyle::Get().GetSlateColor("Colors.Secondary"));
		}

		const UClass* ExpectedClass = Row->ExpectedBaseClass.IsValid() ? Row->ExpectedBaseClass.Get() : UObject::StaticClass();
		if (Row->IsClassReference())
		{
			return SNew(SBox)
				.MinDesiredWidth(220.0f)
				[
					SNew(SClassPropertyEntryBox)
					.MetaClass(ExpectedClass)
					.AllowNone(true)
					.SelectedClass_Lambda([WeakRow = TWeakPtr<FHazardConfigReferenceRow>(Row)]()
					{
						const FHazardConfigReferenceRowPtr PinnedRow = WeakRow.Pin();
						const FString Path = PinnedRow.IsValid() ? PinnedRow->GetReplacementPathForApply() : FString();
						return Path.IsEmpty() ? nullptr : LoadClass<UObject>(nullptr, *Path);
					})
					.OnSetClass_Lambda([WeakRow = TWeakPtr<FHazardConfigReferenceRow>(Row)](const UClass* NewClass)
					{
						const FHazardConfigReferenceRowPtr PinnedRow = WeakRow.Pin();
						if (!PinnedRow.IsValid())
						{
							return;
						}
						PinnedRow->PendingReplacementPath = FHazardConfigReferenceValidator::GetPathForClass(NewClass);
						PinnedRow->Status = PinnedRow->PendingReplacementPath.IsEmpty() ? EHazardConfigReferenceStatus::Invalid : EHazardConfigReferenceStatus::PendingFix;
						PinnedRow->Message = PinnedRow->PendingReplacementPath.IsEmpty() ? INVTEXT("Replacement was cleared.") : INVTEXT("Replacement is pending.");
					})
				];
		}

		return SNew(SBox)
			.MinDesiredWidth(220.0f)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(ExpectedClass)
				.AllowClear(true)
				.DisplayThumbnail(false)
				.ObjectPath_Lambda([WeakRow = TWeakPtr<FHazardConfigReferenceRow>(Row)]()
				{
					const FHazardConfigReferenceRowPtr PinnedRow = WeakRow.Pin();
					return PinnedRow.IsValid() ? PinnedRow->GetReplacementPathForApply() : FString();
				})
				.OnObjectChanged_Lambda([WeakRow = TWeakPtr<FHazardConfigReferenceRow>(Row)](const FAssetData& AssetData)
				{
					const FHazardConfigReferenceRowPtr PinnedRow = WeakRow.Pin();
					if (!PinnedRow.IsValid())
					{
						return;
					}
					PinnedRow->PendingReplacementPath = AssetData.IsValid() ? AssetData.GetSoftObjectPath().ToString() : FString();
					PinnedRow->Status = PinnedRow->PendingReplacementPath.IsEmpty() ? EHazardConfigReferenceStatus::Invalid : EHazardConfigReferenceStatus::PendingFix;
					PinnedRow->Message = PinnedRow->PendingReplacementPath.IsEmpty() ? INVTEXT("Replacement was cleared.") : INVTEXT("Replacement is pending.");
				})
			];
	}

	TSharedRef<SWidget> MakeActionsWidget()
	{
		auto MakeSmallButton = [](const FText& Label, const FText& ToolTip, const FOnClicked& OnClicked, const TAttribute<bool>& IsEnabled = TAttribute<bool>(true))
		{
			return SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ContentPadding(FMargin(4.0f, 2.0f))
				.ToolTipText(ToolTip)
				.IsEnabled(IsEnabled)
				.OnClicked(OnClicked)
				[
					SNew(STextBlock).Text(Label)
				];
		};

		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 2.0f, 0.0f)
			[
				MakeSmallButton(INVTEXT("Validate"), INVTEXT("Validate this reference again."), FOnClicked::CreateSP(this, &SConfigReferenceValidatorRow::ValidateClicked))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 2.0f, 0.0f)
			[
				MakeSmallButton(INVTEXT("Apply"), INVTEXT("Apply the selected replacement."), FOnClicked::CreateSP(this, &SConfigReferenceValidatorRow::ApplyClicked), TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SConfigReferenceValidatorRow::CanApply)))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 2.0f, 0.0f)
			[
				MakeSmallButton(INVTEXT("Clear"), INVTEXT("Clear pending replacement."), FOnClicked::CreateSP(this, &SConfigReferenceValidatorRow::ClearClicked))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 2.0f, 0.0f)
			[
				MakeSmallButton(INVTEXT("Copy Path"), INVTEXT("Copy current path to clipboard."), FOnClicked::CreateSP(this, &SConfigReferenceValidatorRow::CopyPathClicked))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 2.0f, 0.0f)
			[
				MakeSmallButton(INVTEXT("Browse"), INVTEXT("Browse to the resolved object."), FOnClicked::CreateSP(this, &SConfigReferenceValidatorRow::BrowseClicked), TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SConfigReferenceValidatorRow::CanBrowse)))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				MakeSmallButton(INVTEXT("Try Redirect"), INVTEXT("Try to resolve redirector or native class redirect."), FOnClicked::CreateSP(this, &SConfigReferenceValidatorRow::TryRedirectClicked))
			];
	}

	FText GetRowToolTipText() const
	{
		if (Item.IsValid() && Item->IsGroup())
		{
			return FText::FromString(FString::Printf(TEXT("Declaring Class: %s\nReferences: %d\n%s"), *GetGroupClassName(), Item->Children.Num(), *GetGroupStatusSummary()));
		}
		return Row.IsValid() ? Row->GetToolTipText() : FText::GetEmpty();
	}

	FReply ValidateClicked()
	{
		FHazardConfigReferenceValidator Validator;
		Validator.Validate(Row);
		return FReply::Handled();
	}

	FReply ApplyClicked()
	{
		if (const TSharedPtr<SHazardConfigReferenceValidatorWindow> Owner = OwnerWidget.Pin())
		{
			TArray<FHazardConfigReferenceRowPtr> RowsToFix;
			RowsToFix.Add(Row);
			Owner->ApplyFixes(RowsToFix);
		}
		return FReply::Handled();
	}

	FReply ClearClicked()
	{
		Row->PendingReplacementPath.Reset();
		FHazardConfigReferenceValidator Validator;
		Validator.Validate(Row);
		return FReply::Handled();
	}

	FReply CopyPathClicked() const
	{
		FPlatformApplicationMisc::ClipboardCopy(*Row->CurrentPath);
		return FReply::Handled();
	}

	FReply BrowseClicked() const
	{
		UObject* Object = nullptr;
		const FString Path = !Row->ResolvedObjectPath.IsEmpty() ? Row->ResolvedObjectPath : Row->CurrentPath;
		if (Row->IsClassReference())
		{
			if (UClass* Class = LoadClass<UObject>(nullptr, *Path))
			{
				Object = Class->ClassGeneratedBy ? Class->ClassGeneratedBy : Class;
			}
		}
		else
		{
			Object = StaticLoadObject(UObject::StaticClass(), nullptr, *Path);
		}

		if (Object && GEditor)
		{
			TArray<UObject*> Objects;
			Objects.Add(Object);
			GEditor->SyncBrowserToObjects(Objects);
		}
		return FReply::Handled();
	}

	FReply TryRedirectClicked()
	{
		FHazardConfigReferenceValidator Validator;
		if (Validator.TryResolveRedirect(Row))
		{
			Row->Status = EHazardConfigReferenceStatus::Redirected;
		}
		else
		{
			Row->Message = INVTEXT("No redirect could be resolved.");
		}
		return FReply::Handled();
	}

	bool CanApply() const
	{
		return Row.IsValid() && Row->IsFixable();
	}

	bool CanBrowse() const
	{
		return Row.IsValid() && (Row->Status == EHazardConfigReferenceStatus::Valid || !Row->ResolvedObjectPath.IsEmpty());
	}

	FHazardConfigReferenceRowPtr Row;
	FHazardConfigReferenceTreeItemPtr Item;
	TWeakPtr<SHazardConfigReferenceValidatorWindow> OwnerWidget;
	TAttribute<FText> HighlightText;
};

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SHazardConfigReferenceValidatorWindow::Construct(const FArguments& InArgs)
{
	Scan();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8.0f, 8.0f, 8.0f, 4.0f)
		[
			MakeToolbar()
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(3.0f)
			[
				MakeResultsTable()
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
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

TSharedRef<SWidget> SHazardConfigReferenceValidatorWindow::MakeToolbar()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				ConfigReferenceValidatorUI::MakeToolbarButton(INVTEXT("Scan"), "Icons.Search", INVTEXT("Scan Developer Settings config references."), FOnClicked::CreateSP(this, &ThisClass::ScanClicked))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				ConfigReferenceValidatorUI::MakeToolbarButton(INVTEXT("Rescan"), "Icons.Refresh", INVTEXT("Reload settings config and scan again."), FOnClicked::CreateSP(this, &ThisClass::RescanClicked))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				ConfigReferenceValidatorUI::MakeToolbarButton(INVTEXT("Fix Selected"), "Icons.Check", INVTEXT("Apply pending or suggested replacements for selected rows."), FOnClicked::CreateSP(this, &ThisClass::FixSelectedClicked))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				ConfigReferenceValidatorUI::MakeToolbarButton(INVTEXT("Fix All Fixable"), "Icons.CheckCircle", INVTEXT("Apply every pending or redirected replacement currently found."), FOnClicked::CreateSP(this, &ThisClass::FixAllFixableClicked))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
			[
				ConfigReferenceValidatorUI::MakeToolbarButton(INVTEXT("Clear Pending Fixes"), "Icons.X", INVTEXT("Clear all pending replacements."), FOnClicked::CreateSP(this, &ThisClass::ClearPendingFixesClicked))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				ConfigReferenceValidatorUI::MakeToolbarButton(INVTEXT("Copy Report"), "Icons.Clipboard", INVTEXT("Copy a plain text validation report to the clipboard."), FOnClicked::CreateSP(this, &ThisClass::CopyReportClicked))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SAssignNew(SearchBox, SSearchBox)
				.HintText(INVTEXT("Search class, property, path, config, or status..."))
				.OnTextChanged(this, &ThisClass::HandleSearchChanged)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				ConfigReferenceValidatorUI::MakeCheckBox(INVTEXT("Show Invalid Only"), INVTEXT("Show invalid, redirected, pending, and failed rows only."), TAttribute<ECheckBoxState>::CreateLambda([this]() { return bShowInvalidOnly ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateSP(this, &ThisClass::SetShowInvalidOnly))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				ConfigReferenceValidatorUI::MakeCheckBox(INVTEXT("Show Empty"), INVTEXT("Include empty references."), TAttribute<ECheckBoxState>::CreateLambda([this]() { return bShowEmpty ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateSP(this, &ThisClass::SetShowEmpty))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				ConfigReferenceValidatorUI::MakeCheckBox(INVTEXT("Show Unsupported"), INVTEXT("Include discovered references that cannot be safely edited."), TAttribute<ECheckBoxState>::CreateLambda([this]() { return bShowUnsupported ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateSP(this, &ThisClass::SetShowUnsupported))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(18.0f, 0.0f, 12.0f, 0.0f)
			[
				ConfigReferenceValidatorUI::MakeCheckBox(INVTEXT("Project Settings Only"), INVTEXT("Only include settings classes owned by project modules."), TAttribute<ECheckBoxState>::CreateLambda([this]() { return bProjectSettingsOnly ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateSP(this, &ThisClass::SetProjectSettingsOnly))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				ConfigReferenceValidatorUI::MakeCheckBox(INVTEXT("Include Plugin Settings"), INVTEXT("Include settings classes owned by plugins."), TAttribute<ECheckBoxState>::CreateLambda([this]() { return bIncludePluginSettings ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateSP(this, &ThisClass::SetIncludePluginSettings))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				ConfigReferenceValidatorUI::MakeCheckBox(INVTEXT("Include Engine Settings"), INVTEXT("Include engine and editor settings classes."), TAttribute<ECheckBoxState>::CreateLambda([this]() { return bIncludeEngineSettings ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateSP(this, &ThisClass::SetIncludeEngineSettings))
			]
		];
}

TSharedRef<SWidget> SHazardConfigReferenceValidatorWindow::MakeResultsTable()
{
	const TSharedPtr<SHeaderRow> HeaderRow =
		SNew(SHeaderRow)
		+ SHeaderRow::Column(Column_Status).DefaultLabel(INVTEXT("Status")).FixedWidth(96.0f)
		+ SHeaderRow::Column(Column_SettingsClass).DefaultLabel(INVTEXT("Settings Class")).ManualWidth(180.0f)
		+ SHeaderRow::Column(Column_ConfigFile).DefaultLabel(INVTEXT("Config File")).ManualWidth(150.0f)
		+ SHeaderRow::Column(Column_ConfigSection).DefaultLabel(INVTEXT("Config Section")).ManualWidth(220.0f)
		+ SHeaderRow::Column(Column_PropertyPath).DefaultLabel(INVTEXT("Property Path")).ManualWidth(260.0f)
		+ SHeaderRow::Column(Column_ReferenceType).DefaultLabel(INVTEXT("Reference Type")).FixedWidth(120.0f)
		+ SHeaderRow::Column(Column_CurrentPath).DefaultLabel(INVTEXT("Current Path")).ManualWidth(320.0f)
		+ SHeaderRow::Column(Column_ResolvedObject).DefaultLabel(INVTEXT("Resolved Object")).ManualWidth(260.0f)
		+ SHeaderRow::Column(Column_Replacement).DefaultLabel(INVTEXT("Replacement")).ManualWidth(260.0f)
		+ SHeaderRow::Column(Column_Actions).DefaultLabel(INVTEXT("Actions")).ManualWidth(390.0f);

	SAssignNew(TreeView, STreeView<FHazardConfigReferenceTreeItemPtr>)
		.TreeItemsSource(&FilteredTreeItems)
		.SelectionMode(ESelectionMode::Multi)
		.OnGenerateRow(this, &ThisClass::HandleGenerateRow)
		.OnGetChildren(this, &ThisClass::HandleGetChildren)
		.HeaderRow(HeaderRow);

	return TreeView.ToSharedRef();
}

TSharedRef<ITableRow> SHazardConfigReferenceValidatorWindow::HandleGenerateRow(FHazardConfigReferenceTreeItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SConfigReferenceValidatorRow, OwnerTable)
		.Item(Item)
		.OwnerWidget(StaticCastSharedRef<SHazardConfigReferenceValidatorWindow>(AsShared()))
		.HighlightText(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &ThisClass::GetSearchHighlightText)));
}

void SHazardConfigReferenceValidatorWindow::HandleGetChildren(FHazardConfigReferenceTreeItemPtr Item, TArray<FHazardConfigReferenceTreeItemPtr>& OutChildren) const
{
	if (Item.IsValid())
	{
		OutChildren.Append(Item->Children);
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SHazardConfigReferenceValidatorWindow::Scan()
{
	FHazardConfigReferenceScanOptions Options;
	Options.bProjectSettingsOnly = bProjectSettingsOnly;
	Options.bIncludePluginSettings = bIncludePluginSettings;
	Options.bIncludeEngineSettings = bIncludeEngineSettings;

	FHazardConfigReferenceScanner Scanner;
	Scanner.Scan(Options, Rows);
	RefreshFilter();
}

void SHazardConfigReferenceValidatorWindow::RefreshFilter()
{
	RebuildTree();
	RefreshTable();
}

void SHazardConfigReferenceValidatorWindow::RefreshTable()
{
	if (TreeView.IsValid())
	{
		TreeView->RequestTreeRefresh();
		for (const FHazardConfigReferenceTreeItemPtr& RootItem : FilteredTreeItems)
		{
			TreeView->SetItemExpansion(RootItem, true);
		}
	}
}

void SHazardConfigReferenceValidatorWindow::HandleSearchChanged(const FText& InSearchText)
{
	SearchText = InSearchText;
	RefreshFilter();
}

void SHazardConfigReferenceValidatorWindow::SetShowInvalidOnly(const ECheckBoxState NewState)
{
	bShowInvalidOnly = NewState == ECheckBoxState::Checked;
	RefreshFilter();
}

void SHazardConfigReferenceValidatorWindow::SetShowEmpty(const ECheckBoxState NewState)
{
	bShowEmpty = NewState == ECheckBoxState::Checked;
	RefreshFilter();
}

void SHazardConfigReferenceValidatorWindow::SetShowUnsupported(const ECheckBoxState NewState)
{
	bShowUnsupported = NewState == ECheckBoxState::Checked;
	RefreshFilter();
}

void SHazardConfigReferenceValidatorWindow::SetProjectSettingsOnly(const ECheckBoxState NewState)
{
	bProjectSettingsOnly = NewState == ECheckBoxState::Checked;
	Scan();
}

void SHazardConfigReferenceValidatorWindow::SetIncludePluginSettings(const ECheckBoxState NewState)
{
	bIncludePluginSettings = NewState == ECheckBoxState::Checked;
	Scan();
}

void SHazardConfigReferenceValidatorWindow::SetIncludeEngineSettings(const ECheckBoxState NewState)
{
	bIncludeEngineSettings = NewState == ECheckBoxState::Checked;
	Scan();
}

FReply SHazardConfigReferenceValidatorWindow::ScanClicked()
{
	Scan();
	return FReply::Handled();
}

FReply SHazardConfigReferenceValidatorWindow::RescanClicked()
{
	Scan();
	return FReply::Handled();
}

FReply SHazardConfigReferenceValidatorWindow::FixSelectedClicked()
{
	ApplyFixes(GetSelectedRows());
	return FReply::Handled();
}

FReply SHazardConfigReferenceValidatorWindow::FixAllFixableClicked()
{
	TArray<FHazardConfigReferenceRowPtr> RowsToFix;
	for (const FHazardConfigReferenceRowPtr& Row : Rows)
	{
		if (Row.IsValid() && Row->IsFixable())
		{
			RowsToFix.Add(Row);
		}
	}
	ApplyFixes(RowsToFix);
	return FReply::Handled();
}

FReply SHazardConfigReferenceValidatorWindow::ClearPendingFixesClicked()
{
	FHazardConfigReferenceValidator Validator;
	for (const FHazardConfigReferenceRowPtr& Row : Rows)
	{
		if (Row.IsValid() && Row->HasPendingReplacement())
		{
			Row->PendingReplacementPath.Reset();
			Validator.Validate(Row);
		}
	}
	RefreshFilter();
	return FReply::Handled();
}

FReply SHazardConfigReferenceValidatorWindow::CopyReportClicked() const
{
	const FString Report = BuildReport();
	FPlatformApplicationMisc::ClipboardCopy(*Report);
	return FReply::Handled();
}

bool SHazardConfigReferenceValidatorWindow::DoesRowMatchFilter(const FHazardConfigReferenceRowPtr& Row) const
{
	if (!Row.IsValid())
	{
		return false;
	}

	if (bShowInvalidOnly)
	{
		const bool bProblemStatus = Row->Status == EHazardConfigReferenceStatus::Invalid
			|| Row->Status == EHazardConfigReferenceStatus::Redirected
			|| Row->Status == EHazardConfigReferenceStatus::PendingFix
			|| Row->Status == EHazardConfigReferenceStatus::SaveFailed;
		if (!bProblemStatus)
		{
			return false;
		}
	}
	if (!bShowEmpty && Row->Status == EHazardConfigReferenceStatus::Empty)
	{
		return false;
	}
	if (!bShowUnsupported && Row->Status == EHazardConfigReferenceStatus::Unsupported)
	{
		return false;
	}

	const FString Filter = SearchText.ToString().TrimStartAndEnd();
	if (Filter.IsEmpty())
	{
		return true;
	}

	TArray<FString> SearchStrings;
	SearchStrings.Add(Row->GetSettingsClassName());
	SearchStrings.Add(Row->GetDeclaringClassName());
	SearchStrings.Add(Row->ReferencePath.ToString());
	SearchStrings.Add(Row->CurrentPath);
	SearchStrings.Add(Row->ConfigFile);
	SearchStrings.Add(Row->ConfigSection);
	SearchStrings.Add(Row->GetStatusString());
	SearchStrings.Add(Row->GetReferenceKindString());
	SearchStrings.Add(Row->ResolvedObjectPath);
	SearchStrings.Add(Row->SuggestedReplacementPath);
	SearchStrings.Add(Row->PendingReplacementPath);

	for (const FString& SearchString : SearchStrings)
	{
		if (SearchString.Contains(Filter, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

void SHazardConfigReferenceValidatorWindow::RebuildTree()
{
	FilteredTreeItems.Reset();
	FilteredReferenceCount = 0;

	TMap<UClass*, FHazardConfigReferenceTreeItemPtr> GroupItemsByClass;
	for (const FHazardConfigReferenceRowPtr& Row : Rows)
	{
		if (!DoesRowMatchFilter(Row))
		{
			continue;
		}

		UClass* DeclaringClass = Row->DeclaringClass.IsValid() ? Row->DeclaringClass.Get() : Row->SettingsClass.Get();
		FHazardConfigReferenceTreeItemPtr& GroupItem = GroupItemsByClass.FindOrAdd(DeclaringClass);
		if (!GroupItem.IsValid())
		{
			GroupItem = MakeShared<FHazardConfigReferenceTreeItem>();
			GroupItem->DeclaringClass = DeclaringClass;
			FilteredTreeItems.Add(GroupItem);
		}

		FHazardConfigReferenceTreeItemPtr ChildItem = MakeShared<FHazardConfigReferenceTreeItem>();
		ChildItem->DeclaringClass = DeclaringClass;
		ChildItem->Row = Row;
		GroupItem->Children.Add(ChildItem);
		++FilteredReferenceCount;
	}

	FilteredTreeItems.Sort([](const FHazardConfigReferenceTreeItemPtr& A, const FHazardConfigReferenceTreeItemPtr& B)
	{
		const FString AName = A.IsValid() && A->DeclaringClass.IsValid() ? A->DeclaringClass->GetName() : FString();
		const FString BName = B.IsValid() && B->DeclaringClass.IsValid() ? B->DeclaringClass->GetName() : FString();
		return AName < BName;
	});

	for (const FHazardConfigReferenceTreeItemPtr& GroupItem : FilteredTreeItems)
	{
		if (!GroupItem.IsValid())
		{
			continue;
		}

		GroupItem->Children.Sort([](const FHazardConfigReferenceTreeItemPtr& A, const FHazardConfigReferenceTreeItemPtr& B)
		{
			const FString APath = A.IsValid() && A->Row.IsValid() ? A->Row->ReferencePath.ToString() : FString();
			const FString BPath = B.IsValid() && B->Row.IsValid() ? B->Row->ReferencePath.ToString() : FString();
			return APath < BPath;
		});
	}
}

FText SHazardConfigReferenceValidatorWindow::GetStatusText() const
{
	int32 ValidCount = 0;
	int32 InvalidCount = 0;
	int32 EmptyCount = 0;
	int32 RedirectedCount = 0;
	int32 UnsupportedCount = 0;
	int32 PendingCount = 0;
	int32 FailedCount = 0;

	for (const FHazardConfigReferenceRowPtr& Row : Rows)
	{
		if (!Row.IsValid())
		{
			continue;
		}
		switch (Row->Status)
		{
		case EHazardConfigReferenceStatus::Valid:
			++ValidCount;
			break;
		case EHazardConfigReferenceStatus::Invalid:
			++InvalidCount;
			break;
		case EHazardConfigReferenceStatus::Empty:
			++EmptyCount;
			break;
		case EHazardConfigReferenceStatus::Redirected:
			++RedirectedCount;
			break;
		case EHazardConfigReferenceStatus::Unsupported:
			++UnsupportedCount;
			break;
		case EHazardConfigReferenceStatus::PendingFix:
			++PendingCount;
			break;
		case EHazardConfigReferenceStatus::SaveFailed:
			++FailedCount;
			break;
		default:
			break;
		}
	}

	return FText::Format(INVTEXT("{0} of {1} reference(s) shown. Valid: {2}, Invalid: {3}, Empty: {4}, Redirected: {5}, Unsupported: {6}, Pending: {7}, Save Failed: {8}. Last fix: {9} fixed, {10} failed."),
		FText::AsNumber(FilteredReferenceCount),
		FText::AsNumber(Rows.Num()),
		FText::AsNumber(ValidCount),
		FText::AsNumber(InvalidCount),
		FText::AsNumber(EmptyCount),
		FText::AsNumber(RedirectedCount),
		FText::AsNumber(UnsupportedCount),
		FText::AsNumber(PendingCount),
		FText::AsNumber(FailedCount),
		FText::AsNumber(LastFixedCount),
		FText::AsNumber(LastFailedCount));
}

FSlateColor SHazardConfigReferenceValidatorWindow::GetStatusColor() const
{
	for (const FHazardConfigReferenceRowPtr& Row : Rows)
	{
		if (Row.IsValid() && (Row->Status == EHazardConfigReferenceStatus::Invalid || Row->Status == EHazardConfigReferenceStatus::SaveFailed))
		{
			return FAppStyle::Get().GetSlateColor("Colors.AccentRed");
		}
	}
	for (const FHazardConfigReferenceRowPtr& Row : Rows)
	{
		if (Row.IsValid() && (Row->Status == EHazardConfigReferenceStatus::Redirected || Row->Status == EHazardConfigReferenceStatus::PendingFix))
		{
			return FAppStyle::Get().GetSlateColor("Colors.AccentYellow");
		}
	}
	return FSlateColor::UseForeground();
}

FText SHazardConfigReferenceValidatorWindow::GetSearchHighlightText() const
{
	return SearchText;
}

TArray<FHazardConfigReferenceRowPtr> SHazardConfigReferenceValidatorWindow::GetSelectedRows() const
{
	TArray<FHazardConfigReferenceRowPtr> SelectedRows;
	if (!TreeView.IsValid())
	{
		return SelectedRows;
	}

	for (const FHazardConfigReferenceTreeItemPtr& Item : TreeView->GetSelectedItems())
	{
		if (!Item.IsValid())
		{
			continue;
		}

		if (Item->Row.IsValid())
		{
			SelectedRows.Add(Item->Row);
			continue;
		}

		for (const FHazardConfigReferenceTreeItemPtr& Child : Item->Children)
		{
			if (Child.IsValid() && Child->Row.IsValid())
			{
				SelectedRows.Add(Child->Row);
			}
		}
	}

	return SelectedRows;
}

void SHazardConfigReferenceValidatorWindow::ApplyFixes(const TArray<FHazardConfigReferenceRowPtr>& RowsToFix)
{
	LastFixedCount = 0;
	LastFailedCount = 0;
	FHazardConfigReferenceFixer Fixer;

	for (const FHazardConfigReferenceRowPtr& Row : RowsToFix)
	{
		if (!Row.IsValid() || !Row->IsFixable())
		{
			continue;
		}

		FText Error;
		if (Fixer.ApplyFix(Row, Error))
		{
			++LastFixedCount;
		}
		else
		{
			++LastFailedCount;
			Row->Status = EHazardConfigReferenceStatus::SaveFailed;
			Row->Message = Error;
		}
	}

	FNotificationInfo Info(FText::Format(INVTEXT("Config Reference Validator: {0} fixed, {1} failed."), FText::AsNumber(LastFixedCount), FText::AsNumber(LastFailedCount)));
	Info.ExpireDuration = 4.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	if (LastFixedCount > 0)
	{
		Scan();
	}
	else
	{
		RefreshFilter();
	}
}

FString SHazardConfigReferenceValidatorWindow::BuildReport() const
{
	int32 ValidCount = 0;
	int32 InvalidCount = 0;
	int32 EmptyCount = 0;
	int32 RedirectedCount = 0;
	int32 UnsupportedCount = 0;

	TArray<FString> Lines;
	Lines.Add(TEXT("Config Reference Validator Report"));
	Lines.Add(FString::Printf(TEXT("Total references: %d"), Rows.Num()));

	for (const FHazardConfigReferenceRowPtr& Row : Rows)
	{
		if (!Row.IsValid())
		{
			continue;
		}
		switch (Row->Status)
		{
		case EHazardConfigReferenceStatus::Valid:
			++ValidCount;
			break;
		case EHazardConfigReferenceStatus::Invalid:
			++InvalidCount;
			break;
		case EHazardConfigReferenceStatus::Empty:
			++EmptyCount;
			break;
		case EHazardConfigReferenceStatus::Redirected:
			++RedirectedCount;
			break;
		case EHazardConfigReferenceStatus::Unsupported:
			++UnsupportedCount;
			break;
		default:
			break;
		}
	}

	Lines.Add(FString::Printf(TEXT("Valid: %d"), ValidCount));
	Lines.Add(FString::Printf(TEXT("Invalid: %d"), InvalidCount));
	Lines.Add(FString::Printf(TEXT("Empty: %d"), EmptyCount));
	Lines.Add(FString::Printf(TEXT("Redirected: %d"), RedirectedCount));
	Lines.Add(FString::Printf(TEXT("Unsupported: %d"), UnsupportedCount));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("Invalid and redirected references:"));

	for (const FHazardConfigReferenceRowPtr& Row : Rows)
	{
		if (!Row.IsValid())
		{
			continue;
		}
		if (Row->Status == EHazardConfigReferenceStatus::Invalid || Row->Status == EHazardConfigReferenceStatus::Redirected || Row->Status == EHazardConfigReferenceStatus::SaveFailed)
		{
			Lines.Add(FString::Printf(TEXT("%s | %s | %s | %s | %s | %s"),
				*Row->GetDeclaringClassName(),
				*Row->GetSettingsClassName(),
				*Row->ConfigFile,
				*Row->ReferencePath.ToString(),
				*Row->CurrentPath,
				*Row->GetReplacementPathForApply()));
		}
	}

	return FString::Join(Lines, TEXT("\n"));
}
}
