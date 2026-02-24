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
			"HeadMountedDisplay",   // UMotionControllerComponent
			"UMG",                  // Widget
			"Niagara",              // Particle effects
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
