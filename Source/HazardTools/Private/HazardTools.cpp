// Copyright Neyl Sullivan 2022

#include "HazardTools.h"
#include "HazardToolsUtils.h"
#include "LevelEditor.h"
#include "SObjectOutliner.h"
#include "SStyleBrowser.h"
#include "PackageFlags/HazardToolsPackageFlags.h"


DEFINE_LOG_CATEGORY(LogHazardTools);

class FHazardToolsModule : public IModuleInterface
{
	TSharedPtr<FUICommandList> PluginCommands;
	TMap<FName, TSharedPtr<FTabSpawnerEntry>> RegisteredTabs;

public:
	virtual void StartupModule() override
	{
		UE_LOG(LogHazardTools, Log, TEXT("FHazardToolsModule::StartupModule"));

		if (!IsRunningCommandlet())
		{
			auto& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
			const TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
			MenuExtender->AddMenuBarExtension("Help", EExtensionHook::Before, PluginCommands, FMenuBarExtensionDelegate::CreateRaw(this, &FHazardToolsModule::MakeMenu));
			LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

			RegisterNomadTabSpawner("HazardToolsObjectOutlinerTab", INVTEXT("Object Outliner"), FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&)
			{
				return SNew(SDockTab)
				       .TabRole(NomadTab)
				       [
					       SNew(HazardTools::SObjectOutliner)
				       ];
			}));

			RegisterNomadTabSpawner("HazardToolsStyleBrowserTab", INVTEXT("Style Browser"), FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&)
			{
				return SNew(SDockTab)
				       .TabRole(NomadTab)
				       [
					       SNew(HazardTools::SStyleBrowser)
				       ];
			}));
		}
	}

	virtual void ShutdownModule() override
	{
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

	void MakeMenu(FMenuBarBuilder& MenuBuilder)
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
	}
};

IMPLEMENT_MODULE(FHazardToolsModule, HazardTools)
