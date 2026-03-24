// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class LogManagerDemoTarget : TargetRules
{
	public LogManagerDemoTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		//DefaultBuildSettings = BuildSettingsVersion.Latest; // Ou V5 pour la 5.6
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange( new string[] { "LogManagerDemo" } );

/*		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			// Force le linker à être plus permissif avec les librairies tierces
			bOverrideBuildEnvironment = true;			
		}*/
	}
}
