// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class LibLogManager : ModuleRules
{
	public LibLogManager(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		PublicIncludePaths.Add(ModuleDirectory);

		if(Target.Platform == UnrealTargetPlatform.Win64)
		{
			RuntimeDependencies.Add(Path.Combine(ModuleDirectory, "Windows/liblogmanager.dll"), StagedFileType.NonUFS);
		}
		else if (Target.Platform == UnrealTargetPlatform.Android)
		{
			// 1. Le linker doit voir la lib
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "Android/arm64-v8a/liblogmanager.so"));
			// 2. Le packager doit copier la lib dans l’APK
			RuntimeDependencies.Add(Path.Combine(ModuleDirectory, "Android/arm64-v8a/liblogmanager.so"));

			//PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "Android/arm64-v8a/clang_rt.asan-aarch64-android.so")); 
			//PublicDelayLoadDLLs.Add(Path.Combine(ModuleDirectory, "Android/arm64-v8a/clang_rt.asan-aarch64-android.so"));
		}
	}
}