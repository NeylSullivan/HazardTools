// Copyright Neyl Sullivan 2022

using UnrealBuildTool;

public class HazardTools : ModuleRules
{
	public HazardTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		if (Target.bCompileFreeType)
		{
			AddEngineThirdPartyPrivateStaticDependencies(Target, "FreeType2", "UElibPNG", "zlib");
		}

		PrivateIncludePaths.AddRange(
			new[]
			{
				"HazardTools/Private",
				"HazardTools/Private/PackageFlags",
				"HazardTools/Private/ObjectOutliner",
				"HazardTools/Private/StyleBrowser",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"CoreUObject",
				"Engine",
				"Projects",
				"InputCore",
				"UnrealEd",
				"LevelEditor",
				"EditorStyle",
				"MaterialEditor",
				"Slate",
				"SlateCore",
				"EditorWidgets",
				"UMG",
				"UMGEditor",
				"Kismet",
				"PropertyEditor",
				"ContentBrowser",
				"DeveloperSettings",
				"AssetTools",
				"AssetRegistry",
				"ClassViewer",
				"ToolMenus",
				"ApplicationCore",
				"MeshDescription",
				"Settings"
			}
		);
		DynamicallyLoadedModuleNames.AddRange(new string[] { });
	}
}
