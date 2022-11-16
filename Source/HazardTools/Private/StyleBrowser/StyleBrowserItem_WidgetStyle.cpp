// Copyright Neyl Sullivan 2022


#include "StyleBrowserItem_WidgetStyle.h"
#include "SlateOptMacros.h"
#include "HazardToolsUtils.h"
#include "Styling/SegmentedControlStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/ToolBarStyle.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Input/SSegmentedControl.h"

namespace HazardTools
{
const FName FStyleBrowserItem_WidgetStyle::TypeName("WidgetStyle");

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

#pragma region Draw Style Helpers

TSharedRef<SWidget> DrawTextBlockStyle(const FStyleBrowserItem_WidgetStyle& Item)
{
	const auto* Style = static_cast<const FTextBlockStyle*>(&Item.Style.Get());
	return
		SNew(STextBlock)
		.Text(FStyleBrowserItem::PreviewText)
		.TextStyle(Style);
}

TSharedRef<SWidget> DrawEditableTextStyle(const FStyleBrowserItem_WidgetStyle& Item)
{
	const auto* Style = static_cast<const FEditableTextStyle*>(&Item.Style.Get());
	return
		SNew(SEditableText)
		.Text(FStyleBrowserItem::PreviewText)
		.Style(Style);
}

TSharedRef<SWidget> DrawEditableTextBoxStyle(const FStyleBrowserItem_WidgetStyle& Item)
{
	const auto* Style = static_cast<const FEditableTextBoxStyle*>(&Item.Style.Get());
	return
		SNew(SEditableTextBox)
		.Text(FStyleBrowserItem::PreviewText)
		.Style(Style);
}

TSharedRef<SWidget> DrawInlineEditableTextBlockStyle(const FStyleBrowserItem_WidgetStyle& Item)
{
	const auto* Style = static_cast<const FInlineEditableTextBlockStyle*>(&Item.Style.Get());
	return
		SNew(SInlineEditableTextBlock)
		.Text(FStyleBrowserItem::PreviewText)
		.Style(Style);
}

TSharedRef<SWidget> DrawSegmentedControlStyle(const FStyleBrowserItem_WidgetStyle& Item)
{
	const auto* Style = static_cast<const FSegmentedControlStyle*>(&Item.Style.Get());

	return
		SNew(SSegmentedControl<int32>)
		.Style(Style)
		.Value(0)
		+ SSegmentedControl<int32>::Slot(0).Text(INVTEXT("Option 1"))
		+ SSegmentedControl<int32>::Slot(1).Text(INVTEXT("Option 2"))
		+ SSegmentedControl<int32>::Slot(2).Text(INVTEXT("Option 3"))
		+ SSegmentedControl<int32>::Slot(3).Text(INVTEXT("Option 4"));
}

TSharedRef<SWidget> DrawToolBarStyle(const FStyleBrowserItem_WidgetStyle& Item)
{
	FSlimHorizontalToolBarBuilder Toolbar(nullptr, FMultiBoxCustomization::None);

	// Special case for toolbar styling // TODO update snipped generation
	Toolbar.SetStyle(FSlateStyleRegistry::FindSlateStyle(Item.StyleSetName), Item.Name);

	FUIAction ButtonAction;
	Toolbar.AddToolBarButton(ButtonAction, NAME_None, INVTEXT("Button"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.box-perspective"));

	static bool IsChecked = false;
	FUIAction ToggleButtonAction;
	ToggleButtonAction.GetActionCheckState.BindLambda([&] { return IsChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; });
	ToggleButtonAction.ExecuteAction.BindLambda([&] { IsChecked = !IsChecked; });

	Toolbar.AddToolBarButton(ToggleButtonAction,
		NAME_None,
		INVTEXT("Toggle"),
		FText::GetEmpty(),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.box-perspective"),
		EUserInterfaceActionType::ToggleButton);

	FUIAction ComboAction;
	Toolbar.AddComboButton(ComboAction,
		FOnGetContent::CreateLambda(
			[&]()
			{
				FMenuBuilder Menu(true, nullptr);
				Menu.AddMenuEntry(INVTEXT("Combo Menu Entry 1"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.box-perspective"), FUIAction());
				Menu.AddMenuEntry(INVTEXT("Combo Menu Entry 2"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.box-perspective"), FUIAction());
				Menu.AddMenuEntry(INVTEXT("Combo Menu Entry 3"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.box-perspective"), FUIAction());

				return Menu.MakeWidget();
			}),
		INVTEXT("Dropdown"),
		FText::GetEmpty(),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.box-perspective")
		);

	Toolbar.BeginSection("SettingsTest");
	{
		Toolbar.AddToolBarButton(ButtonAction, NAME_None, INVTEXT("With Settings"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.box-perspective"));

		Toolbar.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateLambda(
				[&]()
				{
					FMenuBuilder Menu(true, nullptr);
					Menu.AddMenuEntry(INVTEXT("Settings Menu Entry 1"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.box-perspective"), FUIAction());
					Menu.AddMenuEntry(INVTEXT("Settings Menu Entry 2"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.box-perspective"), FUIAction());
					Menu.AddMenuEntry(INVTEXT("Settings Menu Entry 3"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.box-perspective"), FUIAction());

					return Menu.MakeWidget();
				}),
			TAttribute<FText>(),
			TAttribute<FText>(),
			TAttribute<FSlateIcon>(),
			true
			);
	}
	Toolbar.EndSection();

	return Toolbar.MakeWidget();
}

TSharedRef<SWidget> DrawButtonStyle(const FStyleBrowserItem_WidgetStyle& Item)
{
	const auto* Style = static_cast<const FButtonStyle*>(&Item.Style.Get());

	if (Style->Normal.DrawAs == ESlateBrushDrawType::Image) // Draw without text, as icon only
	{
		return
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(Style)
			];
	}

	return SNew(SButton)
			.Text(INVTEXT("Preview Label"))
			.ButtonStyle(Style);
}

TSharedRef<SWidget> DrawComboButtonStyle(const FStyleBrowserItem_WidgetStyle& Item)
{
	const auto* Style = static_cast<const FComboButtonStyle*>(&Item.Style.Get());

	return
		SNew(SComboButton)
		.ComboButtonStyle(Style)
		.HasDownArrow(true)
		.OnGetMenuContent_Lambda([]()
		                  {
			                  FMenuBuilder Menu(true, nullptr);
			                  Menu.AddMenuEntry(INVTEXT("Menu Entry 1"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.box-perspective"), FUIAction());
			                  Menu.AddMenuEntry(INVTEXT("Menu Entry 2"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.box-perspective"), FUIAction());
			                  Menu.AddMenuEntry(INVTEXT("Menu Entry 3"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.box-perspective"), FUIAction());
			                  Menu.AddMenuEntry(INVTEXT("Menu Entry 4"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.box-perspective"), FUIAction());
			                  return Menu.MakeWidget();
		                  })
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(INVTEXT("Preview Label"))
		];
}

TSharedRef<SWidget> DrawComboBoxStyle(const FStyleBrowserItem_WidgetStyle& Item)
{
	const auto* Style = static_cast<const FComboBoxStyle*>(&Item.Style.Get());

	static TArray<TSharedPtr<FString>> ComboBoxSource = {
		MakeShared<FString>(TEXT("Option 1")),
		MakeShared<FString>(TEXT("Option 2")),
		MakeShared<FString>(TEXT("Option 3")),
		MakeShared<FString>(TEXT("Option 4"))};

	return
		SNew(SComboBox<TSharedPtr<FString>>)
		.ComboBoxStyle(Style)
		.OptionsSource(&ComboBoxSource)
		.OnGenerateWidget_Lambda([](const TSharedPtr<FString> SourceEntry)
		                                    {
			                                    return SNew(SBorder)
			                                           .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			                                           [
				                                           SNew(STextBlock)
				                                           .Text(FText::FromString(*SourceEntry.Get()))
			                                           ];
		                                    })
		.ContentPadding(0.0f)
		.InitiallySelectedItem(nullptr)
		[
			SNew(STextBlock)
			.Text(INVTEXT("Preview Label"))
		];
}

TSharedRef<SWidget> DrawSpinBoxStyle(const FStyleBrowserItem_WidgetStyle& Item)
{
	const auto* Style = static_cast<const FSpinBoxStyle*>(&Item.Style.Get());

	return
		SNew(SBox)
		.MinDesiredWidth(220)
		[
			SNew(SSpinBox<float>)
			.Style(Style)
			.MinValue(0.0f)
			.MaxValue(500.0f)
			.MinSliderValue(TAttribute<TOptional<float>>(-500.0f))
			.MaxSliderValue(TAttribute<TOptional<float>>(500.0f))
			.Value(123.0456789)
			.Delta(0.5f)
		];
}

TSharedRef<SWidget> DrawSliderStyle(const FStyleBrowserItem_WidgetStyle& Item)
{
	const auto* Style = static_cast<const FSliderStyle*>(&Item.Style.Get());

	return
		SNew(SBox)
		.MinDesiredWidth(220)
		[
			SNew(SSlider)
			.Style(Style)
			.Orientation(Orient_Horizontal)
			.Value(0.5f)
		];
}

TSharedRef<SWidget> DrawProgressBarStyle(const FStyleBrowserItem_WidgetStyle& Item)
{
	const auto* Style = static_cast<const FProgressBarStyle*>(&Item.Style.Get());

	return
		SNew(SBox)
		.MinDesiredWidth(220)
		[
			SNew(SProgressBar)
			.Style(Style)
			.Percent(.5f)
		];
}

TSharedRef<SWidget> DrawCheckBoxStyle(const FStyleBrowserItem_WidgetStyle& Item)
{
	const auto* Style = static_cast<const FCheckBoxStyle*>(&Item.Style.Get());

	TSharedRef<SCheckBox> CheckBox =
		SNew(SCheckBox)
		.Style(Style)
		.IsChecked(ECheckBoxState::Checked);

	if (Style->CheckedImage.DrawAs == ESlateBrushDrawType::Image)
	{
		CheckBox->SetContent(SNew(SBox)
		.MinDesiredWidth(Style->CheckedImage.ImageSize.X)
		.MinDesiredHeight(Style->CheckedImage.ImageSize.Y));
	}

	else if (Style->CheckBoxType == ESlateCheckBoxType::ToggleButton)
	{
		CheckBox->SetContent(SNew(STextBlock).Text(INVTEXT("Preview Label")));
	}

	return CheckBox;
}

TSharedRef<SWidget> DrawHyperlinkStyle(const FStyleBrowserItem_WidgetStyle& Item)
{
	const auto* Style = static_cast<const FHyperlinkStyle*>(&Item.Style.Get());

	return
		SNew(SHyperlink)
		.Text(INVTEXT("Hyperlink Text"))
		.Style(Style)
		.ToolTipText(INVTEXT("Hyperlink Tooltip Text"));
}

TSharedRef<SWidget> DrawSplitterStyle(const FStyleBrowserItem_WidgetStyle& Item)
{
	const auto* Style = static_cast<const FSplitterStyle*>(&Item.Style.Get());

	return
		SNew(SBorder)
		.Padding(8.f)
		.BorderImage(FAppStyle::Get().GetBrush("Checkerboard"))
		.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.05f))
		[
			SNew(SSplitter)
			.Style(Style)
			.Orientation(Orient_Horizontal)

			+ SSplitter::Slot()
			.Value(1.f)
			[
				SNew(SBorder)
				.Padding(FMargin(4))
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SBox)
					.MinDesiredHeight(128.f)
					.MinDesiredWidth(96.f)
				]
			]

			+ SSplitter::Slot()
			.Value(1.f)
			[
				SNew(SBorder)
				.Padding(FMargin(4.f))
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SBox)
					.MinDesiredHeight(128.f)
					.MinDesiredWidth(96.f)
				]
			]
		];
}

#pragma endregion Draw Style Helpers

TMap<FName, FWidgetVisualizerFunctionRef> FStyleBrowserItem_WidgetStyle::Visualizers = TMap<FName, FWidgetVisualizerFunctionRef>();
TMap<FName, FWeakScriptStructPtr> FStyleBrowserItem_WidgetStyle::WidgetStyleScriptStructsMap = TMap<FName, FWeakScriptStructPtr>();

FStyleBrowserItem_WidgetStyle::FStyleBrowserItem_WidgetStyle(const FName& Name, const FName& StyleSetName, const TSharedRef<FSlateWidgetStyle> Style)
	: FStyleBrowserItem(Name, StyleSetName)
	, Style(Style)
{
	if (Visualizers.IsEmpty())
	{
		Visualizers.Add(FTextBlockStyle::TypeName, DrawTextBlockStyle);
		Visualizers.Add(FEditableTextStyle::TypeName, DrawEditableTextStyle);
		Visualizers.Add(FEditableTextBoxStyle::TypeName, DrawEditableTextBoxStyle);
		Visualizers.Add(FButtonStyle::TypeName, DrawButtonStyle);
		Visualizers.Add(FComboButtonStyle::TypeName, DrawComboButtonStyle);
		Visualizers.Add(FComboBoxStyle::TypeName, DrawComboBoxStyle);
		Visualizers.Add(FCheckBoxStyle::TypeName, DrawCheckBoxStyle);
		Visualizers.Add(FHyperlinkStyle::TypeName, DrawHyperlinkStyle);
		Visualizers.Add(FSplitterStyle::TypeName, DrawSplitterStyle);

		Visualizers.Add(FSpinBoxStyle::TypeName, DrawSpinBoxStyle);
		Visualizers.Add(FSliderStyle::TypeName, DrawSliderStyle);
		Visualizers.Add(FProgressBarStyle::TypeName, DrawProgressBarStyle);
		Visualizers.Add(FInlineEditableTextBlockStyle::TypeName, DrawInlineEditableTextBlockStyle);
		Visualizers.Add(FSegmentedControlStyle::TypeName, DrawSegmentedControlStyle);
		Visualizers.Add(FToolBarStyle::TypeName, DrawToolBarStyle);
	}

	if (WidgetStyleScriptStructsMap.IsEmpty())
	{
		for (TObjectIterator<UScriptStruct> Itr; Itr; ++Itr)
		{
			if (UScriptStruct* ScriptStruct = *Itr; ScriptStruct->GetSuperStruct() && ScriptStruct->GetSuperStruct()->GetFName() == FSlateWidgetStyle::StaticStruct()->GetFName())
			{
				WidgetStyleScriptStructsMap.Add(FName(ScriptStruct->GetStructCPPName()), FWeakScriptStructPtr(ScriptStruct));
			}
		}
	}
}

TSharedRef<SWidget> FStyleBrowserItem_WidgetStyle::GetPreviewColumnWidget()
{
	FSlateColor TextColor = FSlateColor::UseForeground();
	FText Text = FText::FromName(Style->GetTypeName());

	if (Visualizers.Contains(Style->GetTypeName()) == false)
	{
		TextColor = FAppStyle::Get().GetSlateColor("Colors.Error");
		Text = FText::Format(INVTEXT("{0} (No preview)"), Text);
	}
	return
		SNew(STextBlock)
		.Text(Text)
		.ColorAndOpacity(TextColor);
}

TSharedRef<SWidget> FStyleBrowserItem_WidgetStyle::GetPreviewAreaWidget()
{
	if (const auto* VisualizerRefPtr = Visualizers.Find(Style->GetTypeName()))
	{
		const auto VisualizerRef = *VisualizerRefPtr;
		const TSharedRef<SWidget> VisualizerWidget = VisualizerRef(*this);

		return
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			  .HAlign(HAlign_Fill)
			  .VAlign(VAlign_Top)
			  .AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("DialogueWaveDetails.HeaderBorder"))
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					  .HAlign(HAlign_Fill)
					  .FillWidth(1.f)
					[
						SNew(SSpacer)
					]

					+ SHorizontalBox::Slot()
					  .HAlign(HAlign_Left)
					  .Padding(2.f)
					  .AutoWidth()
					[
						SNew(SCheckBox)
						.Style(&FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("ToggleButtonCheckbox"))
						.IsChecked_Lambda([VisualizerWidget] { return VisualizerWidget->IsEnabled() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([VisualizerWidget](const ECheckBoxState NewState) { VisualizerWidget->SetEnabled(NewState == ECheckBoxState::Checked); })
						[
							SNew(STextBlock)
							.Font(FAppStyle::Get().GetFontStyle("NormalFont"))
							.ColorAndOpacity(FAppStyle::Get().GetSlateColor("Colors.White50"))
							.Text(INVTEXT("Enabled"))
						]
					]
				]
			]

			+ SVerticalBox::Slot()
			  .Padding(8.f)
			  .HAlign(HAlign_Center)
			  .VAlign(VAlign_Center)
			  .FillHeight(1.f)
			[
				VisualizerWidget
			];
	}

	return FStyleBrowserItem::GetPreviewAreaWidget(); // Default - NO PREVIEW AVAILABLE
}

FText FStyleBrowserItem_WidgetStyle::GetPreviewAreaText()
{
	if (const auto* ScriptStructPtr = WidgetStyleScriptStructsMap.Find(Style->GetTypeName()))
	{
		if (const UScriptStruct* ScriptStruct = ScriptStructPtr->Get())
		{
			if (FText StyleInfo; FHazardToolsUtils::UStructToText(ScriptStruct, &Style.Get(), StyleInfo))
			{
				return StyleInfo;
			}
		}
	}

	return FStyleBrowserItem::GetPreviewAreaText(); // Default - empty text
}

FString FStyleBrowserItem_WidgetStyle::GetCodeSnippet()
{
	return FString::Printf(TEXT("GetWidgetStyle<%s>(\"%s\")"), *Style->GetTypeName().ToString(), *Name.ToString());
}

void FStyleBrowserItem_WidgetStyle::PopulateAdditionalSearchStrings(TArray<FString>& OutSearchStrings) const
{
	OutSearchStrings.Emplace(Style->GetTypeName().ToString());
	if (Visualizers.Contains(Style->GetTypeName()) == false)
	{
		OutSearchStrings.Emplace("NO PREVIEW AVAILABLE");
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
}
