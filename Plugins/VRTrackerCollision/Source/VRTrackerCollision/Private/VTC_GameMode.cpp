// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "VTC_GameMode.h"
#include "Pawn/VTC_TrackerPawn.h"
#include "Controller/VTC_SimPlayerController.h"

AVTC_GameMode::AVTC_GameMode()
{
	// 게임 시작 시 VTC_TrackerPawn을 자동 스폰
	DefaultPawnClass = AVTC_TrackerPawn::StaticClass();

	// 시뮬레이션 입력 처리용 PlayerController
	// Enhanced Input IMC/IA 에셋은 BP_VTC_SimPlayerController에서 연결
	PlayerControllerClass = AVTC_SimPlayerController::StaticClass();
}
