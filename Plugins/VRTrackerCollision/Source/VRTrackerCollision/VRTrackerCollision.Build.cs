// Copyright GMTCK PQDQ Team. All Rights Reserved.

using UnrealBuildTool;

public class VRTrackerCollision : ModuleRules
{
	public VRTrackerCollision(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[]
		{
			// Plugin public headers are auto-discovered
		});

		PrivateIncludePaths.AddRange(new string[]
		{
			"VRTrackerCollision/Private"
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",        // UEnhancedInputComponent, UInputMappingContext
			"HeadMountedDisplay",   // UMotionControllerComponent
			"UMG",                  // Widget
			"Niagara",              // Particle effects
			"Json",                 // FJsonObject — VTC_VehiclePreset.cpp
			"JsonUtilities",        // FJsonObjectConverter — VTC_VehiclePreset.cpp
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"RenderCore",
			"RHI"
		});

		// OpenXR plugin dependency (Vive Pro 2 / Tracker support)
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicDependencyModuleNames.Add("OpenXRHMD");
		}
	}
}
