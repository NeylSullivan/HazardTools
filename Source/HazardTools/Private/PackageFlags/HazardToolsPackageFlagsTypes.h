// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"

struct FHazardToolsPackageFlagsDescription
{
	enum class EState
	{
		NotInitialized,
		Checked,
		Unchecked,
		Mixed
	};

	EPackageFlags Flag;
	FText DisplayName;
	FText Tooltip;
	FText Info;
	FText InfoTooltip;

	FHazardToolsPackageFlagsDescription(const EPackageFlags InFlag, const FString& InDisplayName, const FString& InTooltip)
	{
		Flag = InFlag;
		DisplayName = FText::FromString(InDisplayName);
		Tooltip = FText::FromString(InTooltip);
	}


	EState InState = EState::NotInitialized;
	EState OutState = EState::NotInitialized;
};

namespace HazardToolsPackageFlagsDescription
{
static TArray<FHazardToolsPackageFlagsDescription> ConstructCleanDescriptionArray()
{
	TArray<FHazardToolsPackageFlagsDescription> Descriptions;

#ifdef HZT_ADD_FLAG // To prevent redefinition from included header, should not be a problem in this particular case but to be on the safe side
	#error "Multiple defines"
#endif

#define HZT_ADD_FLAG(InFlag, InTooltip )  Descriptions.Add(FHazardToolsPackageFlagsDescription(InFlag, TEXT(#InFlag), TEXT(InTooltip)))

	HZT_ADD_FLAG(PKG_NewlyCreated, "Newly created package, not saved yet. In editor only.");
	HZT_ADD_FLAG(PKG_ClientOptional, "Purely optional for clients.");
	HZT_ADD_FLAG(PKG_ServerSideOnly, "Only needed on the server side.");
	HZT_ADD_FLAG(PKG_CompiledIn, "This package is from \"compiled in\" classes.");
	HZT_ADD_FLAG(PKG_ForDiffing, "This package was loaded just for the purposes of diffing");
	HZT_ADD_FLAG(PKG_EditorOnly, "This is editor-only package (for example: editor module script package)");
	HZT_ADD_FLAG(PKG_Developer, "Developer module");
	HZT_ADD_FLAG(PKG_UncookedOnly, "Loaded only in uncooked builds (i.e. runtime in editor)");
	HZT_ADD_FLAG(PKG_ContainsMapData, "Contains map data (UObjects only referenced by a single ULevel) but is stored in a different package");
	HZT_ADD_FLAG(PKG_Compiling, "Package is currently being compiled");
	HZT_ADD_FLAG(PKG_ContainsMap, "Set if the package contains a ULevel/ UWorld object");
	HZT_ADD_FLAG(PKG_RequiresLocalizationGather, "Set if the package contains any data to be gathered by localization");
	HZT_ADD_FLAG(PKG_PlayInEditor, "Set if the package was created for the purpose of PIE");
	HZT_ADD_FLAG(PKG_ContainsScript, "Package is allowed to contain UClass objects");
	HZT_ADD_FLAG(PKG_DisallowExport, "Editor should not export asset in this package");
	HZT_ADD_FLAG(PKG_ReloadingForCooker, "This package is reloading in the cooker, try to avoid getting data we will never need. We won't save this package.");
	HZT_ADD_FLAG(PKG_FilterEditorOnly, "Package has editor-only data filtered out");

#undef HZT_ADD_FLAG

	return Descriptions; // return array by value but it's ok
}
};
