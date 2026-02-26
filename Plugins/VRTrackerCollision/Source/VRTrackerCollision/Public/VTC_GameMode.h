// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_GameMode.h — VTC_TrackerPawn을 기본 Pawn으로 사용하는 GameMode

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Controller/VTC_SimPlayerController.h"
#include "VTC_GameMode.generated.h"

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="VTC Game Mode"))
class VRTRACKERCOLLISION_API AVTC_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AVTC_GameMode();
};
