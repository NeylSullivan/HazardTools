// Copyright Neyl Sullivan 2022


#include "StyleBrowserStyleSetProxy.h"

const TMap<FName, TSharedRef<FSlateWidgetStyle>>& FStyleBrowserStyleSetProxy::GetWidgetStyleValues(const ISlateStyle& StyleSet)
{
	// ReSharper disable once CppRedundantCastExpression
	return reinterpret_cast<const FStyleBrowserStyleSetProxy&>(StyleSet).WidgetStyleValues; // Ugly hack
}

const TMap<FName, float>& FStyleBrowserStyleSetProxy::GetFloatValues(const ISlateStyle& StyleSet)
{
	// ReSharper disable once CppRedundantCastExpression
	return reinterpret_cast<const FStyleBrowserStyleSetProxy&>(StyleSet).FloatValues; // Ugly hack
}

const TMap<FName, FVector2d>& FStyleBrowserStyleSetProxy::GetVector2DValues(const ISlateStyle& StyleSet)
{
	// ReSharper disable once CppRedundantCastExpression
	return reinterpret_cast<const FStyleBrowserStyleSetProxy&>(StyleSet).Vector2DValues; // Ugly hack
}

const TMap<FName, FLinearColor>& FStyleBrowserStyleSetProxy::GetColorValues(const ISlateStyle& StyleSet)
{
	// ReSharper disable once CppRedundantCastExpression
	return reinterpret_cast<const FStyleBrowserStyleSetProxy&>(StyleSet).ColorValues; // Ugly hack
}

const TMap<FName, FSlateColor>& FStyleBrowserStyleSetProxy::GetSlateColorValues(const ISlateStyle& StyleSet)
{
	// ReSharper disable once CppRedundantCastExpression
	return reinterpret_cast<const FStyleBrowserStyleSetProxy&>(StyleSet).SlateColorValues; // Ugly hack
}

const TMap<FName, FMargin>& FStyleBrowserStyleSetProxy::GetMarginValues(const ISlateStyle& StyleSet)
{
	// ReSharper disable once CppRedundantCastExpression
	return reinterpret_cast<const FStyleBrowserStyleSetProxy&>(StyleSet).MarginValues; // Ugly hack
}

const TMap<FName, FSlateBrush*>& FStyleBrowserStyleSetProxy::GetBrushResources(const ISlateStyle& StyleSet)
{
	// ReSharper disable once CppRedundantCastExpression
	return reinterpret_cast<const FStyleBrowserStyleSetProxy&>(StyleSet).BrushResources; // Ugly hack
}

const TMap<FName, FSlateFontInfo>& FStyleBrowserStyleSetProxy::GetFontInfoResources(const ISlateStyle& StyleSet)
{
	// ReSharper disable once CppRedundantCastExpression
	return reinterpret_cast<const FStyleBrowserStyleSetProxy&>(StyleSet).FontInfoResources; // Ugly hack
}

const TMap<FName, FSlateSound>& FStyleBrowserStyleSetProxy::GetSounds(const ISlateStyle& StyleSet)
{
	// ReSharper disable once CppRedundantCastExpression
	return reinterpret_cast<const FStyleBrowserStyleSetProxy&>(StyleSet).Sounds; // Ugly hack
}

const FSlateStyleSet* FStyleBrowserStyleSetProxy::GetParentStyleSet(const ISlateStyle& StyleSet)
{
	if (const ISlateStyle* ParentStyle = reinterpret_cast<const FStyleBrowserStyleSetProxy&>(StyleSet).GetParentStyle())
	{
		return reinterpret_cast<const FSlateStyleSet*>(ParentStyle);
	}
	return nullptr;
}
