// Copyright Neyl Sullivan 2022


#include "SStyleBrowser.h"

#include "HazardToolsUtils.h"
#include "SlateOptMacros.h"
#include "StyleBrowserModel.h"
#include "SStyleBrowserTableRow.h"
#include "Fonts/FontMeasure.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Styling/SegmentedControlStyle.h"

namespace HazardTools
{
const FName SStyleBrowser::Column_ID_Name = "Name";
const FName SStyleBrowser::Column_ID_Set = "Set";
const FName SStyleBrowser::Column_ID_Type = "Type";
const FName SStyleBrowser::Column_ID_Preview = "Preview";

static void OnFileLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	if (const FString* Url = Metadata.Find(TEXT("href")))
	{
		const FString Path = FPaths::ConvertRelativePathToFull(*Url);
		if (!Path.Len() || !IFileManager::Get().FileExists(*Path))
		{
			return;
		}
		//UE_LOG(LogHazardTools, Warning, TEXT("OnFileLinkClicked: href=%s"), *Path)
		FPlatformProcess::ExploreFolder(*Path);
	}
}

SStyleBrowser::~SStyleBrowser()
{
	SettingsClass::GetMutable().TryUpdateDefaultConfigFile();
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SStyleBrowser::Construct(const FArguments& InArgs)
{
	FSegmentedControlStyle Style =
		FSegmentedControlStyle()
		.SetFirstControlStyle(FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("EditorViewportToolBar.ToggleButton.Start"))
		.SetLastControlStyle(FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("EditorViewportToolBar.ToggleButton.End"))
		.SetControlStyle(FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("EditorViewportToolBar.ToggleButton.Middle"));

	CustomSegmentedControlStyle = MakeShared<FSegmentedControlStyle>(Style);

	//Setup the SearchBox filter
	{
		const auto Delegate = TTextFilter<const FStyleBrowserItemPtr&>::FItemToStringArray::CreateSP(this, &ThisClass::PopulateSearchStrings);
		SearchBoxFilter = MakeShareable(new TTextFilter(Delegate));
		SearchBoxFilter->OnChanged().AddSP(this, &ThisClass::Populate);
	}

	Model = MakeShared<FStyleBrowserModel>();
	Model->SetShouldPassTextFilter(FStyleBrowserModel::FShouldPassItem::CreateSP(this, &ThisClass::ShouldItemPassTextFilter));
	Model->SetShouldAddFromParentStyleSets(SettingsClass::Get().bShouldAddFromParentStyle);
	Model->SetStyleTypeNameToDisplay(SettingsClass::Get().StyleTypeNameToDisplay);

	const TSharedRef<SVerticalBox> LeftPanelVerticalBox =
		SNew(SVerticalBox)

		// List view
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(FMargin(0.0f, 4.0f))
			[
				MakeListView()
			]
		]

		// Bottom panel status bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Header"))
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.Padding(FMargin(14, 9))
			[
				SNew(STextBlock)
				.Text(this, &ThisClass::GetFilterStatusText)
				.ColorAndOpacity(this, &ThisClass::GetFilterStatusTextColor)
			]
		];

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
		.FillHeight(1.f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)

			+ SSplitter::Slot()
			.Value(2)
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
				MakePreviewArea()
			]
		]
	];

	Populate();
}

float SStyleBrowser::GetAvailableStyleSetsDropDownButtonWidth() const
{
	TArray<FName> StyleSetNames = Model->GetAvailableStyleSets();
	float MaxWidth = 0.f;
	const auto& TextBlockFontInfo = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText").Font;
	for (auto StyleSetName : StyleSetNames)
	{
		const FVector2D TextSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(FText::FromName(StyleSetName), TextBlockFontInfo);
		MaxWidth = FMath::Max(MaxWidth, TextSize.X);
	}

	constexpr float ExtraPadding = 80.f;

	return MaxWidth + ExtraPadding;
}

TSharedRef<SWidget> SStyleBrowser::GetAvailableStyleSetsDropDownButtonContent() const
{
	TArray<FName> StyleSetNames = Model->GetAvailableStyleSets();

	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.BeginSection(NAME_None, INVTEXT("Style Sets"));
	{
		for (auto StyleSetName : StyleSetNames)
		{
			MenuBuilder.AddMenuEntry(
				FText::FromName(StyleSetName),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateLambda([&, StyleSetName]()
					{
						Model->SetDesiredStyleSetName(StyleSetName);
						Populate();
					}),
					FCanExecuteAction(),
					FIsActionChecked::CreateLambda([&, StyleSetName]()
					{
						return StyleSetName == Model->GetDesiredStyleSetName();
					})
					),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
				);
		}
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

bool SStyleBrowser::IsTextFilterActive() const
{
	return FilterTextBoxWidget->GetText().ToString().Len() > 0;
}

TSharedRef<SWidget> SStyleBrowser::MakeToolbar()
{
	const TSharedRef<SHorizontalBox> Toolbar = SNew(SHorizontalBox);

	Toolbar->AddSlot()
	       .VAlign(VAlign_Center)
	       .AutoWidth()
	[
		SNew(SBox)
		.HAlign(HAlign_Fill)
		.WidthOverride(GetAvailableStyleSetsDropDownButtonWidth())
		[
			SNew(SComboButton)
			.OnGetMenuContent(this, &SStyleBrowser::GetAvailableStyleSetsDropDownButtonContent)
			.HasDownArrow(true)
			.ButtonContent()
			[
				SNew(STextBlock).Text_Lambda([this]()
				{
					return FText::FromName(Model->GetDesiredStyleSetName());
				})
			]
		]
	];

	Toolbar->AddSlot()
	       .VAlign(VAlign_Center)
	       .AutoWidth()
	[
		SNew(SCheckBox)
			.Style(&FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("ToggleButtonCheckBox"))
			.IsEnabled_Lambda([&]()
		               {
			               return Model->HasParentStyleSet();
		               })
			.ToolTipText_Lambda([&]()
		               {
			               if (Model->HasParentStyleSet())
			               {
				               const FString HierarchyString = Model->GetStyleSetsHierarchy();
				               const FText HierarchyText = HierarchyString.IsEmpty() ? FText::GetEmpty() : FText::Format(INVTEXT("\n{0}"), FText::FromString(HierarchyString));
				               return FText::Format(INVTEXT("Append styles from parent style set(s):{0}"), HierarchyText);
			               }
			               return FText::GetEmpty();
		               })
			.IsChecked_Lambda([&]
		               {
			               return SettingsClass::Get().bShouldAddFromParentStyle && Model->HasParentStyleSet() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		               })
			.OnCheckStateChanged_Lambda([&](const ECheckBoxState NewState)
		               {
			               SettingsClass::GetMutable().bShouldAddFromParentStyle = (NewState == ECheckBoxState::Checked);
			               Model->SetShouldAddFromParentStyleSets(SettingsClass::GetMutable().bShouldAddFromParentStyle);
			               Populate();
		               })
		[
			SNew(STextBlock)
			.Font(FAppStyle::Get().GetFontStyle("FontAwesome.12"))
			.Justification(ETextJustify::Center)
			.Text(FText::FromString(TEXT("\xf234")))
		]
	];

	Toolbar->AddSlot()
	       .HAlign(HAlign_Fill)
	       .FillWidth(1.f)
	[
		SAssignNew(FilterTextBoxWidget, SSearchBox)
		.Visibility(EVisibility::Visible)
		.HintText(INVTEXT("Search..."))
		.ToolTipText(INVTEXT("Type here to search"))
		.OnTextChanged(this, &ThisClass::OnFilterTextChanged)
	];

	TSharedPtr<SSegmentedControl<FName>> SegmentedControl;
	Toolbar->AddSlot()
	       .VAlign(VAlign_Center)
	       .AutoWidth()
	[
		SAssignNew(SegmentedControl, SSegmentedControl<FName>)
		.Style(CustomSegmentedControlStyle.Get())
		.Value_Lambda([&]() { return Model->GetStyleTypeNameToDisplay(); })
		.OnValueChanged_Lambda([&](const FName NewStyleTypeNameToDisplay)
		{
			SettingsClass::GetMutable().StyleTypeNameToDisplay = NewStyleTypeNameToDisplay;
			Model->SetStyleTypeNameToDisplay(NewStyleTypeNameToDisplay);
			Populate();
		})

		+ SSegmentedControl<FName>::Slot(NAME_None).Text(INVTEXT("All"))
	];

	for (const auto& StyleCategory : Model->GetAvailableStyleTypeCategories())
	{
		const FString DisplayString = FName::NameToDisplayString(StyleCategory.ToString(), false);

		SegmentedControl->AddSlot(StyleCategory)
		                .AttachWidget
		                (
			                SNew(SBox)
							.HAlign(HAlign_Center)
							.Padding(FMargin(4.f, 0.f))
			                [
				                SNew(STextBlock)
			                .Text(FText::FromString(DisplayString))
			                .Justification(ETextJustify::Center)
			                ]);
	}

	return Toolbar;
}

TSharedRef<SWidget> SStyleBrowser::MakeListView()
{
	const TSharedPtr<SHeaderRow> HeaderRowWidget =
		SNew(SHeaderRow)
		.CanSelectGeneratedColumn(true)

		+ SHeaderRow::Column(Column_ID_Name)
		  .DefaultLabel(INVTEXT("Name"))
		  .FillWidth(0.5)
		  .ShouldGenerateWidget(true)

		+ SHeaderRow::Column(Column_ID_Set)
		  .DefaultLabel(INVTEXT("Set"))
		  .FillWidth(0.25)

		+ SHeaderRow::Column(Column_ID_Type)
		  .DefaultLabel(INVTEXT("Type"))
		  .FillWidth(0.25)

		+ SHeaderRow::Column(Column_ID_Preview)
		  .DefaultLabel(INVTEXT("Preview"))
		  .FillWidth(0.25)
		  .ShouldGenerateWidget(true);

	SAssignNew(ListView, SListView<FStyleBrowserItemPtr>)
	.ItemHeight(24.0f)
	.ListItemsSource(&Model->GetFilteredContent())
	.SelectionMode(ESelectionMode::Single)
	.OnGenerateRow(this, &ThisClass::HandleListGenerateRow)
	.OnSelectionChanged(this, &ThisClass::HandleListSelectionChanged)
	.HeaderRow(HeaderRowWidget);

	return ListView.ToSharedRef();
}

TSharedRef<SWidget> SStyleBrowser::MakePreviewArea()
{
	return
		SNew(SBox)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SSplitter)
			.Orientation(Orient_Vertical)

			+ SSplitter::Slot()
			.Value(1.f)
			[
				SNew(SBorder)
				.Padding(FMargin(4.f))
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)

					// Style name button
					+ SVerticalBox::Slot()
					  .HAlign(HAlign_Fill)
					  .VAlign(VAlign_Top)
					  .AutoHeight()
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("HoverHintOnly"))
						.OnClicked(this, &ThisClass::OnCopyStyleNameClicked)
						.ContentPadding(8.f)
						[
							SAssignNew(PreviewAreaHeaderText, STextBlock)
							.Text(FText::GetEmpty())
							.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("ProjectBrowser.VersionOverlayText"))
							.ColorAndOpacity(FLinearColor(0.728f, 0.364f, 0.003f))
						]
					]

					// Snipped button
					+ SVerticalBox::Slot()
					  .HAlign(HAlign_Fill)
					  .VAlign(VAlign_Top)
					  .AutoHeight()
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("HoverHintOnly"))
						.OnClicked(this, &SStyleBrowser::OnCopyCodeSnippedClicked)
						.ContentPadding(8.f)
						[
							SAssignNew(PreviewAreaHeaderSnippedText, SRichTextBlock)
							.Text(FText::GetEmpty())
							.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.Normal"))
							.DecoratorStyleSet(&FAppStyle::Get())
							.Justification(ETextJustify::Left)
						]
					]

					+ SVerticalBox::Slot()
					  .HAlign(HAlign_Fill)
					  .VAlign(VAlign_Fill)
					  .Padding(12.f)
					  .FillHeight(1.f)
					[
						SNew(SBorder)
						.Padding(0.f)
						.BorderImage(FAppStyle::Get().GetBrush("Checkerboard"))
						.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.05f))
						.Clipping(EWidgetClipping::ClipToBounds)
						[
							SAssignNew(PreviewAreaBox, SBox)
						]
					]
				]
			]

			+ SSplitter::Slot()
			.Value(1.f)
			[
				SNew(SBorder)
				.Padding(FMargin(4.f))
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(PreviewAreaFooterText, SRichTextBlock)
						.Text(FText::GetEmpty())
						.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.Normal"))
						.DecoratorStyleSet(&FAppStyle::Get())
						.Justification(ETextJustify::Left)

						+ SRichTextBlock::HyperlinkDecorator(TEXT("FileHyperLink"), FSlateHyperlinkRun::FOnClick::CreateStatic(&OnFileLinkClicked))
					]
				]
			]
		];
}

TSharedRef<ITableRow> SStyleBrowser::HandleListGenerateRow(const FStyleBrowserItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SStyleBrowserTableRow, OwnerTable, Item.ToSharedRef(), SharedThis(this));
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FSlateColor SStyleBrowser::GetFilterStatusTextColor() const
{
	if (IsTextFilterActive() == false)
	{
		return FSlateColor::UseForeground();
	}
	if (Model->GetFilteredNum() == 0)
	{
		static FSlateColor AccentRedColor = FAppStyle::Get().GetSlateColor("Colors.AccentRed");
		return AccentRedColor;
	}

	static FSlateColor AccentGreenColor = FAppStyle::Get().GetSlateColor("Colors.AccentGreen");
	return AccentGreenColor;
}

void SStyleBrowser::HandleListSelectionChanged(const FStyleBrowserItemPtr InItem, ESelectInfo::Type SelectInfo) const
{
	if (!InItem.IsValid())
	{
		if (PreviewAreaHeaderText.IsValid())
		{
			PreviewAreaHeaderText->SetText(FText::GetEmpty());
		}
		if (PreviewAreaHeaderSnippedText.IsValid())
		{
			PreviewAreaHeaderSnippedText->SetText(FText::GetEmpty());
		}
		if (PreviewAreaBox.IsValid())
		{
			PreviewAreaBox->SetContent(SNullWidget::NullWidget);
		}
		if (PreviewAreaFooterText.IsValid())
		{
			PreviewAreaFooterText->SetText(FText::GetEmpty());
		}
		return;
	}

	if (PreviewAreaHeaderText.IsValid())
	{
		PreviewAreaHeaderText->SetText(FText::FromName(InItem->Name));
	}

	if (PreviewAreaHeaderSnippedText.IsValid())
	{
		PreviewAreaHeaderSnippedText->SetText(Model->GetFormattedCodeSnipped(InItem.ToSharedRef()));
	}

	if (PreviewAreaBox.IsValid())
	{
		PreviewAreaBox->SetContent(InItem->GetPreviewAreaWidget());
	}

	if (PreviewAreaFooterText.IsValid())
	{
		PreviewAreaFooterText->SetText(InItem->GetPreviewAreaText());
	}
}

FText SStyleBrowser::GetFilterStatusText() const
{
	if (IsTextFilterActive() == false)
	{
		return FText::Format(INVTEXT("{0} styles"), FText::AsNumber(Model->GetTotalNum()));
	}

	if (Model->GetFilteredNum() == 0)
	{
		return FText::Format(INVTEXT("No matching styles ({0} total)"), FText::AsNumber(Model->GetTotalNum()));
	}
	return FText::Format(INVTEXT("Filtered {0} from {1} styles"), FText::AsNumber(Model->GetFilteredNum()), FText::AsNumber(Model->GetTotalNum()));
}

void SStyleBrowser::Populate() const
{
	Model->UpdateContent();
	ListView->RequestListRefresh();
}

void SStyleBrowser::OnFilterTextChanged(const FText& InFilterText) const
{
	SearchBoxFilter->SetRawFilterText(InFilterText);
	FilterTextBoxWidget->SetError(SearchBoxFilter->GetFilterErrorText());
}

FReply SStyleBrowser::OnCopyStyleNameClicked() const
{
	if (PreviewAreaHeaderText.IsValid() && !PreviewAreaHeaderText->GetText().IsEmptyOrWhitespace())
	{
		FHazardToolsUtils::SetClipboardText(PreviewAreaHeaderText->GetText().ToString());
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SStyleBrowser::OnCopyCodeSnippedClicked() const
{
	if (ListView->GetNumItemsSelected() > 0 && ListView->GetSelectedItems()[0].IsValid())
	{
		if (const FString Snipped = Model->GetRawCodeSnipped(ListView->GetSelectedItems()[0].ToSharedRef()); !Snipped.IsEmpty())
		{
			FHazardToolsUtils::SetClipboardText(Snipped);
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

void SStyleBrowser::PopulateSearchStrings(const FStyleBrowserItemPtr& Item, TArray<FString>& OutSearchStrings) const
{
	OutSearchStrings.Emplace(Item->Name.ToString());
	Item->PopulateAdditionalSearchStrings(OutSearchStrings);
}

bool SStyleBrowser::ShouldItemPassTextFilter(const FStyleBrowserItemPtr& Item) const
{
	if (!SearchBoxFilter->PassesFilter(Item))
	{
		return false;
	}
	return true;
}

TAttribute<FText> SStyleBrowser::GetTextFilterHighlightText() const
{
	return TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([&]()
	{
		if (SearchBoxFilter.IsValid())
		{
			return SearchBoxFilter->GetRawFilterText();
		}
		return FText();
	}));
}
}
