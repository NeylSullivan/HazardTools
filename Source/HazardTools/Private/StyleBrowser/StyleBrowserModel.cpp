// Copyright Neyl Sullivan 2022


#include "StyleBrowserModel.h"

#include "EditorStyleSet.h"
#include "HazardToolsUtils.h"
#include "StyleBrowserStyleSetProxy.h"
#include "StyleBrowserItem.h"
#include "StyleBrowserItem_WidgetStyle.h"
#include "Styling/SlateStyleRegistry.h"

namespace HazardTools
{
FStyleBrowserModel::FStyleBrowserModel()
{
	FSlateStyleRegistry::IterateAllStyles(
		[&](const ISlateStyle& Style)
		{
			AvailableStyleSets.Add(Style.GetStyleSetName());
			return true; // Continue iteration
		}
		);

	AvailableStyleSets.Sort([](const FName& A, const FName& B)
	{
		return A.ToString() < B.ToString();
	});

	DesiredStyleSetName = FAppStyle::Get().GetStyleSetName();

	AvailableStyleTypeCategories.Emplace(FStyleBrowserItem_Float::TypeName);
	AvailableStyleTypeCategories.Emplace(FStyleBrowserItem_Vector2D::TypeName);
	AvailableStyleTypeCategories.Emplace(FStyleBrowserItem_Color::TypeName);
	AvailableStyleTypeCategories.Emplace(FStyleBrowserItem_SlateColor::TypeName);
	AvailableStyleTypeCategories.Emplace(FStyleBrowserItem_Margin::TypeName);
	AvailableStyleTypeCategories.Emplace(FStyleBrowserItem_Brush::TypeName);
	AvailableStyleTypeCategories.Emplace(FStyleBrowserItem_WidgetStyle::TypeName);
	AvailableStyleTypeCategories.Emplace(FStyleBrowserItem_FontStyle::TypeName);
	AvailableStyleTypeCategories.Emplace(FStyleBrowserItem_Sound::TypeName);
}

template <typename StyleBrowserItemType, typename StyleValueType>
void FStyleBrowserModel::Fill(TFunctionRef<const TMap<FName, StyleValueType>&(const ISlateStyle&)> Function, const ISlateStyle* ProcessedStyle /*= nullptr*/)
{
	static_assert(TIsDerivedFrom<StyleBrowserItemType, FStyleBrowserItem>::IsDerived, "StyleBrowserItemType should be derived from FStyleBrowserItem only");

	if (!StyleTypeNameToDisplay.IsNone() && StyleTypeNameToDisplay != StyleBrowserItemType::TypeName)
	{
		return;
	}

	const ISlateStyle& ActualStyle = ProcessedStyle != nullptr ? *ProcessedStyle : *CurrentStyleSet;
	// Set (and display) style set name only for items from PARENT style, NOT CURRENT
	//const FName StyleSetName = ProcessedStyle != nullptr ? ActualStyle.GetStyleSetName() : NAME_None;
	const TMap<FName, StyleValueType>& Values = Function(ActualStyle);
	for (const TTuple<FName, StyleValueType>& Pair : Values)
	{
		FStyleBrowserItemRef StyleBrowserItem = MakeShared<StyleBrowserItemType>(Pair.Key, ActualStyle.GetStyleSetName(), Pair.Value);
		StyleBrowserItem->bAddedFromParentStyle = ProcessedStyle != nullptr;
		/*if (ShouldItemPassTextFilterDelegate.IsBound() && ShouldItemPassTextFilterDelegate.Execute(StyleBrowserItem.Get()) == false)
		{
			continue;;
		}*/
		Content.Add(StyleBrowserItem);
	}

	if (bShouldAddFromParentStyleSets)
	{
		if (const FSlateStyleSet* ParentStyleSet = FStyleBrowserStyleSetProxy::GetParentStyleSet(ActualStyle))
		{
			Fill<StyleBrowserItemType, StyleValueType>(Function, ParentStyleSet);
		}
	}
}

void FStyleBrowserModel::SetDesiredStyleSetName(const FName InDesiredStyleSetName)
{
	if (InDesiredStyleSetName != DesiredStyleSetName)
	{
		DesiredStyleSetName = InDesiredStyleSetName;
		bPerformFullRebuild = true;
	}
}

FString FStyleBrowserModel::GetStyleSetsHierarchy() const
{
	TArray<const ISlateStyle*> Hierarchy;
	Hierarchy.Add(GetCurrentStyleSetPtr());

	while (const ISlateStyle* ParentStyle = FStyleBrowserStyleSetProxy::GetParentStyleSet(*Hierarchy.Last()))
	{
		Hierarchy.Add(ParentStyle);
	}

	if (Hierarchy.Num() < 2)
	{
		return FString(); // No hierarchy exist - just return empty
	}

	FString Result = Hierarchy[0]->GetStyleSetName().ToString();

	for (int i = 1; i < Hierarchy.Num(); ++i)
	{
		Result += FString::Printf(TEXT(" <- %s"), *Hierarchy[i]->GetStyleSetName().ToString());
	}
	return Result;
}

bool FStyleBrowserModel::HasParentStyleSet() const
{
	return FStyleBrowserStyleSetProxy::GetParentStyleSet(*GetCurrentStyleSetPtr()) != nullptr;
}

void FStyleBrowserModel::SetStyleTypeNameToDisplay(const FName InStyleTypeNameToDisplay)
{
	if (InStyleTypeNameToDisplay != StyleTypeNameToDisplay)
	{
		StyleTypeNameToDisplay = InStyleTypeNameToDisplay;
		bPerformFullRebuild = true;
	}
}

const ISlateStyle* FStyleBrowserModel::GetCurrentStyleSetPtr() const
{
	const ISlateStyle* FoundStylePtr = FSlateStyleRegistry::FindSlateStyle(DesiredStyleSetName);
	return FoundStylePtr != nullptr ? FoundStylePtr : &FAppStyle::Get();
}


void FStyleBrowserModel::UpdateContent()
{
	if (bPerformFullRebuild)
	{
		Content.Reset();

		CurrentStyleSet = GetCurrentStyleSetPtr();

		check(CurrentStyleSet)

		Fill<FStyleBrowserItem_Float, float>(FStyleBrowserStyleSetProxy::GetFloatValues);
		Fill<FStyleBrowserItem_Vector2D, FVector2f>(FStyleBrowserStyleSetProxy::GetVector2DValues);
		Fill<FStyleBrowserItem_Margin, FMargin>(FStyleBrowserStyleSetProxy::GetMarginValues);
		Fill<FStyleBrowserItem_Color, FLinearColor>(FStyleBrowserStyleSetProxy::GetColorValues);
		Fill<FStyleBrowserItem_SlateColor, FSlateColor>(FStyleBrowserStyleSetProxy::GetSlateColorValues);
		Fill<FStyleBrowserItem_Brush, FSlateBrush*>(FStyleBrowserStyleSetProxy::GetBrushResources);
		Fill<FStyleBrowserItem_WidgetStyle, TSharedRef<FSlateWidgetStyle>>(FStyleBrowserStyleSetProxy::GetWidgetStyleValues);
		Fill<FStyleBrowserItem_FontStyle, FSlateFontInfo>(FStyleBrowserStyleSetProxy::GetFontInfoResources);
		Fill<FStyleBrowserItem_Sound, FSlateSound>(FStyleBrowserStyleSetProxy::GetSounds);
	}

	FilteredContent.Reset();

	for (const FStyleBrowserItemPtr ItemPtr : Content)
	{
		if (ShouldItemPassTextFilterDelegate.IsBound() && ShouldItemPassTextFilterDelegate.Execute(ItemPtr))
		{
			FilteredContent.Add(ItemPtr);
		}
	}
}

FText FStyleBrowserModel::GetFormattedCodeSnipped(const FStyleBrowserItemRef InItem) const
{
	FString ItemCodeSnipped = InItem->GetCodeSnippet();

	ItemCodeSnipped = ItemCodeSnipped.Replace(
		*FString::Printf(TEXT("\"%s\""), *InItem->Name.ToString()),
		*FString::Printf(TEXT("<SyntaxHighlight.NodeAttributeValue>\"%s\"</>"), *InItem->Name.ToString()));

	if (InItem->GetTypeName() == FStyleBrowserItem_WidgetStyle::TypeName)
	{
		const TSharedRef<FStyleBrowserItem_WidgetStyle> WidgetStyleItem = StaticCastSharedRef<FStyleBrowserItem_WidgetStyle>(InItem);

		ItemCodeSnipped = ItemCodeSnipped.Replace(
			*FString::Printf(TEXT("<%s>"), *WidgetStyleItem->Style->GetTypeName().ToString()),
			*FString::Printf(TEXT("<<SyntaxHighlight.Node>%s</>>"), *WidgetStyleItem->Style->GetTypeName().ToString()));
	}

	return FText::FromString(GetStyleSetCodeSnippedPart(InItem, true) + ItemCodeSnipped);
}

FString FStyleBrowserModel::GetRawCodeSnipped(const FStyleBrowserItemRef InItem)
{
	return GetStyleSetCodeSnippedPart(InItem, false) + InItem->GetCodeSnippet();
}

FString FStyleBrowserModel::GetStyleSetCodeSnippedPart(const FStyleBrowserItemRef InItem, const bool bFormatForSyntaxHighlighting)
{
	const FName ItemStyleSetName = InItem->StyleSetName;
	check(ItemStyleSetName.IsNone() == false)

	const FString& Prefix_Type = bFormatForSyntaxHighlighting ? FHazardToolsUtils::Tag_Prefix_Type : FString();
	const FString& Suffix_Type = bFormatForSyntaxHighlighting ? FHazardToolsUtils::Tag_Suffix : FString();

	const FString& Prefix_Literal = bFormatForSyntaxHighlighting ? FHazardToolsUtils::Tag_Prefix_Literal : FString();
	const FString& Suffix_Literal = bFormatForSyntaxHighlighting ? FHazardToolsUtils::Tag_Suffix : FString();

	if (ItemStyleSetName == FAppStyle::Get().GetStyleSetName())
	{
		return FString::Printf(TEXT("%sFAppStyle%s::Get()."), *Prefix_Type, *Suffix_Type);
	}

	// Deprecated in UE 5.1
	/* if (ItemStyleSetName == FEditorStyle::Get().GetStyleSetName())
	{
		return FString::Printf(TEXT("%sFEditorStyle%s::Get()."), *Prefix_Type, *Suffix_Type);
	}*/

	if (FCoreStyle::IsInitialized() && ItemStyleSetName == FCoreStyle::GetCoreStyle().GetStyleSetName())
	{
		return FString::Printf(TEXT("%sFCoreStyle%s::GetCoreStyle()."), *Prefix_Type, *Suffix_Type);
	}

	if (ItemStyleSetName == FCoreStyle::Get().GetStyleSetName())
	{
		return FString::Printf(TEXT("%sFCoreStyle%s::Get()."), *Prefix_Type, *Suffix_Type);
	}

	return FString::Printf(TEXT("%sFSlateStyleRegistry%s::FindSlateStyle(%s\"%s\"%s)."),
		*Prefix_Type,
		*Suffix_Type,
		*Prefix_Literal,
		*ItemStyleSetName.ToString(),
		*Suffix_Literal);
}
}
