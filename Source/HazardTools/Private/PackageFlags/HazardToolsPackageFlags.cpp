// Copyright Neyl Sullivan 2022

#include "HazardToolsPackageFlags.h"
#include "HazardToolsPackageFlagsTypes.h"
#include "SHazardToolsPackageFlagsDialog.h"
#include "MainFrame/Public/Interfaces/IMainFrameModule.h"

TArray<UPackage*> GetSelectedPackages()
{
	check(GEditor);

	TArray<FAssetData> SelectedAssets;
	GEditor->GetContentBrowserSelections(SelectedAssets);

	TArray<UPackage*> SelectedPackages;

	for (int32 i = 0; i < SelectedAssets.Num(); i++)
	{
		if (const UObject* AssetObject = SelectedAssets[i].GetAsset())
		{
			if (UPackage* Package = AssetObject->GetOutermost())
			{
				SelectedPackages.Add(Package);
			}
		}
	}

	return SelectedPackages;
}

void CollectFlagsForSelectedAssets(TArray<FHazardToolsPackageFlagsDescription>& Descriptions)
{
	const TArray<UPackage*> SelectedPackages = GetSelectedPackages();

	if (SelectedPackages.Num() == 0)
	{
		for (int FlagIdx = 0; FlagIdx < Descriptions.Num(); FlagIdx++)
		{
			FHazardToolsPackageFlagsDescription& Desc = Descriptions[FlagIdx];

			Desc.InState = FHazardToolsPackageFlagsDescription::EState::Unchecked;
		}
		return;
	}

	for (FHazardToolsPackageFlagsDescription& Desc : Descriptions)
	{
		TArray<FString> PackagesWithFlagNames;

		for (const UPackage* Package : SelectedPackages)
		{
			const bool bPackageHasFlag = Package->HasAllPackagesFlags(Desc.Flag);

			// if first checked asset
			if (Desc.InState == FHazardToolsPackageFlagsDescription::EState::NotInitialized)
			{
				//set flag state directly
				Desc.InState = bPackageHasFlag ? FHazardToolsPackageFlagsDescription::EState::Checked : FHazardToolsPackageFlagsDescription::EState::Unchecked;
			}
			else //can lead to mixed values
			{
				if ((bPackageHasFlag && Desc.InState == FHazardToolsPackageFlagsDescription::EState::Unchecked)
				    || (!bPackageHasFlag && Desc.InState == FHazardToolsPackageFlagsDescription::EState::Checked))
				{
					Desc.InState = FHazardToolsPackageFlagsDescription::EState::Mixed;
				}
			}

			//counter
			if (bPackageHasFlag)
			{
				PackagesWithFlagNames.Add(Package->GetPathName());
			}
		}

		//compose info text
		if (Desc.InState == FHazardToolsPackageFlagsDescription::EState::Mixed) //only for mixed flags
		{
			Desc.Info = FText::FromString(FString::Printf(TEXT("With Flag: %i/%i"), PackagesWithFlagNames.Num(), SelectedPackages.Num()));

			FString TooltipStr;

			for (int32 i = 0; i < PackagesWithFlagNames.Num(); i++)
			{
				TooltipStr += PackagesWithFlagNames[i];
				if (i != PackagesWithFlagNames.Num() - 1)
				{
					TooltipStr += LINE_TERMINATOR;
				}
			}
			Desc.InfoTooltip = FText::FromString(TooltipStr);
		}
	}
}

void SetChangedFlagsForSelectedAssets(const TArray<FHazardToolsPackageFlagsDescription>& Descriptions)
{
	const TArray<UPackage*> SelectedPackages = GetSelectedPackages();

	if (SelectedPackages.Num() == 0)
	{
		return;
	}

	for (UPackage* Package : SelectedPackages)
	{
		for (int FlagIdx = 0; FlagIdx < Descriptions.Num(); FlagIdx++)
		{
			const FHazardToolsPackageFlagsDescription& Desc = Descriptions[FlagIdx];

			switch (Desc.OutState)
			{
				case FHazardToolsPackageFlagsDescription::EState::NotInitialized:
					break;
				case FHazardToolsPackageFlagsDescription::EState::Checked:
					if (Package->HasAllPackagesFlags(Desc.Flag) == false) //check only for unchecked
					{
						Package->SetPackageFlags(Desc.Flag);
						Package->SetDirtyFlag(true);
					}
					break;
				case FHazardToolsPackageFlagsDescription::EState::Unchecked:
					if (Package->HasAllPackagesFlags(Desc.Flag) == true) //uncheck only for checked
					{
						Package->ClearPackageFlags(Desc.Flag);
						Package->SetDirtyFlag(true);
					}
					break;
				default:
					checkNoEntry(); //case FHazardToolsPackagesFlagsDescription::EState::Mixed:
			}
		}
	}
}

uint32 GetSelectedAssetsInfo(FText& ItemsInfo)
{
	TArray<FAssetData> SelectedAssets;
	GEditor->GetContentBrowserSelections(SelectedAssets);

	FString ItemsInfoStr;

	for (int32 i = 0; i < SelectedAssets.Num(); i++)
	{
		ItemsInfoStr += SelectedAssets[i].GetFullName();
		if (i != SelectedAssets.Num() - 1)
		{
			ItemsInfoStr += LINE_TERMINATOR;
		}
	}

	ItemsInfo = FText::FromString(ItemsInfoStr);

	return SelectedAssets.Num();
}


void FHazardToolsPackageFlags::ShowModalDialog()
{
	auto Descriptions = HazardToolsPackageFlagsDescription::ConstructCleanDescriptionArray();

	CollectFlagsForSelectedAssets(Descriptions);

	const TSharedPtr<SWindow> Window =
		SNew(SWindow)
		.Title(INVTEXT("Edit Packages Flags"))
		.ClientSize(SHazardToolsPackagesFlagsDialog::Default_Window_Size)
		.SizingRule(ESizingRule::UserSized)
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	FText SelectedItemsInfoText;
	const uint32 SelectedItemsNum = GetSelectedAssetsInfo(SelectedItemsInfoText);

	const TSharedRef<SHazardToolsPackagesFlagsDialog> Dialog =
		SNew(SHazardToolsPackagesFlagsDialog)
		.ParentWindow(Window)
		.FlagsDescriptions(Descriptions)
		.TotalSelectedItemsNum(SelectedItemsNum)
		.SelectedItemsInfo(SelectedItemsInfoText);

	Window->SetContent(Dialog);

	const IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");

	FSlateApplication::Get().AddModalWindow(Window.ToSharedRef(), MainFrameModule.GetParentWindow());

	if (Dialog.Get().IsOkayClicked() == false)
	{
		return;
	}

	Dialog.Get().FillFlagsDescriptions(Descriptions);
	SetChangedFlagsForSelectedAssets(Descriptions);
}
