// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "VTC_GameMode.h"
#include "Pawn/VTC_TrackerPawn.h"
#include "Controller/VTC_SimPlayerController.h"
#include "VTC_GameInstance.h"

AVTC_GameMode::AVTC_GameMode()
{
	// 시뮬레이션 레벨: TrackerPawn + SimPlayerController(WASD/마우스)
	DefaultPawnClass = AVTC_TrackerPawn::StaticClass();

	// 시뮬레이션 입력 처리용 PlayerController
	// Enhanced Input IMC/IA 에셋은 BP_VTC_SimPlayerController에서 연결
	PlayerControllerClass = AVTC_SimPlayerController::StaticClass();
}

void AVTC_GameMode::InitGame(const FString& MapName, const FString& Options,
                             FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// 시뮬레이션 레벨에 진입하면 RunMode를 Simulation으로 강제.
	// INI에 VR 모드가 저장되어 있어도 이 레벨에서는 Simulation이 맞다.
	if (UVTC_GameInstance* GI = GetGameInstance<UVTC_GameInstance>())
	{
		GI->SessionConfig.RunMode = EVTCRunMode::Simulation;
		UE_LOG(LogTemp, Log, TEXT("[VTC] GameMode::InitGame — RunMode forced to Simulation."));
	}
}
