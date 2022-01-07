// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class CarlaUE4 : ModuleRules
{
	private bool IsWindows(ReadOnlyTargetRules Target)
	{
		return (Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32);
	}
	public CarlaUE4(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivatePCHHeaderFile = "CarlaUE4.h";

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG" });

		if (Target.Type == TargetType.Editor)
		{
			PublicDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
		}
		else
		{
			// only build this carla exception in Editor mode
			PublicDefinitions.Add("NO_DREYEVR_EXCEPTIONS");
		}
		// Add module for SteamVR support with UE4
		PublicDependencyModuleNames.AddRange(new string[] { "HeadMountedDisplay" });

		// SRanipal plugin for Windows
		if (IsWindows(Target))
		{ // SRanipal unfortunately only works on Windows
			bEnableExceptions = true; // enable unwind semantics for C++-style exceptions
			PrivateDependencyModuleNames.AddRange(new string[] { "SRanipalEye", "LogitechWheelPlugin" });
			PrivateIncludePathModuleNames.AddRange(new string[] { "SRanipalEye" });
		}


		PrivateDependencyModuleNames.AddRange( new string[] {"ImageWriteQueue"});
		
		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
