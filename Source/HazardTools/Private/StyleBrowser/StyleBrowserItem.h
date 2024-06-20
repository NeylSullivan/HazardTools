// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"

struct FSlateRoundedBoxBrush;
/**
 * 
 */
namespace HazardTools
{
struct FStyleBrowserItem : TSharedFromThis<FStyleBrowserItem>
{
	FStyleBrowserItem(const FName& Name, const FName& StyleSetName)
		: Name(Name)
		, StyleSetName(StyleSetName)
	{
	}

	virtual ~FStyleBrowserItem() = default;

	virtual TSharedRef<SWidget> GetPreviewColumnWidget() = 0;
	virtual TSharedRef<SWidget> GetPreviewAreaWidget();
	virtual FText GetPreviewAreaText() { return FText::GetEmpty(); };
	virtual FName GetTypeName() = 0;
	virtual FString GetCodeSnippet() { return FString(); }

	virtual void PopulateAdditionalSearchStrings(TArray<FString>& OutSearchStrings) const
	{
	};

	const FName Name;
	const FName StyleSetName;
	bool bAddedFromParentStyle = false;

	static const FText PreviewText;
};

struct FStyleBrowserItem_Float : FStyleBrowserItem
{
	const float FloatValue;
	static const FName TypeName;

	FStyleBrowserItem_Float(const FName& Name, const FName& StyleSetName, const float FloatValue)
		: FStyleBrowserItem(Name, StyleSetName)
		, FloatValue(FloatValue)
	{
	}

	virtual TSharedRef<SWidget> GetPreviewColumnWidget() override;
	virtual TSharedRef<SWidget> GetPreviewAreaWidget() override;
	virtual FName GetTypeName() override { return TypeName; }
	virtual FString GetCodeSnippet() override;
};

struct FStyleBrowserItem_Vector2D : FStyleBrowserItem
{
	const FVector2f Vector2dValue;
	static const FName TypeName;

	FStyleBrowserItem_Vector2D(const FName& Name, const FName& StyleSetName, const FVector2f Vector2dValue)
		: FStyleBrowserItem(Name, StyleSetName)
		, Vector2dValue(Vector2dValue)
	{
	}

	virtual TSharedRef<SWidget> GetPreviewColumnWidget() override;
	virtual TSharedRef<SWidget> GetPreviewAreaWidget() override;
	virtual FName GetTypeName() override { return TypeName; }
	virtual FString GetCodeSnippet() override;
};

struct FStyleBrowserItem_Color : FStyleBrowserItem
{
	const FLinearColor Color;
	static const FName TypeName;

	FStyleBrowserItem_Color(const FName& Name, const FName& StyleSetName, const FLinearColor& Color)
		: FStyleBrowserItem(Name, StyleSetName)
		, Color(Color)
	{
	}

	virtual TSharedRef<SWidget> GetPreviewColumnWidget() override;
	virtual TSharedRef<SWidget> GetPreviewAreaWidget() override;
	virtual FName GetTypeName() override { return TypeName; }
	virtual FString GetCodeSnippet() override;

private:
	TSharedPtr<FSlateRoundedBoxBrush> RoundedBoxBrush;
};

struct FStyleBrowserItem_SlateColor : FStyleBrowserItem
{
	const FSlateColor SlateColor;
	static const FName TypeName;

	FStyleBrowserItem_SlateColor(const FName& Name, const FName& StyleSetName, const FSlateColor& SlateColor)
		: FStyleBrowserItem(Name, StyleSetName)
		, SlateColor(SlateColor)
	{
	}

	virtual TSharedRef<SWidget> GetPreviewColumnWidget() override;
	virtual TSharedRef<SWidget> GetPreviewAreaWidget() override;
	virtual FText GetPreviewAreaText() override;
	virtual FName GetTypeName() override { return TypeName; }
	virtual FString GetCodeSnippet() override;

private:
	TSharedPtr<FSlateRoundedBoxBrush> RoundedBoxBrush;
};

struct FStyleBrowserItem_Margin : FStyleBrowserItem
{
	const FMargin Margin;
	static const FName TypeName;

	FStyleBrowserItem_Margin(const FName& Name, const FName& StyleSetName, const FMargin Margin)
		: FStyleBrowserItem(Name, StyleSetName)
		, Margin(Margin)
	{
	}

	virtual TSharedRef<SWidget> GetPreviewColumnWidget() override;
	virtual TSharedRef<SWidget> GetPreviewAreaWidget() override;
	virtual FName GetTypeName() override { return TypeName; }
	virtual FString GetCodeSnippet() override;
};

struct FStyleBrowserItem_Brush : FStyleBrowserItem
{
	const FSlateBrush* Brush;
	static const FName TypeName;

	FStyleBrowserItem_Brush(const FName& Name, const FName& StyleSetName, const FSlateBrush* Brush)
		: FStyleBrowserItem(Name, StyleSetName)
		, Brush(Brush)
	{
	}

	virtual TSharedRef<SWidget> GetPreviewColumnWidget() override;
	virtual TSharedRef<SWidget> GetPreviewAreaWidget() override;
	virtual FText GetPreviewAreaText() override;
	virtual FName GetTypeName() override { return TypeName; }
	virtual FString GetCodeSnippet() override;
};


struct FStyleBrowserItem_FontStyle : FStyleBrowserItem
{
	const FSlateFontInfo& FontInfo;
	static const FName TypeName;

	FStyleBrowserItem_FontStyle(const FName& Name, const FName& StyleSetName, const FSlateFontInfo& FontInfo)
		: FStyleBrowserItem(Name, StyleSetName)
		, FontInfo(FontInfo)
	{
	}

	virtual TSharedRef<SWidget> GetPreviewColumnWidget() override;
	virtual TSharedRef<SWidget> GetPreviewAreaWidget() override;
	virtual FText GetPreviewAreaText() override;
	virtual FName GetTypeName() override { return TypeName; }
	virtual FString GetCodeSnippet() override;
};

struct FStyleBrowserItem_Sound : FStyleBrowserItem
{
	const FSlateSound& Sound;
	static const FName TypeName;

	FStyleBrowserItem_Sound(const FName& Name, const FName& StyleSetName, const FSlateSound& Sound)
		: FStyleBrowserItem(Name, StyleSetName)
		, Sound(Sound)
	{
	}

	virtual TSharedRef<SWidget> GetPreviewColumnWidget() override;
	virtual TSharedRef<SWidget> GetPreviewAreaWidget() override;
	virtual FText GetPreviewAreaText() override;
	virtual FName GetTypeName() override { return TypeName; }
	virtual FString GetCodeSnippet() override;
};
}
