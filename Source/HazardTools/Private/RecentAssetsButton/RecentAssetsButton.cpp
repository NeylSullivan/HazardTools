// Copyright Neyl Sullivan 2022


#include "RecentAssetsButton.h"

#include "MRUFavoritesList.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Interfaces/IMainFrameModule.h"
#include "Styling/SlateIconFinder.h"

static FSlateIcon GetIconFromAssetPath(const FString& AssetPath)
{
	FSlateIcon Icon;

	TArray<FAssetData> AssetDataList;
	if (IAssetRegistry::GetChecked().GetAssetsByPackageName(*AssetPath, AssetDataList))
	{
		for (const auto& AssetData : AssetDataList)
		{
			if (const UClass* Class = FindObject<UClass>(AssetData.AssetClassPath))
			{
				Icon = FSlateIconFinder::FindIconForClass(Class);
				break;
			}
		}
	}

	return Icon;
}


static bool HasNoPlayWorld()
{
	return GEditor->PlayWorld == nullptr;
}

static bool CanShowRecentAssets()
{
	return HasNoPlayWorld();
}

static bool ShouldDisplayRecentLevelEntry(const FString PathToCheck)
{
	if (CanShowRecentAssets() == false)
	{
		return false;
	}

	if (GEditor)
	{
		if (const UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			if (PathToCheck == World->GetPackage()->GetPathName())
			{
				return false;
			}
		}
	}

	return true;
}

static void OpenRecentAsset_Clicked(const FString AssetPath)
{
	if (ensure(AssetPath.Len()))
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(AssetPath);
	}
}


static void FillRecentOpenAssetMenu(FMenuBuilder& MenuBuilder)
{
	const FMainMRUFavoritesList* RecentlyOpenedAssetsList = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->GetRecentlyOpenedAssets();
	check(RecentlyOpenedAssetsList != nullptr);

	const int32 NumOfRecentAssets = FMath::Min(RecentlyOpenedAssetsList->GetNumItems(), 15);

	TArray<FString> ValidRecentlyOpenedAssetPathList;

	for (int32 Index = 0; Index < NumOfRecentAssets; Index++)
	{
		const FString RecentlyOpenedAssetPath = RecentlyOpenedAssetsList->GetMRUItem(Index);

		TArray<FAssetData> AssetDataList;
		if (!IAssetRegistry::GetChecked().GetAssetsByPackageName(*RecentlyOpenedAssetPath, AssetDataList))
		{
			continue;
		}

		static const FTopLevelAssetPath WorldClassPathName(TEXT("/Script/Engine"), TEXT("World"));
		static const FTopLevelAssetPath LevelSequenceClassPathName(TEXT("/Script/LevelSequence"), TEXT("LevelSequence"));

		bool bContainsLevelAsset = false;
		bool bContainsLevelSequenceAsset = false;
		for (const auto& AssetData : AssetDataList)
		{
			if (AssetData.AssetClassPath == WorldClassPathName)
			{
				bContainsLevelAsset = true;
				break;
			}

			if (AssetData.AssetClassPath == LevelSequenceClassPathName)
			{
				bContainsLevelSequenceAsset = true;
				break;
			}
		}
		if (bContainsLevelAsset || bContainsLevelSequenceAsset)
		{
			continue;
		}

		ValidRecentlyOpenedAssetPathList.Add(RecentlyOpenedAssetPath);
	}

	if (ValidRecentlyOpenedAssetPathList.IsEmpty())
	{
		return;
	}
	MenuBuilder.BeginSection(NAME_None, INVTEXT("Recent Assets"));

	for (auto RecentlyOpenedAssetPath : ValidRecentlyOpenedAssetPathList)
	{
		const FText Label = FText::FromString(FPaths::GetBaseFilename(RecentlyOpenedAssetPath));
		const FText ToolTip = FText::Format(INVTEXT("Opens recent asset: {0}"), FText::FromString(RecentlyOpenedAssetPath));
		const FSlateIcon Icon = GetIconFromAssetPath(RecentlyOpenedAssetPath);

		MenuBuilder.AddMenuEntry(
			Label,
			ToolTip,
			Icon,
			FUIAction(
				FExecuteAction::CreateStatic(&OpenRecentAsset_Clicked, RecentlyOpenedAssetPath),
				FCanExecuteAction::CreateStatic(&HasNoPlayWorld),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateStatic(&HasNoPlayWorld)
				)
			);
	}

	MenuBuilder.EndSection();
}

static void FillRecentOpenLevelsMenu(FMenuBuilder& MenuBuilder)
{
	const FMainMRUFavoritesList& MRUFavorites = *FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame").GetMRUFavoritesList();
	const int32 NumRecent = MRUFavorites.GetNumItems();

	const int32 AllowedRecent = FMath::Min(NumRecent, 10);

	TArray<FString> ValidRecentlyOpenedLevelPathList;

	for (int32 CurRecentIndex = 0; CurRecentIndex < AllowedRecent; ++CurRecentIndex)
	{
		if (!MRUFavorites.MRUItemPassesCurrentFilter(CurRecentIndex))
		{
			continue;
		}

		ValidRecentlyOpenedLevelPathList.Add(MRUFavorites.GetMRUItem(CurRecentIndex));
	}

	if (ValidRecentlyOpenedLevelPathList.IsEmpty())
	{
		return;
	}

	MenuBuilder.BeginSection(NAME_None, INVTEXT("Recent Levels"));

	static FSlateIcon Icon = FSlateIconFinder::FindIconForClass(UWorld::StaticClass());

	for (auto RecentlyOpenedLevelPath : ValidRecentlyOpenedLevelPathList)
	{
		const FText Label = FText::FromString(FPaths::GetBaseFilename(RecentlyOpenedLevelPath));
		const FText ToolTip = FText::Format(INVTEXT("Opens recent level: {0}"), FText::FromString(RecentlyOpenedLevelPath));

		MenuBuilder.AddMenuEntry(
			Label,
			ToolTip,
			Icon,
			FUIAction(
				FExecuteAction::CreateStatic(&OpenRecentAsset_Clicked, RecentlyOpenedLevelPath),
				FCanExecuteAction::CreateStatic(&HasNoPlayWorld),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateStatic(&ShouldDisplayRecentLevelEntry, RecentlyOpenedLevelPath)
				)
			);
	}

	MenuBuilder.EndSection();
}

static void FillRecentOpenLevelSequencesMenu(FMenuBuilder& MenuBuilder)
{
	const FMainMRUFavoritesList* RecentlyOpenedAssetsList = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->GetRecentlyOpenedAssets();
	check(RecentlyOpenedAssetsList != nullptr);

	TArray<FString> ValidRecentlyOpenedAssetPathList;
	constexpr int32 MaxNumRecentlyOpenLevelSequencesToAdd = 10;

	// Iterate all available assets
	for (int32 Index = 0; Index < RecentlyOpenedAssetsList->GetNumItems(); Index++)
	{
		const FString RecentlyOpenedAssetPath = RecentlyOpenedAssetsList->GetMRUItem(Index);

		TArray<FAssetData> AssetDataList;
		if (!IAssetRegistry::GetChecked().GetAssetsByPackageName(*RecentlyOpenedAssetPath, AssetDataList))
		{
			continue;
		}

		static const FTopLevelAssetPath LevelSequenceClassPathName(TEXT("/Script/LevelSequence"), TEXT("LevelSequence"));

		for (const auto& AssetData : AssetDataList)
		{
			if (AssetData.AssetClassPath == LevelSequenceClassPathName)
			{
				ValidRecentlyOpenedAssetPathList.Add(RecentlyOpenedAssetPath);
			}
		}

		if (ValidRecentlyOpenedAssetPathList.Num() >= MaxNumRecentlyOpenLevelSequencesToAdd)
		{
			break;
		}
	}

	if (ValidRecentlyOpenedAssetPathList.IsEmpty())
	{
		return;
	}
	MenuBuilder.BeginSection(NAME_None, INVTEXT("Recent Level Sequences"));

	for (auto RecentlyOpenedAssetPath : ValidRecentlyOpenedAssetPathList)
	{
		const FText Label = FText::FromString(FPaths::GetBaseFilename(RecentlyOpenedAssetPath));
		const FText ToolTip = FText::Format(INVTEXT("Opens recent level sequence: {0}"), FText::FromString(RecentlyOpenedAssetPath));
		const FSlateIcon Icon = GetIconFromAssetPath(RecentlyOpenedAssetPath);

		MenuBuilder.AddMenuEntry(
			Label,
			ToolTip,
			Icon,
			FUIAction(
				FExecuteAction::CreateStatic(&OpenRecentAsset_Clicked, RecentlyOpenedAssetPath),
				FCanExecuteAction::CreateStatic(&HasNoPlayWorld),
				FIsActionChecked(),
				FIsActionButtonVisible::CreateStatic(&HasNoPlayWorld)
				)
			);
	}

	MenuBuilder.EndSection();
}

static TSharedRef<SWidget> GetRecentAssetsDropdown()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	FillRecentOpenAssetMenu(MenuBuilder);
	FillRecentOpenLevelsMenu(MenuBuilder);
	FillRecentOpenLevelSequencesMenu(MenuBuilder);

	return MenuBuilder.MakeWidget();
};

void FRecentAssetsButton::Register()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	FToolMenuSection& Section = Menu->AddSection("HazardToolsExtensions", TAttribute<FText>(), FToolMenuInsert("Play", EToolMenuInsertType::After));

	FToolMenuEntry RecentAssetsButtonEntry = FToolMenuEntry::InitComboButton(
		"RecentAssetsButton",
		FUIAction(
			FExecuteAction(),
			FCanExecuteAction::CreateStatic(&HasNoPlayWorld),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateStatic(&CanShowRecentAssets)),
		FOnGetContent::CreateStatic(&GetRecentAssetsDropdown),
		INVTEXT("Recent Assets"),
		INVTEXT("Select a recent asset to open"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "PlacementBrowser.Icons.Recent")
		);
	RecentAssetsButtonEntry.StyleNameOverride = "CalloutToolbar";
	Section.AddEntry(RecentAssetsButtonEntry);
}
