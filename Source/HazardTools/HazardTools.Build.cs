// Copyright Neyl Sullivan 2022

using UnrealBuildTool;

public class HazardTools : ModuleRules
{
	public HazardTools(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] { });

		PrivateIncludePaths.AddRange(
			new[]
			{
				"HazardTools/Private",
				"HazardTools/Private/PackageFlags",
				"HazardTools/Private/ObjectOutliner",
				"HazardTools/Private/StyleBrowser",
			}
		);

		PublicDependencyModuleNames.AddRange(
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
				"PropertyEditor",
				"ContentBrowser",
				"DeveloperSettings",
				"AssetTools",
				"ToolMenus",
				"ApplicationCore",
				"MeshDescription"
			}
		);
		DynamicallyLoadedModuleNames.AddRange(new string[] { });
	}
}