// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_GameMode.h — 시뮬레이션(데스크탑) 전용 GameMode
//
// [역할]
//   시뮬레이션 레벨(VTC_SimTestLevel)에서 사용하는 GameMode.
//   SimPlayerController(WASD/마우스)로 TrackerPawn을 데스크탑에서 조작한다.
//
//   InitGame()에서 GameInstance.RunMode를 Simulation으로 강제:
//   - INI에 VR 모드가 저장되어 있어도 Sim 레벨에서는 무조건 Simulation으로 동작.
//
// [VR 레벨]
//   VR 전용 레벨은 VTC_VRGameMode(BP_VTC_VRGameMode)를 사용하세요.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Controller/VTC_SimPlayerController.h"
#include "VTC_GameMode.generated.h"

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="VTC Simulation Game Mode"))
class VRTRACKERCOLLISION_API AVTC_GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AVTC_GameMode();

protected:
	// BeginPlay()보다 훨씬 먼저 호출됨 — RunMode 강제 설정에 최적
	virtual void InitGame(const FString& MapName, const FString& Options,
	                      FString& ErrorMessage) override;
};
