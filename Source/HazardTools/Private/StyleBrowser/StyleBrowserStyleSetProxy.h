// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"
/**
 * 
 */
class FStyleBrowserStyleSetProxy : FSlateStyleSet
{
public:
	explicit FStyleBrowserStyleSetProxy(const FName& InStyleSetName)
		: FSlateStyleSet(InStyleSetName)
	{
	}

	static const TMap<FName, TSharedRef<FSlateWidgetStyle>>& GetWidgetStyleValues(const ISlateStyle& StyleSet);
	static const TMap<FName, float>& GetFloatValues(const ISlateStyle& StyleSet);
	static const TMap<FName, FVector2d>& GetVector2DValues(const ISlateStyle& StyleSet);
	static const TMap<FName, FLinearColor>& GetColorValues(const ISlateStyle& StyleSet);
	static const TMap<FName, FSlateColor>& GetSlateColorValues(const ISlateStyle& StyleSet);
	static const TMap<FName, FMargin>& GetMarginValues(const ISlateStyle& StyleSet);
	static const TMap<FName, FSlateBrush*>& GetBrushResources(const ISlateStyle& StyleSet);
	static const TMap<FName, FSlateFontInfo>& GetFontInfoResources(const ISlateStyle& StyleSet);
	static const TMap<FName, FSlateSound>& GetSounds(const ISlateStyle& StyleSet);

	static const FSlateStyleSet* GetParentStyleSet(const ISlateStyle& StyleSet);
};
