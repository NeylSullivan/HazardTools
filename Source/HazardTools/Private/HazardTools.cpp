// Copyright Neyl Sullivan 2022

#include "HazardTools.h"
#include "HazardToolsUtils.h"
#include "LevelEditor.h"
#include "SObjectOutliner.h"
#include "PrimaryAssetTypes/SPrimaryAssetTypesManager.h"
#include "SStyleBrowser.h"
#include "PackageFlags/HazardToolsPackageFlags.h"
#include "RecentAssetsButton/RecentAssetsButton.h"
#include "ConfigReferenceValidator/SHazardConfigReferenceValidatorWindow.h"
#include "UMGToSlate/SHazardUMGToSlate.h"


DEFINE_LOG_CATEGORY(LogHazardTools);

class FHazardToolsModule : public IModuleInterface
{
	TSharedPtr<FUICommandList> PluginCommands;
	TMap<FName, TSharedPtr<FTabSpawnerEntry>> RegisteredTabs;
	FDelegateHandle SlateExportMenuCallback;

public:
	virtual void StartupModule() override
	{
		UE_LOG(LogHazardTools, Log, TEXT("FHazardToolsModule::StartupModule"));

		if (!IsRunningCommandlet())
		{
			RegisterNomadTabSpawner(HazardTools::SHazardUMGToSlate::TabName, INVTEXT("UMG to Slate"), FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&) {
				return SNew(SDockTab).TabRole(NomadTab)
					[
						SNew(HazardTools::SHazardUMGToSlate)
					];
			}));

			RegisterNomadTabSpawner("HazardToolsObjectOutlinerTab", INVTEXT("Object Outliner"), FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&) {
				return SNew(SDockTab)
					.TabRole(NomadTab)
					[
						SNew(HazardTools::SObjectOutliner)
					];
			}));

			RegisterNomadTabSpawner("HazardToolsStyleBrowserTab", INVTEXT("Style Browser"), FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&) {
				return SNew(SDockTab)
					.TabRole(NomadTab)
					[
						SNew(HazardTools::SStyleBrowser)
					];
			}));

			RegisterNomadTabSpawner("HazardToolsPrimaryAssetTypesTab", INVTEXT("Primary Asset Types"), FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&) {
				return SNew(SDockTab)
					.TabRole(NomadTab)
					[
						SNew(HazardTools::SPrimaryAssetTypesManager)
					];
			}));

			RegisterNomadTabSpawner("HazardToolsConfigReferenceValidatorTab", INVTEXT("Config Reference Validator"), FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&) {
				return SNew(SDockTab)
					.TabRole(NomadTab)
					[
						SNew(HazardTools::SHazardConfigReferenceValidatorWindow)
					];
			}));

			if (FSlateApplication::IsInitialized())
			{
				UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FHazardToolsModule::ExtendMenu));
				UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateStatic(&FRecentAssetsButton::Register));
				SlateExportMenuCallback = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateStatic(&HazardTools::SHazardUMGToSlate::RegisterMenus));
			}
		}
	}

	virtual void ShutdownModule() override
	{
		UToolMenus::UnRegisterStartupCallback(SlateExportMenuCallback);
		UToolMenus::UnregisterOwner(HazardTools::SHazardUMGToSlate::TabName);
		for (const auto& NomadTabSpawnerName : RegisteredTabs)
		{
			FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(NomadTabSpawnerName.Key);
		}
		UE_LOG(LogHazardTools, Log, TEXT("FHazardToolsModule::ShutdownModule"));
	}

private:
	void RegisterNomadTabSpawner(const FName TabName, const FText& TabTitle, const FOnSpawnTab& OnSpawnTab)
	{
		check(RegisteredTabs.Contains(TabName) == false);

		auto& Entry = FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabName, OnSpawnTab)
		                                      .SetDisplayName(TabTitle)
		                                      .SetMenuType(ETabSpawnerMenuType::Hidden);

		RegisteredTabs.Add(TabName, Entry.AsSpawnerEntry());
	}

	static UToolMenu* GetOrCreateRootMenu()
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu");

		FToolMenuSection& Section = Menu->FindOrAddSection("HazardTools");
		FToolMenuEntry& HazardToolsEntry =
			Section.AddSubMenu(
				"HazardTools",
				INVTEXT("Hazard Tools"),
				INVTEXT("Hazard Tools"),
				FNewToolMenuChoice()
				);

		HazardToolsEntry.InsertPosition = FToolMenuInsert("Help", EToolMenuInsertType::After);

		UToolMenu* HazardToolsMenu = UToolMenus::Get()->RegisterMenu("LevelEditor.MainMenu.HazardTools", NAME_None, EMultiBoxType::Menu, /*bWarnIfAlreadyRegistered*/false);

		return HazardToolsMenu;
	}

	void ExtendMenu()
	{
		UToolMenu* Menu = GetOrCreateRootMenu();

		Menu->AddDynamicSection(NAME_None, FNewToolMenuDelegate::CreateLambda([this](UToolMenu* InMenu) {
				{
					FToolMenuSection& Section = InMenu->FindOrAddSection("HazardTools", INVTEXT("Tools"));
					Section.AddMenuEntry(
						"PackageFlags",
						INVTEXT("Packages Flags"),
						INVTEXT("Show Packages Flags Dialog for Asset(s) Selected in Content Browser"),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateStatic(&FHazardToolsPackageFlags::ShowModalDialog),
							FCanExecuteAction::CreateStatic(&FHazardToolsUtils::HasSelectedAssets)));
				}

				for (const auto& Tab : RegisteredTabs)
				{
					FToolMenuSection& Section = InMenu->FindOrAddSection("HazardTools", INVTEXT("Tools"));
					Section.AddMenuEntry(
						Tab.Value.Get()->GetFName(),
						Tab.Value.Get()->GetDisplayName(),
						Tab.Value.Get()->GetTooltipText(),
						Tab.Value.Get()->GetIcon(),
						FUIAction(FExecuteAction::CreateLambda([Tab]() {
							FGlobalTabmanager::Get()->TryInvokeTab(Tab.Key);
						})));
				}
			}
			));
	}

	/*void MakeMenu(FMenuBarBuilder& MenuBuilder)
	{
		MenuBuilder.AddPullDownMenu(
			INVTEXT("Hazard Tools"),
			INVTEXT("Hazard Tools Plugin Utilities"),
			FNewMenuDelegate::CreateRaw(this, &FHazardToolsModule::FillMenu),
			"HazardToolsMenu",
			"HazardToolsMenu"
			);
	}

	void FillMenu(FMenuBuilder& MenuBuilder) const
	{
		MenuBuilder.BeginSection(NAME_None, INVTEXT("Hazard Tools"));

		MenuBuilder.AddMenuEntry(
			INVTEXT("Packages Flags"),
			INVTEXT("Show Packages Flags Dialog for Asset(s) Selected in Content Browser"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateStatic(&FHazardToolsPackageFlags::ShowModalDialog),
				FCanExecuteAction::CreateStatic(&FHazardToolsUtils::HasSelectedAssets)));

		for (const auto& Tab : RegisteredTabs)
		{
			MenuBuilder.AddMenuEntry(
				Tab.Value.Get()->GetDisplayName(),
				Tab.Value.Get()->GetTooltipText(),
				Tab.Value.Get()->GetIcon(),
				FUIAction(FExecuteAction::CreateLambda([Tab]()
				{
					FGlobalTabmanager::Get()->TryInvokeTab(Tab.Key);
				})));
		}

		MenuBuilder.EndSection();
	}*/
};

IMPLEMENT_MODULE(FHazardToolsModule, HazardTools)