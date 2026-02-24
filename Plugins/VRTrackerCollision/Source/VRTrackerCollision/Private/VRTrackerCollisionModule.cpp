// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "VRTrackerCollisionModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FVRTrackerCollisionModule"

void FVRTrackerCollisionModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("[VTC] VRTrackerCollision Plugin Loaded."));
}

void FVRTrackerCollisionModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("[VTC] VRTrackerCollision Plugin Unloaded."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FVRTrackerCollisionModule, VRTrackerCollision)
