// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"
#include "StyleBrowserItem.h"

namespace HazardTools
{
using FWidgetVisualizerFunctionRef = TFunctionRef<TSharedRef<SWidget> (const struct FStyleBrowserItem_WidgetStyle&)>;
using FWeakScriptStructPtr = TWeakObjectPtr<UScriptStruct>;

struct FStyleBrowserItem_WidgetStyle : FStyleBrowserItem
{
	const TSharedRef<FSlateWidgetStyle> Style;
	static const FName TypeName;

	FStyleBrowserItem_WidgetStyle(const FName& Name, const FName& StyleSetName, const TSharedRef<FSlateWidgetStyle> Style);

	virtual TSharedRef<SWidget> GetPreviewColumnWidget() override;
	virtual TSharedRef<SWidget> GetPreviewAreaWidget() override;
	virtual FName GetTypeName() override { return TypeName; }
	virtual FText GetPreviewAreaText() override;
	virtual FString GetCodeSnippet() override;
	virtual void PopulateAdditionalSearchStrings(TArray<FString>& OutSearchStrings) const override;
private:
	static TMap<FName, FWidgetVisualizerFunctionRef> Visualizers;
	static TMap<FName, FWeakScriptStructPtr> WidgetStyleScriptStructsMap;
};
}
