// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_SimPlayerController.h — 시뮬레이션 모드용 PlayerController
//
// [왜 PlayerController에서 입력을 처리하는가]
//   UEnhancedInputLocalPlayerSubsystem은 LocalPlayer 소유 객체이며,
//   PlayerController의 BeginPlay()에서 GetLocalPlayer()를 통해
//   가장 안정적으로 접근할 수 있다.
//   Pawn::PossessedBy()에서 등록하면 Controller 유효성/타이밍에 의존하게 되어
//   PIE 환경에서 간헐적으로 입력이 안 잡히는 문제가 생긴다.
//
// [사용법]
//   1. Content Browser에서 VTC_SimPlayerController를 부모로 블루프린트 생성
//      (BP_VTC_SimPlayerController 권장)
//   2. BP Details > VTC|Input 에서 IMC / IA 에셋 연결
//   3. BP_VTC_GameMode > PlayerControllerClass = BP_VTC_SimPlayerController 설정

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "VTC_SimPlayerController.generated.h"

class AVTC_TrackerPawn;

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="VTC Sim Player Controller"))
class VRTRACKERCOLLISION_API AVTC_SimPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	// IMC 등록: BeginPlay에서는 GetLocalPlayer()가 항상 유효하다
	virtual void BeginPlay() override;

	// 액션 바인딩: InputComponent가 생성된 직후 호출됨
	virtual void SetupInputComponent() override;

public:
	// ─── Enhanced Input 에셋 ─────────────────────────────────────────────
	// BP_VTC_SimPlayerController Details > "VTC|Input" 에서 연결

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Input",
		meta=(DisplayName="Sim Input Mapping Context"))
	TObjectPtr<UInputMappingContext> SimInputMappingContext;

	// Axis2D — X: 전후(W/S), Y: 좌우(A/D)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Input")
	TObjectPtr<UInputAction> IA_Move;

	// Axis2D — X: Yaw(마우스X), Y: Pitch(마우스Y)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Input")
	TObjectPtr<UInputAction> IA_Look;

	// Digital — Backspace: 시뮬레이션 모드 토글
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Input")
	TObjectPtr<UInputAction> IA_ToggleSim;

	// Digital — R: 무릎 오프셋 초기화
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Input")
	TObjectPtr<UInputAction> IA_ResetKnees;

	// Axis2D — X: 좌우(NumPad4/6), Y: 전후(NumPad2/8)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Input")
	TObjectPtr<UInputAction> IA_AdjustLeftKnee;

	// Axis2D — X: 좌우(ArrowLeft/Right), Y: 전후(ArrowDown/Up)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Input")
	TObjectPtr<UInputAction> IA_AdjustRightKnee;

private:
	// 현재 빙의된 Pawn을 AVTC_TrackerPawn으로 캐스트해 반환
	AVTC_TrackerPawn* GetTrackerPawn() const;

	// ─── 입력 핸들러 (Pawn의 해당 함수로 포워딩) ──────────────────────
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_ToggleSim(const FInputActionValue& Value);
	void Input_ResetKnees(const FInputActionValue& Value);
	void Input_AdjustLeftKnee(const FInputActionValue& Value);
	void Input_AdjustRightKnee(const FInputActionValue& Value);
};
