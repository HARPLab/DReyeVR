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
		ShadowVariableWarningLevel = WarningLevel.Off; // -Wno-shadow

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG" });

		if (Target.Type == TargetType.Editor)
		{
			PublicDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
		}
		else
		{
			// only build this carla exception in package mode
			PublicDefinitions.Add("NO_DREYEVR_EXCEPTIONS");
		}

		// Add module for SteamVR support with UE4
		PublicDependencyModuleNames.AddRange(new string[] { "HeadMountedDisplay" });

		if (IsWindows(Target))
		{ 
			bEnableExceptions = true; // enable unwind semantics for C++-style exceptions
		}

		////////////////////////////////////////////////////////////////////////////////////
		// Edit these variables to enable/disable features of DReyeVR
		bool UseSRanipalPlugin = true;
		bool UseLogitechPlugin = true;
		////////////////////////////////////////////////////////////////////////////////////

		if (!IsWindows(Target))
		{
			// adjust definitions so they are OS-compatible
			UseSRanipalPlugin = false;       // SRanipal only works on Windows
			UseLogitechPlugin = false; // LogitechWheelPlugin also only works on Windows
		}

		// Add these preprocessor definitions to code
		PublicDefinitions.Add("USE_SRANIPAL_PLUGIN=" + (UseSRanipalPlugin ? "true" : "false"));
		PublicDefinitions.Add("USE_LOGITECH_PLUGIN=" + (UseLogitechPlugin ? "true" : "false"));

		// Add plugin dependencies 
		if (UseSRanipalPlugin)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "SRanipalEye" });
			PrivateIncludePathModuleNames.AddRange(new string[] { "SRanipalEye" });
		}

		if (UseLogitechPlugin)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "LogitechWheelPlugin" });
		}


		PrivateDependencyModuleNames.AddRange( new string[] {"ImageWriteQueue"});
	}
}
