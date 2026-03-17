// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "VTC_GameMode.h"
#include "Pawn/VTC_TrackerPawn.h"
#include "Controller/VTC_OperatorController.h"

AVTC_GameMode::AVTC_GameMode()
{
	// 게임 시작 시 VTC_TrackerPawn을 자동 스폰
	DefaultPawnClass = AVTC_TrackerPawn::StaticClass();

	// VR 세션 제어용 PlayerController
	PlayerControllerClass = AVTC_OperatorController::StaticClass();
}
