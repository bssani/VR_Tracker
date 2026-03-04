// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_VRGameMode.h — VR 전용 GameMode
//
// [역할]
//   VR 레벨(VTC_VRTestLevel)에서만 사용하는 GameMode.
//   SimPlayerController 대신 OperatorController를 기본 컨트롤러로 사용해
//   WASD 입력 없이 세션 관리(1/2/3/4 키)만 활성화한다.
//
//   InitGame()에서 GameInstance.RunMode를 VR로 강제 설정:
//   - INI에 Simulation 모드가 저장되어 있어도 VR 레벨에서는 무조건 VR로 동작.
//   - BeginPlay()보다 먼저 실행되므로 TrackerPawn/OperatorController BeginPlay에서
//     올바른 RunMode를 읽게 된다.
//
// [Blueprint]
//   BP_VTC_VRGameMode 생성 후 VR 테스트 레벨의 World Settings > GameMode Override
//   에 설정한다. OperatorController와 TrackerPawn은 Blueprint에서도 override 가능.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VTC_VRGameMode.generated.h"

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="VTC VR Game Mode"))
class VRTRACKERCOLLISION_API AVTC_VRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AVTC_VRGameMode();

protected:
	// BeginPlay()보다 훨씬 먼저 호출됨 — RunMode 강제 설정에 최적
	virtual void InitGame(const FString& MapName, const FString& Options,
	                      FString& ErrorMessage) override;
};
