// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"
#include "StyleBrowserFwd.h"

/**
 * 
 */
namespace HazardTools
{
class FStyleBrowserModel : public TSharedFromThis<FStyleBrowserModel>
{
public:
	DECLARE_DELEGATE_RetVal_OneParam(bool, FShouldPassItem, const FStyleBrowserItemPtr& Item);

	FStyleBrowserModel();

	void SetShouldPassTextFilter(const FShouldPassItem& InShouldItemPassTextFilterDelegate) { ShouldItemPassTextFilterDelegate = InShouldItemPassTextFilterDelegate; }

	const TArray<FStyleBrowserItemPtr>& GetFilteredContent() const { return FilteredContent; }

	void UpdateContent();

	int32 GetTotalNum() const { return Content.Num(); }
	int32 GetFilteredNum() const { return FilteredContent.Num(); }

	static FString GetRawCodeSnipped(const FStyleBrowserItemRef InItem);
	FText GetFormattedCodeSnipped(const FStyleBrowserItemRef InItem) const;

	const TArray<FName>& GetAvailableStyleSets() const { return AvailableStyleSets; }
	const TArray<FName>& GetAvailableStyleTypeCategories() const { return AvailableStyleTypeCategories; }

	FName GetDesiredStyleSetName() const { return DesiredStyleSetName; }
	void SetDesiredStyleSetName(const FName InDesiredStyleSetName);

	//UE_NODISCARD bool IsShouldAddFromParentStyleSets() const { return bShouldAddFromParentStyleSets; }
	void SetShouldAddFromParentStyleSets(const bool bInShouldAddFromParentStyles) { bShouldAddFromParentStyleSets = bInShouldAddFromParentStyles; }
	// return StyleSet hierarchy in form CurrentStyleSetName <- ParentStyleSetName <- ParentStyleSetName
	FString GetStyleSetsHierarchy() const;
	bool HasParentStyleSet() const;

	UE_NODISCARD FName GetStyleTypeNameToDisplay() const { return StyleTypeNameToDisplay; }
	void SetStyleTypeNameToDisplay(const FName InStyleTypeNameToDisplay);

private:
	// return Style Set to explore based on DesiredStyleSetName or default FAppStyle on fail
	const ISlateStyle* GetCurrentStyleSetPtr() const;

	static FString GetStyleSetCodeSnippedPart(const FStyleBrowserItemRef InItem, bool bFormatForSyntaxHighlighting);

	template <typename StyleBrowserItemType, typename StyleValueType>
	void Fill(TFunctionRef<const TMap<FName, StyleValueType>&(const ISlateStyle&)> Function, const ISlateStyle* ProcessedStyle = nullptr);

	bool bPerformFullRebuild = true;

	TArray<FStyleBrowserItemPtr> Content;
	TArray<FStyleBrowserItemPtr> FilteredContent;

	FShouldPassItem ShouldItemPassTextFilterDelegate;
	bool bShouldAddFromParentStyleSets = true;
	FName DesiredStyleSetName; // Initialized in ctor with FAppStyle::Get().GetStyleSetName()
	const ISlateStyle* CurrentStyleSet = nullptr;
	TArray<FName> AvailableStyleSets;

	TArray<FName> AvailableStyleTypeCategories;
	// NAME_None mean "Display all without filtering"	
	FName StyleTypeNameToDisplay = NAME_None;
};
}
