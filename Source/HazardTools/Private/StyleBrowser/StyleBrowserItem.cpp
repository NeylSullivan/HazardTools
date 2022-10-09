// Copyright Neyl Sullivan 2022


#include "StyleBrowserItem.h"
#include "SlateOptMacros.h"
#include "HazardToolsUtils.h"
#include "IDetailTreeNode.h"
#include "StyleBrowserFontAwesomeGlyphs.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformWrapPanel.h"


namespace HazardTools
{
const FText FStyleBrowserItem::PreviewText = INVTEXT("The quick brown fox jumps over the lazy dog.\n\nTHE QUICK BROWN FOX JUMPS OVER THE LAZY DOG.");

const FName FStyleBrowserItem_Float::TypeName("Float");
const FName FStyleBrowserItem_Vector2D::TypeName("Vector2D");
const FName FStyleBrowserItem_Color::TypeName("Color");
const FName FStyleBrowserItem_SlateColor::TypeName("SlateColor");
const FName FStyleBrowserItem_Margin::TypeName("Margin");
const FName FStyleBrowserItem_Brush::TypeName("Brush");
const FName FStyleBrowserItem_FontStyle::TypeName("FontStyle");
const FName FStyleBrowserItem_Sound::TypeName("Sound");


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

UE_NODISCARD TSharedRef<SWidget> MakeColorPreviewAreaWidget(const TSharedPtr<FSlateRoundedBoxBrush> RoundedBoxBrush, const FLinearColor LinearColor)
{
	const FSlateColor FontColor = FAppStyle::Get().GetSlateColor(LinearColor.GetLuminance() > 0.5f ? "Colors.Background" : "Colors.White");

	return
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.Padding(8)
			.WidthOverride(120)
			.HeightOverride(100)
			[
				SNew(SOverlay)

				+ SOverlay::Slot()
				[
					SNew(SBorder)
					.BorderImage(RoundedBoxBrush.Get())
				]

				+ SOverlay::Slot()
				.Padding(12)
				[
					SNew(SBox)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Font(FAppStyle::Get().GetFontStyle("SmallFont"))
						.ColorAndOpacity(FontColor)
						.Text(FText::FromString(LinearColor.ToFColor(true).ToHex()))
					]
				]
			]
		];
}

UE_NODISCARD FString MakeNiceFloatString(const double InFloat)
{
	const FString String = FString::SanitizeFloat(InFloat, 0);
	if (String == TEXT("0"))
	{
		return TEXT(".0f");
	}
	if (static const FString Delimiter(TEXT(".")); String.Contains(Delimiter))
	{
		return String + TEXT("f");
	}
	return String + TEXT(".f");
}

TSharedRef<SWidget> FStyleBrowserItem::GetPreviewAreaWidget()
{
	return
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("RoundedError"))
			.ColorAndOpacity(FAppStyle::Get().GetBrush("RoundedError")->OutlineSettings.Color.GetSpecifiedColor())
			.Padding(24.f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				  .HAlign(HAlign_Fill)
				  .VAlign(VAlign_Center)
				  .FillWidth(1.f)
				[
					SNew(STextBlock)
					.Text(INVTEXT("NO PREVIEW AVAILABLE"))
					.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("ContentBrowser.TopBar.Font"))
				]

				+ SHorizontalBox::Slot()
				  .HAlign(HAlign_Right)
				  .VAlign(VAlign_Center)
				  .AutoWidth()
				  .Padding(24.f, 0.f, 0.f, 0.f)
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Warning.Large"))
				]
			]
		];
}

#pragma region FStyleBrowserItem_Float


TSharedRef<SWidget> FStyleBrowserItem_Float::GetPreviewColumnWidget()
{
	return
		SNew(STextBlock)
		.Text(FText::FromString(MakeNiceFloatString(FloatValue)));
}

TSharedRef<SWidget> FStyleBrowserItem_Float::GetPreviewAreaWidget()
{
	return
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
				.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("ProjectBrowser.VersionOverlayText"))
				.ColorAndOpacity(FAppStyle::Get().GetSlateColor("EditorViewport.ActiveBorderColor"))
				.Text(FText::FromString(MakeNiceFloatString(FloatValue)))
		];
}

FString FStyleBrowserItem_Float::GetCodeSnippet()
{
	return FString::Printf(TEXT("GetFloat(\"%s\")"), *Name.ToString());
}
#pragma endregion FStyleBrowserItem_Float

#pragma region FStyleBrowserItem_Vector2D

TSharedRef<SWidget> FStyleBrowserItem_Vector2D::GetPreviewColumnWidget()
{
	const FString X = MakeNiceFloatString(Vector2dValue.X);
	const FString Y = MakeNiceFloatString(Vector2dValue.Y);
	return
		SNew(STextBlock)
		.Text(FText::Format(INVTEXT("FVector2d ({0}, {1})"), FText::FromString(X), FText::FromString(Y)));
}

TSharedRef<SWidget> FStyleBrowserItem_Vector2D::GetPreviewAreaWidget()
{
	const FString X = MakeNiceFloatString(Vector2dValue.X);
	const FString Y = MakeNiceFloatString(Vector2dValue.Y);
	return
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("ProjectBrowser.VersionOverlayText"))
			.ColorAndOpacity(FAppStyle::Get().GetSlateColor("EditorViewport.ActiveBorderColor"))
			.Text(FText::Format(INVTEXT("FVector2d ({0}, {1})"), FText::FromString(X), FText::FromString(Y)))
		];
}

FString FStyleBrowserItem_Vector2D::GetCodeSnippet()
{
	return FString::Printf(TEXT("GetVector(\"%s\")"), *Name.ToString());
}
#pragma endregion FStyleBrowserItem_Vector2D

#pragma region FStyleBrowserItem_Color

TSharedRef<SWidget> FStyleBrowserItem_Color::GetPreviewColumnWidget()
{
	return
		SNew(SImage)
		.Image(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.ColorAndOpacity(FSlateColor(Color));
}

TSharedRef<SWidget> FStyleBrowserItem_Color::GetPreviewAreaWidget()
{
	if (!RoundedBoxBrush.IsValid())
	{
		RoundedBoxBrush = MakeShared<FSlateRoundedBoxBrush>(FSlateColor(Color), 6.0f);
	}

	return MakeColorPreviewAreaWidget(RoundedBoxBrush, Color);
}

FString FStyleBrowserItem_Color::GetCodeSnippet()
{
	return FString::Printf(TEXT("GetColor(\"%s\")"), *Name.ToString());
}
#pragma endregion FStyleBrowserItem_Color

#pragma region FStyleBrowserItem_SlateColor

TSharedRef<SWidget> FStyleBrowserItem_SlateColor::GetPreviewColumnWidget()
{
	return
		SNew(SImage)
		.Image(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		.ColorAndOpacity(SlateColor);
}

TSharedRef<SWidget> FStyleBrowserItem_SlateColor::GetPreviewAreaWidget()
{
	if (!RoundedBoxBrush.IsValid())
	{
		RoundedBoxBrush = MakeShared<FSlateRoundedBoxBrush>(SlateColor, 6.0f);
	}

	return MakeColorPreviewAreaWidget(RoundedBoxBrush, SlateColor.GetSpecifiedColor());
}

FText FStyleBrowserItem_SlateColor::GetPreviewAreaText()
{
	if (FText BrushInfoText; FHazardToolsUtils::UStructToText(FSlateColor::StaticStruct(), &SlateColor, BrushInfoText))
	{
		return BrushInfoText;
	}
	return FText::GetEmpty();
}

FString FStyleBrowserItem_SlateColor::GetCodeSnippet()
{
	return FString::Printf(TEXT("GetSlateColor(\"%s\")"), *Name.ToString());
}
#pragma endregion FStyleBrowserItem_SlateColor

#pragma region FStyleBrowserItem_Margin

UE_NODISCARD FString MakeMarginString(const FMargin& Margin)
{
	const FString Left = MakeNiceFloatString(Margin.Left);
	const FString Top = MakeNiceFloatString(Margin.Top);
	const FString Right = MakeNiceFloatString(Margin.Right);
	const FString Bottom = MakeNiceFloatString(Margin.Bottom);

	FStringFormatNamedArguments Args;
	Args.Add(TEXT("Left"), Left);
	Args.Add(TEXT("Top"), Top);
	Args.Add(TEXT("Right"), Right);
	Args.Add(TEXT("Bottom"), Bottom);

	if (Left == Right && Top == Bottom)
	{
		if (Left == Top) // uniform
		{
			return FString::Format(TEXT("FMargin ({Top})"), Args);
		}
		// Horizontal and Vertical
		return FString::Format(TEXT("FMargin ({Left}, {Top})"), Args);
	}
	return FString::Format(TEXT("FMargin ({Left}, {Top}, {Right}, {Bottom})"), Args);
}

TSharedRef<SWidget> FStyleBrowserItem_Margin::GetPreviewColumnWidget()
{
	return SNew(STextBlock)
	       .Text(FText::FromString(MakeMarginString(Margin)));
}

TSharedRef<SWidget> FStyleBrowserItem_Margin::GetPreviewAreaWidget()
{
	return
		SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
				.TextStyle(&FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("ProjectBrowser.VersionOverlayText"))
				.ColorAndOpacity(FAppStyle::Get().GetSlateColor("EditorViewport.ActiveBorderColor"))
				.Text(FText::FromString(MakeMarginString(Margin)))
		];
}

FString FStyleBrowserItem_Margin::GetCodeSnippet()
{
	return FString::Printf(TEXT("GetMargin(\"%s\")"), *Name.ToString());
}
#pragma endregion FStyleBrowserItem_Margin

#pragma region FStyleBrowserItem_Brush

TSharedRef<SWidget> FStyleBrowserItem_Brush::GetPreviewColumnWidget()
{
	if (Brush->DrawAs == ESlateBrushDrawType::Border || Brush->DrawAs == ESlateBrushDrawType::Box)
	{
		return
			SNew(SBox)
			.HeightOverride(24.f)
			[
				SNew(SImage)
				.Image(Brush)
			];
	}

	constexpr float DefaultHeightWithoutPadding = 24.f - 2.f - 2.f;
	float WidthOverride = DefaultHeightWithoutPadding;
	if (Brush && Brush->DrawAs == ESlateBrushDrawType::Image)
	{
		if (const FVector2D Size = Brush->ImageSize; Size.X > 0 && Size.Y > 0)
		{
			WidthOverride = (Size.X * DefaultHeightWithoutPadding / Size.Y);
		}
	}

	return
		SNew(SBox)
		.HeightOverride(24.f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			  .AutoWidth()
			  .Padding(2.f)
			[
				SNew(SBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				.WidthOverride(WidthOverride)
				[
					SNew(SImage)
					.Image(Brush)
				]
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SSpacer)
				.Size(FVector2D(1, 1))
			]
		];
}

TSharedRef<SWidget> FStyleBrowserItem_Brush::GetPreviewAreaWidget()
{
	EHorizontalAlignment HorizontalAlignment = HAlign_Center;
	EVerticalAlignment VerticalAlignment = VAlign_Center;
	FOptionalSize AspectRatio;
	if (Brush->DrawAs == ESlateBrushDrawType::Border || Brush->DrawAs == ESlateBrushDrawType::Box || Brush->DrawAs == ESlateBrushDrawType::RoundedBox)
	{
		HorizontalAlignment = HAlign_Fill;
		VerticalAlignment = VAlign_Fill;
	}
	else if (Brush->DrawAs == ESlateBrushDrawType::Image)
	{
		if (Brush->Tiling == ESlateBrushTileType::Horizontal || Brush->Tiling == ESlateBrushTileType::Both)
		{
			HorizontalAlignment = HAlign_Fill;
		}
		if (Brush->Tiling == ESlateBrushTileType::Vertical || Brush->Tiling == ESlateBrushTileType::Both)
		{
			VerticalAlignment = VAlign_Fill;
		}
		const bool bAnyTiling = HorizontalAlignment == HAlign_Fill || VerticalAlignment == VAlign_Fill;
		if (const FVector2D Size = Brush->ImageSize; !bAnyTiling && Size.X > 0 && Size.Y > 0)
		{
			AspectRatio = Size.X / Size.Y;
		}
	}
	return
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.MinDesiredWidth(196.f)
			.MinDesiredHeight(196.f)
			.HAlign(HorizontalAlignment)
			.VAlign(VerticalAlignment)
			.MinAspectRatio(AspectRatio)
			.MaxAspectRatio(AspectRatio)
			.Padding(FMargin(4.f))
			[
				SNew(SImage)
				.Image(Brush)
			]
		];
}

FText FStyleBrowserItem_Brush::GetPreviewAreaText()
{
	if (FText BrushInfoText; FHazardToolsUtils::UStructToText(FSlateBrush::StaticStruct(), Brush, BrushInfoText))
	{
		return BrushInfoText;
	}

	return FText::GetEmpty();
}

FString FStyleBrowserItem_Brush::GetCodeSnippet()
{
	return FString::Printf(TEXT("GetBrush(\"%s\")"), *Name.ToString());
}
#pragma endregion FStyleBrowserItem_Brush

#pragma region FStyleBrowserItem_FontStyle
TSharedRef<SWidget> FStyleBrowserItem_FontStyle::GetPreviewColumnWidget()
{
	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> GetFontAwesomePreviewAreaWidget(const FSlateFontInfo& FontInfo)
{
	const TSharedRef<SUniformWrapPanel> WrapPanel =
		SNew(SUniformWrapPanel)
		.SlotPadding(4.f)
		.EvenRowDistribution(true);

	for (const TTuple<FString, FText>& Element : FStyleBrowserFontAwesomeGlyphs::EditorGlyphsMap)
	{
		WrapPanel->AddSlot()
		[
			SNew(SButton)
			.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("HoverHintOnly"))
			.OnClicked_Lambda([Element]()
			{
				FHazardToolsUtils::SetClipboardText(Element.Key);
				return FReply::Handled();
			})
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.ToolTipText(FText::FromString(Element.Key))
			.ContentPadding(4.f)
			[
				SNew(STextBlock)
				.Text(Element.Value)
				.Font(FontInfo)
			]
		];
	}
	return
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			WrapPanel
		];
}

TSharedRef<SWidget> FStyleBrowserItem_FontStyle::GetPreviewAreaWidget()
{
	// Special case to display editor glyphs from Font Awesome
	if (Name.ToString().StartsWith(TEXT("FontAwesome")))
	{
		return GetFontAwesomePreviewAreaWidget(FontInfo);
	}

	return
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(PreviewText)
			.Font(FontInfo)
		];
}

FText FStyleBrowserItem_FontStyle::GetPreviewAreaText()
{
	if (FText FontInfoText; FHazardToolsUtils::UStructToText(FSlateFontInfo::StaticStruct(), &FontInfo, FontInfoText))
	{
		return FontInfoText;
	}

	return FStyleBrowserItem::GetPreviewAreaText();
}

FString FStyleBrowserItem_FontStyle::GetCodeSnippet()
{
	return FString::Printf(TEXT("GetFontStyle(\"%s\")"), *Name.ToString());
}
#pragma endregion FStyleBrowserItem_FontStyle

#pragma region FStyleBrowserItem_Sound

TSharedRef<SWidget> FStyleBrowserItem_Sound::GetPreviewColumnWidget()
{
	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> FStyleBrowserItem_Sound::GetPreviewAreaWidget()
{
	return SNullWidget::NullWidget;
}

FText FStyleBrowserItem_Sound::GetPreviewAreaText()
{
	if (FText SoundInfoText; FHazardToolsUtils::UStructToText(FSlateSound::StaticStruct(), &Sound, SoundInfoText))
	{
		return SoundInfoText;
	}

	return FStyleBrowserItem::GetPreviewAreaText();
}

FString FStyleBrowserItem_Sound::GetCodeSnippet()
{
	return FString::Printf(TEXT("GetSound(\"%s\")"), *Name.ToString());
}
#pragma endregion FStyleBrowserItem_Sound

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
};
