// Copyright Neyl Sullivan 2022

#include "SObjectOutliner.h"

#include "SlateOptMacros.h"
#include "ObjectOutlinerTypes.h"
#include "SObjectOutlinerTableRow.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "ObjectOutlinerFilter.h"
#include "ObjectOutlinerModel.h"
#include "StaticMeshDescription.h"
#include "ToolMenus.h"
#include "Algo/ForEach.h"
#include "Animation/AnimNode_TransitionPoseEvaluator.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "HazardToolsObjectOutlinerSettings.h"

namespace HazardTools
{
const FName SObjectOutliner::Column_ID_Name = "Name";
const FName SObjectOutliner::Column_ID_Class = "Class";
const FName SObjectOutliner::Column_ID_Memory = "Memory";

SObjectOutliner::~SObjectOutliner()
{
	SettingsClass::GetMutable().TryUpdateDefaultConfigFile();
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SObjectOutliner::Construct(const FArguments& InArgs)
{
	GenerateImperativeFilters(ImperativeFilters);
	GenerateDropDownFilters(DropDownFilters);

	//DisplayMode = EDisplayMode::List;
	//SortByColumn = NAME_Name;

	// Load config
	auto InitFilters = [](TArray<TSharedPtr<FObjectOutlinerFilter>>& Filters, const TSet<FName>& EnabledFilters)
	{
		for (const TSharedPtr<FObjectOutlinerFilter> Filter : Filters)
		{
			if (EnabledFilters.Contains(Filter->FilterName))
			{
				Filter->bEnabled = true; // Only set filter enabled, some filters may be enabled by default
			}
		}
	};
	InitFilters(ImperativeFilters, SettingsClass::Get().ActiveImperativeFilterNames);
	InitFilters(DropDownFilters, SettingsClass::Get().ActiveDropDownFilterNames);

	//Setup the SearchBox filter
	{
		const auto Delegate = TTextFilter<const UObject&>::FItemToStringArray::CreateSP(this, &SObjectOutliner::PopulateSearchStrings);
		SearchBoxFilter = MakeShareable(new TTextFilter(Delegate));
		SearchBoxFilter->OnChanged().AddSP(this, &SObjectOutliner::Populate);
	}

	Model = MakeShared<FObjectOutlinerModel>()
	        ->SetShouldPassFilter(FObjectOutlinerModel::FShouldPassItem::CreateSP(this, &SObjectOutliner::ShouldItemPassFilter))
	        ->SetShouldPassTextFilter(FObjectOutlinerModel::FShouldPassItem::CreateSP(this, &SObjectOutliner::ShouldItemPassTextFilter));

	const TSharedRef<SVerticalBox> LeftPanelVerticalBox = SNew(SVerticalBox);

	LeftPanelVerticalBox->AddSlot()
	                    .AutoHeight()
	                    .Padding(8.0f, 8.0f, 8.0f, 4.0f)
	[
		MakeToolbar()
	];

	// Tree / List view
	LeftPanelVerticalBox->AddSlot()
	                    .FillHeight(1.0f)
	[
		SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(FMargin(0.0f, 4.0f))
		[
			MakeTreeView()
		]
	];

	// Bottom panel status bar
	LeftPanelVerticalBox->AddSlot()
	                    .AutoHeight()
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("Brushes.Header"))
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Left)
		.Padding(FMargin(14, 9))
		[
			SNew(STextBlock)
			.Text(this, &SObjectOutliner::GetFilterStatusText)
			.ColorAndOpacity(this, &SObjectOutliner::GetFilterStatusTextColor)
		]
	];

	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Horizontal)

		+ SSplitter::Slot()
		.Value(3)
		[
			SNew(SBorder)
			.Padding(FMargin(3))
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				LeftPanelVerticalBox
			]
		]

		+ SSplitter::Slot()
		.Value(1)
		[
			SNew(SBorder)
			.Padding(FMargin(3))
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				MakePropertyEditor()
			]
		]
	];

	Populate();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TAttribute<FText> SObjectOutliner::GetTextFilterHighlightText() const
{
	return TAttribute<FText>::Create(
		TAttribute<FText>::FGetter::CreateLambda([&]()
		{
			if (SearchBoxFilter.IsValid())
			{
				return SearchBoxFilter->GetRawFilterText();
			}
			return FText();
		}));
}

bool SObjectOutliner::IsTreeViewMode()
{
	return static_cast<EDisplayMode>(SettingsClass::Get().DisplayMode) == EDisplayMode::Tree;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void SObjectOutliner::PopulateSearchStrings(const UObject& TreeItem, TArray<FString>& OutSearchStrings) const
{
	OutSearchStrings.Emplace(TreeItem.GetClass()->GetName());
	OutSearchStrings.Emplace(TreeItem.GetName());
}

void SObjectOutliner::Populate() const
{
	// Keep selection
	TWeakObjectPtr<UObject> SelectedObject;
	if (TreeView->GetNumItemsSelected() > 0 && TreeView->GetSelectedItems()[0].IsValid())
	{
		SelectedObject = TreeView->GetSelectedItems()[0].Get()->ObjectPtr;
	}

	// Keep expansion state
	TSet<UObject*> SavedExpandedObjectsSet;
	if (IsTreeViewMode())
	{
		TSet<FObjectOutlinerItemPtr> ExpandedItemsSet;
		TreeView->GetExpandedItems(ExpandedItemsSet);
		for (const auto Item : ExpandedItemsSet)
		{
			if (UObject* Object = Item->ObjectPtr.Get())
			{
				SavedExpandedObjectsSet.Add(Object);
			}
		}
	}

	TMap<UObject*, FObjectOutlinerItemPtr> ProcessedObjectsMap;
	Model->UpdateContent(IsTreeViewMode(), &ProcessedObjectsMap);

	SortItems(Model->GetMutableRootContent());

	//Restore expansion state
	for (const UObject* ExpandedObject : SavedExpandedObjectsSet)
	{
		if (IsValid(ExpandedObject))
		{
			if (const FObjectOutlinerItemPtr* NewExpandedItemCandidatePtr = ProcessedObjectsMap.Find(ExpandedObject))
			{
				TreeView->SetItemExpansion(*NewExpandedItemCandidatePtr, true);
			}
		}
	}

	//Restore selection
	if (SelectedObject.IsValid())
	{
		if (const FObjectOutlinerItemPtr* LastSelectedItemPtr = ProcessedObjectsMap.Find(SelectedObject.Get()))
		{
			FObjectOutlinerItemPtr LastSelectedItem = *LastSelectedItemPtr;
			TreeView->SetItemSelection(LastSelectedItem, true);
			// If selected item inside tree expand it too
			if (IsTreeViewMode())
			{
				while (LastSelectedItem->Parent.IsValid())
				{
					TreeView->SetItemExpansion(LastSelectedItem->Parent.Pin(), true);
					LastSelectedItem = LastSelectedItem->Parent.Pin();
				}
			}
			TreeView->RequestScrollIntoView(LastSelectedItem);
		}
	}

	TreeView->RequestTreeRefresh();
}

TSharedRef<ITableRow> SObjectOutliner::HandleListGenerateRow(const FObjectOutlinerItemPtr ObjectPtr, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SObjectOutlinerTableRow, OwnerTable, ObjectPtr.ToSharedRef(), SharedThis(this));
}

TSharedPtr<SWidget> SObjectOutliner::GetContextMenuContent() const
{
	static const FName MenuName = "HazardTools.ObjectOutliner.ContextMenu";

	UToolMenus* ToolMenus = UToolMenus::Get();
	if (!ToolMenus->IsMenuRegistered(MenuName))
	{
		ToolMenus->RegisterMenu(MenuName);
	}

	const FToolMenuContext Context(nullptr, TSharedPtr<FExtender>());
	UToolMenu* Menu = ToolMenus->GenerateMenu(MenuName, Context);

	if (TreeView->GetNumItemsSelected() > 0 && TreeView->GetSelectedItems()[0].IsValid())
	{
		const FObjectOutlinerItemPtr ObjectOutlinerItem = TreeView->GetSelectedItems()[0];
		FObjectOutlinerItemActions::GenerateContextMenu(Menu, ObjectOutlinerItem);
	}

	TSharedRef<SWidget> MenuWidget = ToolMenus->GenerateWidget(Menu);
	return MenuWidget;
}


void SObjectOutliner::HandleListSelectionChanged(const FObjectOutlinerItemPtr InItem, ESelectInfo::Type /*SelectInfo*/) const
{
	if (!InItem.IsValid())
	{
		return;
	}
	PropertyEditor->SetObject(InItem.Get()->ObjectPtr.Get());
}

TSharedRef<SWidget> SObjectOutliner::GetDropDownFiltersButtonContent()
{
	// Menu should stay open on selection if filters are not being shown
	FMenuBuilder MenuBuilder(false, nullptr);

	MenuBuilder.BeginSection(NAME_None, INVTEXT("Show"));
	{
		for (int i = 0; i < DropDownFilters.Num(); ++i)
		{
			MenuBuilder.AddMenuEntry(
				DropDownFilters[i]->Title,
				DropDownFilters[i]->Tooltip,
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateLambda(
						[&, i]()
						{
							DropDownFilters[i]->bEnabled = !DropDownFilters[i]->bEnabled;
							if (DropDownFilters[i]->bEnabled)
							{
								SettingsClass::GetMutable().ActiveDropDownFilterNames.Add(DropDownFilters[i]->FilterName);
							}
							else
							{
								SettingsClass::GetMutable().ActiveDropDownFilterNames.Remove(DropDownFilters[i]->FilterName);
							}
							Populate();
						}),
					FCanExecuteAction(),
					FIsActionChecked::CreateLambda([&, i]() { return DropDownFilters[i]->bEnabled; })
					),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
				);
		}
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(NAME_None, INVTEXT("Settings"));
	{
		MenuBuilder.AddMenuEntry(
			INVTEXT("Show only checked"),
			FText(),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda(
					[&]()
					{
						SettingsClass::GetMutable().bShowOnlyCheckedObjects = !SettingsClass::Get().bShowOnlyCheckedObjects;
						Populate();
					}),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([&]() { return SettingsClass::Get().bShowOnlyCheckedObjects; })
				),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
			);

		MenuBuilder.AddMenuEntry(
			INVTEXT("Toggle All"),
			INVTEXT("Toggle On/Off all filters"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SObjectOutliner::HandleToggleAllDropDownFilters)),
			NAME_None
			);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}


void SObjectOutliner::HandleToggleAllDropDownFilters()
{
	const int32 EnabledFiltersNum = Algo::Accumulate(DropDownFilters, 0, [](const int32 Sum, const TSharedPtr<FObjectOutlinerFilter> Filter) { return Sum + Filter->bEnabled; });
	const int32 DisabledFiltersNum = DropDownFilters.Num() - EnabledFiltersNum;

	const bool bNewEnabled = DisabledFiltersNum > EnabledFiltersNum;

	Algo::ForEach(DropDownFilters, [&](const TSharedPtr<FObjectOutlinerFilter> Filter) { Filter->bEnabled = bNewEnabled; });

	if (bNewEnabled)
	{
		Algo::ForEach(
			DropDownFilters,
			[&](const TSharedPtr<FObjectOutlinerFilter> Filter)
			{
				SettingsClass::GetMutable().ActiveDropDownFilterNames.Add(Filter->FilterName);
			});
	}
	else
	{
		SettingsClass::GetMutable().ActiveDropDownFilterNames.Empty();
	}
	Populate();
}

TSharedRef<SHorizontalBox> SObjectOutliner::MakeToolbar()
{
	const auto Toolbar = SNew(SHorizontalBox);

	// List/tree modes
	Toolbar->AddSlot()
	       .VAlign(VAlign_Center)
	       .AutoWidth()
	[
		SNew(SSegmentedControl<HazardTools::EDisplayMode>)
		.Value_Lambda([&]() { return static_cast<EDisplayMode>(SettingsClass::Get().DisplayMode); })
		.OnValueChanged_Lambda([&](const EDisplayMode NewDisplayMode)
		{
			SettingsClass::GetMutable().DisplayMode = static_cast<uint8>(NewDisplayMode);
			Populate();
		})

		+ SSegmentedControl<EDisplayMode>::Slot(EDisplayMode::List)
		.ToolTip(INVTEXT("List view"))
		[
			SNew(SBox)
			.HeightOverride(16.f)
			.WidthOverride(16.f)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.ColorAndOpacity(FSlateColor::UseForeground())
				.Image(FAppStyle::Get().GetBrush("DetailsView.EditRawProperties"))
			]
		]

		+ SSegmentedControl<EDisplayMode>::Slot(EDisplayMode::Tree)
		.ToolTip(INVTEXT("Tree view"))
		[
			SNew(SBox)
			.HeightOverride(16.f)
			.WidthOverride(16.f)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.ColorAndOpacity(FSlateColor::UseForeground())
				.Image(FAppStyle::Get().GetBrush("ContentBrowser.ShowSourcesView"))
			]
		]
	];

	Toolbar->AddSlot()
	       .VAlign(VAlign_Center)
	[
		SAssignNew(FilterTextBoxWidget, SSearchBox)
		.Visibility(EVisibility::Visible)
		.HintText(INVTEXT("Search..."))
		.ToolTipText(INVTEXT("Type here to search"))
		.OnTextChanged(this, &SObjectOutliner::OnFilterTextChanged)
	];

	// Refresh button
	Toolbar->AddSlot()
	       .VAlign(VAlign_Center)
	       .AutoWidth()
	       .Padding(4.f, 0.f, 0.f, 0.f)
	[
		SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ToolTipText(INVTEXT("Force referesh object list"))
		.OnClicked(this, &ThisClass::OnRefreshClicked)
		[
			SNew(SImage)
			.ColorAndOpacity(FSlateColor::UseForeground())
			.Image(FAppStyle::Get().GetBrush("Icons.Refresh"))
		]
	];

	Toolbar->AddSlot()
	       .VAlign(VAlign_Center)
	       .AutoWidth()
	       .Padding(4.f, 0.f, 0.f, 0.f)
	[
		MakeImperativeFilterButtons()
	];

	// View mode combo button
	Toolbar->AddSlot()
	       .VAlign(VAlign_Center)
	       .AutoWidth()
	[
		SAssignNew(ViewOptionsComboButton, SComboButton)
			.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButtonWithIcon") // Use the tool bar item style for this button
			.OnGetMenuContent(this, &SObjectOutliner::GetDropDownFiltersButtonContent)
			.HasDownArrow(false)
			.ButtonContent()
		[
			SNew(SImage)
			.ColorAndOpacity(FSlateColor::UseForeground())
			.Image(FAppStyle::Get().GetBrush("Icons.Settings"))
		]
	];

	return Toolbar;
}

TSharedRef<SHorizontalBox> SObjectOutliner::MakeImperativeFilterButtons()
{
	auto FilterBar = SNew(SHorizontalBox)
	                 .Clipping(EWidgetClipping::OnDemand);

	for (int i = 0; i < ImperativeFilters.Num(); ++i)
	{
		FilterBar->AddSlot()
		         .VAlign(VAlign_Center)
		         .AutoWidth()
		         .Padding(1.f)
		[
			SNew(SCheckBox)
			.Style(&FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("ToggleButtonCheckBox"))
			.ToolTipText(ImperativeFilters[i]->Tooltip)
			.IsChecked(ImperativeFilters[i]->bEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([&, i](const ECheckBoxState NewState)
			{
				ImperativeFilters[i]->bEnabled = NewState == ECheckBoxState::Checked ? true : false;
				if (ImperativeFilters[i]->bEnabled)
				{
					SettingsClass::GetMutable().ActiveImperativeFilterNames.Add(ImperativeFilters[i]->FilterName);
				}
				else
				{
					SettingsClass::GetMutable().ActiveImperativeFilterNames.Remove(ImperativeFilters[i]->FilterName);
				}
				Populate();
			})
			[
				SNew(STextBlock)
				.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("SmallButtonText"))
				.Justification(ETextJustify::Center)
				.Text(ImperativeFilters[i]->Title)
			]
		];
	}
	return FilterBar;
}


TSharedRef<SWidget> SObjectOutliner::MakeTreeView()
{
	SAssignNew(HeaderRowWidget, SHeaderRow)
		.CanSelectGeneratedColumn(true)

		+ SHeaderRow::Column(Column_ID_Name)
		  .DefaultLabel(INVTEXT("Name"))
		  .SortMode_Static(&ThisClass::GetColumnSortMode, Column_ID_Name)
		  .OnSort(this, &ThisClass::OnColumnSortModeChanged)
		  .FillWidth(0.75)
		  .ShouldGenerateWidget(true)

		+ SHeaderRow::Column(Column_ID_Class)
		  .DefaultLabel(INVTEXT("Class"))
		  .SortMode_Static(&ThisClass::GetColumnSortMode, Column_ID_Class)
		  .OnSort(this, &ThisClass::OnColumnSortModeChanged)
		  .FillWidth(0.15)
		  .ShouldGenerateWidget(true)

		+ SHeaderRow::Column(Column_ID_Memory)
		  .DefaultLabel(INVTEXT("Memory"))
		  .SortMode_Static(&ThisClass::GetColumnSortMode, Column_ID_Memory)
		  .OnSort(this, &ThisClass::OnColumnSortModeChanged)
		  .FillWidth(0.1)
		  .HAlignCell(HAlign_Right)
		  .HAlignHeader(HAlign_Right);

	SAssignNew(TreeView, STreeView<FObjectOutlinerItemPtr>)
	.ItemHeight(24.0f)
	.TreeItemsSource(&Model->GetRootContent())
	.SelectionMode(ESelectionMode::Single)
	.OnGenerateRow(this, &SObjectOutliner::HandleListGenerateRow)
	// Called to child items for any given parent item
	.OnGetChildren_Static(&SObjectOutliner::OnGetChildrenForOutlinerTree)
	// Called when an item is expanded or collapsed with the shift-key pressed down
	.OnSetExpansionRecursive(this, &SObjectOutliner::SetItemExpansionRecursive)
	.OnSelectionChanged(this, &SObjectOutliner::HandleListSelectionChanged)
	// Make it easier to see hierarchies when there are a lot of items
	.HighlightParentNodesForSelection(true)
	.OnContextMenuOpening(this, &SObjectOutliner::GetContextMenuContent)
	.HeaderRow(HeaderRowWidget);

	return TreeView.ToSharedRef();
}

void SObjectOutliner::OnGetChildrenForOutlinerTree(const FObjectOutlinerItemPtr InParent, TArray<FObjectOutlinerItemPtr>& OutChildren)
{
	if (static_cast<EDisplayMode>(SettingsClass::Get().DisplayMode) != EDisplayMode::Tree)
	{
		return;
	}

	for (auto& Child : InParent->GetChildren())
	{
		// Should never have bogus entries in this list
		check(Child.IsValid());
		OutChildren.Add(Child);
	}

	// If the item needs it's children sorting, do that now
	if (OutChildren.Num() && InParent->bChildrenRequireSort)
	{
		// Sort the children we returned
		SortItems(OutChildren);

		// Empty out the children and repopulate them in the correct order
		InParent->Children.Empty();
		for (auto& Child : OutChildren)
		{
			InParent->Children.Emplace(Child);
		}

		// They no longer need sorting
		InParent->bChildrenRequireSort = false;
	}
}

void SObjectOutliner::SetItemExpansionRecursive(const FObjectOutlinerItemPtr ItemPtr, const bool bInExpansionState)
{
	if (ItemPtr.IsValid())
	{
		TreeView->SetItemExpansion(ItemPtr, bInExpansionState);
		for (auto& Child : ItemPtr->Children)
		{
			if (Child.IsValid())
			{
				SetItemExpansionRecursive(Child, bInExpansionState);
			}
		}
	}
}

TSharedRef<::IDetailsView> SObjectOutliner::MakePropertyEditor()
{
	FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bAllowSearch = true;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ENameAreaSettings::HideNameArea;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Automatic;
		DetailsViewArgs.bAllowFavoriteSystem = false;
		DetailsViewArgs.bShowPropertyMatrixButton = false;
		DetailsViewArgs.bForceHiddenPropertyVisibility = true; // IMPORTANT to display UPROPERTY() only properties
	}
	PropertyEditor = EditModule.CreateDetailView(DetailsViewArgs);
	return PropertyEditor.ToSharedRef();
}

void SObjectOutliner::OnFilterTextChanged(const FText& InFilterText) const
{
	SearchBoxFilter->SetRawFilterText(InFilterText);
	FilterTextBoxWidget->SetError(SearchBoxFilter->GetFilterErrorText());
}

FReply SObjectOutliner::OnRefreshClicked() const
{
	Populate();
	return FReply::Handled();
}


FText SObjectOutliner::GetFilterStatusText() const
{
	if (IsTextFilterActive() == false)
	{
		return FText::Format(
			INVTEXT("{0} objects (Discovered: {1})"),
			FText::AsNumber(Model->GetDisplayedNum()),
			FText::AsNumber(Model->GetDiscoveredNum()));
	}

	if (Model->GetDisplayedNum() == 0)
	{
		return FText::Format(
			INVTEXT("No matching objects ({0} total, discovered {1})"),
			FText::AsNumber(Model->GetFilteredNum()),
			FText::AsNumber(Model->GetDiscoveredNum()));
	}
	return FText::Format(
		INVTEXT("Showing {0} of {1} objects (Discovered: {2})"),
		FText::AsNumber(Model->GetDisplayedNum()),
		FText::AsNumber(Model->GetFilteredNum()),
		FText::AsNumber(Model->GetDiscoveredNum()));
}

FSlateColor SObjectOutliner::GetFilterStatusTextColor() const
{
	if (IsTextFilterActive() == false)
	{
		return FSlateColor::UseForeground();
	}
	if (Model->GetDisplayedNum() == 0)
	{
		static FSlateColor AccentRedColor = FAppStyle::Get().GetSlateColor("Colors.AccentRed");
		return AccentRedColor;
	}

	static FSlateColor AccentGreenColor = FAppStyle::Get().GetSlateColor("Colors.AccentGreen");
	return AccentGreenColor;
}

bool SObjectOutliner::IsTextFilterActive() const
{
	return FilterTextBoxWidget->GetText().ToString().Len() > 0;
}

void SObjectOutliner::OnColumnSortModeChanged(const EColumnSortPriority::Type SortPriority, const FName& ColumnId, const EColumnSortMode::Type InSortMode) const
{
	SettingsClass::GetMutable().SortByColumn = ColumnId;
	SettingsClass::GetMutable().SortMode = InSortMode;
	SortItems(Model->GetMutableRootContent());
	TreeView->RequestTreeRefresh();
}

void SObjectOutliner::SortItems(TArray<FObjectOutlinerItemPtr>& Items)
{
	struct FSortItemPredicate
	{
		FSortItemPredicate(const FName InSortColumnID, const EColumnSortMode::Type InSortColumnMode)
			: SortColumnID(InSortColumnID)
			, SortColumnMode(InSortColumnMode)
		{
		}

		bool operator ()(const FObjectOutlinerItemPtr& A, const FObjectOutlinerItemPtr& B) const
		{
			if (A.IsValid() && B.IsValid())
			{
				const auto Aa = (SortColumnMode == EColumnSortMode::Ascending) ? A->ObjectPtr.Get() : B->ObjectPtr.Get();
				const auto Bb = (SortColumnMode == EColumnSortMode::Ascending) ? B->ObjectPtr.Get() : A->ObjectPtr.Get();

				if (Aa && Bb)
				{
					if (SortColumnID == Column_ID_Name)
					{
						return Aa->GetName() < Bb->GetName();
					}
					if (SortColumnID == Column_ID_Class)
					{
						return Aa->GetClass()->GetName() < Bb->GetClass()->GetName();
					}
					if (SortColumnID == Column_ID_Memory)
					{
						return Aa->GetResourceSizeBytes(EResourceSizeMode::Exclusive) < Bb->GetResourceSizeBytes(EResourceSizeMode::Exclusive);
					}
				}
			}
			return false; // fallback
		}

	private:
		FName SortColumnID;
		EColumnSortMode::Type SortColumnMode;
	};

	Items.Sort(FSortItemPredicate(SettingsClass::Get().SortByColumn, static_cast<EColumnSortMode::Type>(SettingsClass::Get().SortMode)));
	for (const FObjectOutlinerItemPtr Item : Items)
	{
		Item->bChildrenRequireSort = true;
	}
}

bool SObjectOutliner::ShouldItemPassFilter(const UObject* ObjectPtr) const
{
	if (ShouldSkipObject(ObjectPtr, ImperativeFilters))
	{
		return false;
	}

	if (SettingsClass::Get().bShowOnlyCheckedObjects)
	{
		if (IsMatchAnyEnabledFilter(ObjectPtr, DropDownFilters) == false)
		{
			return false;
		}
	}
	else if (IsMatchAnyDisabledFilter(ObjectPtr, DropDownFilters))
	{
		return false;
	}

	return true;
}

bool SObjectOutliner::ShouldItemPassTextFilter(const UObject* ObjectPtr) const
{
	if (!SearchBoxFilter->PassesFilter(*ObjectPtr))
	{
		return false;
	}
	return true;
}

EColumnSortMode::Type SObjectOutliner::GetColumnSortMode(const FName ColumnId)
{
	return SettingsClass::Get().SortByColumn == ColumnId ? static_cast<EColumnSortMode::Type>(SettingsClass::Get().SortMode) : EColumnSortMode::None;
}
}
