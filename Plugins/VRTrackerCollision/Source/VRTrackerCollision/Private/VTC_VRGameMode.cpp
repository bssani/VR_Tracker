// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "VTC_VRGameMode.h"
#include "Controller/VTC_OperatorController.h"
#include "Pawn/VTC_TrackerPawn.h"
#include "VTC_GameInstance.h"

AVTC_VRGameMode::AVTC_VRGameMode()
{
	// VR 레벨: 세션 관리 전용 컨트롤러 (WASD 없음)
	PlayerControllerClass = AVTC_OperatorController::StaticClass();

	// VR 레벨에서도 동일한 TrackerPawn 사용
	DefaultPawnClass = AVTC_TrackerPawn::StaticClass();
}

void AVTC_VRGameMode::InitGame(const FString& MapName, const FString& Options,
                               FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// InitGame은 어떤 Actor의 BeginPlay보다 먼저 실행된다.
	// 여기서 RunMode를 VR로 강제해야 TrackerPawn/OperatorController BeginPlay에서
	// 올바른 RunMode를 읽을 수 있다.
	// INI에 Simulation 모드가 저장되어 있어도 VR 레벨에서는 VR이 맞다.
	if (UVTC_GameInstance* GI = GetGameInstance<UVTC_GameInstance>())
	{
		GI->SessionConfig.RunMode = EVTCRunMode::VR;
		UE_LOG(LogTemp, Log, TEXT("[VTC] VRGameMode::InitGame — RunMode forced to VR."));
	}
}
