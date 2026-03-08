// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "VTC_VRGameMode.h"
#include "Controller/VTC_OperatorController.h"
#include "Pawn/VTC_TrackerPawn.h"

AVTC_VRGameMode::AVTC_VRGameMode()
{
	// VR 레벨: 세션 관리 전용 컨트롤러 (WASD 없음)
	PlayerControllerClass = AVTC_OperatorController::StaticClass();

	// VR 레벨에서 TrackerPawn 사용
	DefaultPawnClass = AVTC_TrackerPawn::StaticClass();
}
