using System.IO;
using UnrealBuildTool;

public class LogManager : ModuleRules
{
	public LogManager(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Projects",
				"LibLogManager",
				"Json",
				"JsonUtilities"
			});

/*		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/LibLogManager/Windows/liblogmanager.dll");
		}
		else*/
		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			// IMPORTANT : staging dans l’APK
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "LogManager_APL.xml"));
		}
	}
}	