// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Controller/VTC_SimPlayerController.h"
#include "Pawn/VTC_TrackerPawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

void AVTC_SimPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// PlayerController::BeginPlay()에서는 GetLocalPlayer()가 항상 유효하다.
	// Pawn::PossessedBy()와 달리 타이밍 문제가 없다.
	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());

	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VTC] EnhancedInputLocalPlayerSubsystem을 찾지 못했습니다. "
			     "Enhanced Input 플러그인이 활성화되어 있는지 확인하세요."));
		return;
	}

	if (SimInputMappingContext)
	{
		Subsystem->AddMappingContext(SimInputMappingContext, 0);
		UE_LOG(LogTemp, Log, TEXT("[VTC] IMC registered in PlayerController::BeginPlay."));
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[VTC] SimInputMappingContext가 없습니다. "
			     "BP_VTC_SimPlayerController > VTC|Input 에서 IMC 에셋을 연결하세요."));
	}
}

void AVTC_SimPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[VTC] EnhancedInputComponent를 찾지 못했습니다. "
			     "Project Settings > Engine > Input > Default Input Component Class = "
			     "EnhancedInputComponent 로 설정되어 있는지 확인하세요."));
		return;
	}

	// Triggered: 키를 누르는 동안 매 프레임 호출
	if (IA_Move)            EIC->BindAction(IA_Move,            ETriggerEvent::Triggered, this, &AVTC_SimPlayerController::Input_Move);
	if (IA_Look)            EIC->BindAction(IA_Look,            ETriggerEvent::Triggered, this, &AVTC_SimPlayerController::Input_Look);
	if (IA_AdjustLeftKnee)  EIC->BindAction(IA_AdjustLeftKnee,  ETriggerEvent::Triggered, this, &AVTC_SimPlayerController::Input_AdjustLeftKnee);
	if (IA_AdjustRightKnee) EIC->BindAction(IA_AdjustRightKnee, ETriggerEvent::Triggered, this, &AVTC_SimPlayerController::Input_AdjustRightKnee);

	// Started: 키를 처음 눌렀을 때 1회만 호출
	if (IA_ToggleSim)       EIC->BindAction(IA_ToggleSim,       ETriggerEvent::Started,   this, &AVTC_SimPlayerController::Input_ToggleSim);
	if (IA_ResetKnees)      EIC->BindAction(IA_ResetKnees,      ETriggerEvent::Started,   this, &AVTC_SimPlayerController::Input_ResetKnees);

	UE_LOG(LogTemp, Log, TEXT("[VTC] Input actions bound in PlayerController::SetupInputComponent."));
}

AVTC_TrackerPawn* AVTC_SimPlayerController::GetTrackerPawn() const
{
	return Cast<AVTC_TrackerPawn>(GetPawn());
}

void AVTC_SimPlayerController::Input_Move(const FInputActionValue& Value)
{
	if (AVTC_TrackerPawn* P = GetTrackerPawn()) P->SimMove(Value);
}

void AVTC_SimPlayerController::Input_Look(const FInputActionValue& Value)
{
	if (AVTC_TrackerPawn* P = GetTrackerPawn()) P->SimLook(Value);
}

void AVTC_SimPlayerController::Input_ToggleSim(const FInputActionValue& Value)
{
	if (AVTC_TrackerPawn* P = GetTrackerPawn()) P->ToggleSimulationMode();
}

void AVTC_SimPlayerController::Input_ResetKnees(const FInputActionValue& Value)
{
	if (AVTC_TrackerPawn* P = GetTrackerPawn()) P->ResetKneeAdjustments();
}

void AVTC_SimPlayerController::Input_AdjustLeftKnee(const FInputActionValue& Value)
{
	if (AVTC_TrackerPawn* P = GetTrackerPawn()) P->SimAdjustLeftKnee(Value);
}

void AVTC_SimPlayerController::Input_AdjustRightKnee(const FInputActionValue& Value)
{
	if (AVTC_TrackerPawn* P = GetTrackerPawn()) P->SimAdjustRightKnee(Value);
}
