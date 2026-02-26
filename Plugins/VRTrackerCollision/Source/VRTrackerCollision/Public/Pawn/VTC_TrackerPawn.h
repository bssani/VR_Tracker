// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_TrackerPawn.h — VR HMD Camera + 5개 Tracker를 하나의 Pawn에 통합
//
// [왜 이게 필요한가]
//   UMotionControllerComponent는 Pawn의 VR Origin 기준으로 월드 위치를 계산하므로
//   Tracker는 반드시 Pawn 안에서 관리해야 VR Origin이 일치한다.
//
// [구조]
//   Root
//   └─ VROrigin (SceneComponent) ← VR 트래킹 공간의 원점 (바닥 기준)
//        ├─ Camera (CameraComponent)  ← HMD 위치/방향 자동 추적
//        ├─ MC_Waist      (MotionController, Special_1)
//        ├─ MC_LeftKnee   (MotionController, Special_2)
//        ├─ MC_RightKnee  (MotionController, Special_3)
//        ├─ MC_LeftFoot   (MotionController, Special_4)
//        └─ MC_RightFoot  (MotionController, Special_5)
//
// [사용법]
//   BP_VTC_TrackerPawn을 GameMode의 DefaultPawnClass로 설정하면
//   게임 시작 시 자동 스폰된다.
//
// [시뮬레이션 모드]
//   bSimulationMode = true 이면 VR 장비 없이 데스크탑에서 테스트 가능.
//   WASD 이동 + 마우스 룩 + 착석 자세 시뮬레이션.
//   bAutoDetectSimulation = true면 HMD 미감지 시 자동 전환.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "Tracker/VTC_TrackerInterface.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "VTC_TrackerPawn.generated.h"

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="VTC Tracker Pawn"))
class VRTRACKERCOLLISION_API AVTC_TrackerPawn : public APawn, public IVTC_TrackerInterface
{
	GENERATED_BODY()

public:
	AVTC_TrackerPawn();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override; // IMC 등록 타이밍

public:
	// ─── VR 카메라 ───────────────────────────────────────────────────────────

	// VR 트래킹 공간의 원점 (방 기준 좌표계 시작점 = 바닥)
	// 이 컴포넌트 하위에 Camera와 모든 MotionController를 붙인다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|VR")
	TObjectPtr<USceneComponent> VROrigin;

	// HMD 위치와 방향을 자동으로 추적하는 카메라
	// OpenXR Plugin이 매 프레임 HMD 자세를 이 컴포넌트에 반영한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|VR")
	TObjectPtr<UCameraComponent> Camera;

	// ─── Tracker MotionController (모두 VROrigin 하위) ──────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Trackers")
	TObjectPtr<UMotionControllerComponent> MC_Waist;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Trackers")
	TObjectPtr<UMotionControllerComponent> MC_LeftKnee;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Trackers")
	TObjectPtr<UMotionControllerComponent> MC_RightKnee;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Trackers")
	TObjectPtr<UMotionControllerComponent> MC_LeftFoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Trackers")
	TObjectPtr<UMotionControllerComponent> MC_RightFoot;

	// ─── MotionSource 이름 (SteamVR Tracker Role과 일치시켜야 함) ────────────
	// SteamVR → Settings → Controllers → Manage Trackers에서 할당한 Role과 맞춰야 한다.
	// 언리얼 엔진 최신 버전에서는 Special_X 대신 역할 이름으로 바로 매핑 가능.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Tracker Config")
	FName MotionSource_Waist = FName("Waist");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Tracker Config")
	FName MotionSource_LeftKnee = FName("LeftKnee");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Tracker Config")
	FName MotionSource_RightKnee = FName("RightKnee");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Tracker Config")
	FName MotionSource_LeftFoot = FName("LeftFoot");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Tracker Config")
	FName MotionSource_RightFoot = FName("RightFoot");

	// ─── IVTC_TrackerInterface 구현 ─────────────────────────────────────────

	virtual FVTCTrackerData GetTrackerData(EVTCTrackerRole TrackerRole) const override;
	virtual FVector         GetTrackerLocation(EVTCTrackerRole TrackerRole) const override;
	virtual bool            IsTrackerActive(EVTCTrackerRole TrackerRole) const override;
	virtual bool            AreAllTrackersActive() const override;
	virtual int32           GetActiveTrackerCount() const override;

	// Blueprint에서도 호출 가능하도록 래핑 (인터페이스 메서드는 BP에서 직접 호출 불가)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Tracker")
	FVTCTrackerData BP_GetTrackerData(EVTCTrackerRole TrackerRole) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Tracker")
	FVector BP_GetTrackerLocation(EVTCTrackerRole TrackerRole) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Tracker")
	bool BP_IsTrackerActive(EVTCTrackerRole TrackerRole) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Tracker")
	bool BP_AreAllTrackersActive() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Tracker")
	int32 BP_GetActiveTrackerCount() const;

	// ─── Delegates ──────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "VTC|Tracker|Events")
	FOnVTCTrackerUpdated OnTrackerUpdated;

	UPROPERTY(BlueprintAssignable, Category = "VTC|Tracker|Events")
	FOnVTCAllTrackersUpdated OnAllTrackersUpdated;

	// ─── Debug ──────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Debug")
	bool bShowDebugSpheres = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Debug")
	float DebugSphereRadius = 5.0f;

	// ═══════════════════════════════════════════════════════════════════════
	//  시뮬레이션 모드 (VR 장비 없이 데스크탑에서 테스트)
	// ═══════════════════════════════════════════════════════════════════════

	// true면 VR 트래커 대신 시뮬레이션된 좌표를 사용
	// HMD가 감지되지 않으면 자동으로 true가 될 수 있음 (bAutoDetectSimulation)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation")
	bool bSimulationMode = false;

	// true면 BeginPlay 시점에 HMD 감지 여부를 확인하고
	// 감지되지 않으면 자동으로 시뮬레이션 모드로 전환
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation")
	bool bAutoDetectSimulation = true;

	// ─── 시뮬레이션 이동 설정 ───────────────────────────────────────────────

	// WASD 이동 속도 (cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation|Movement",
		meta=(ClampMin=50.0f, ClampMax=1000.0f, EditCondition="bSimulationMode"))
	float SimMoveSpeed = 200.0f;

	// 마우스 감도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation|Movement",
		meta=(ClampMin=0.1f, ClampMax=5.0f, EditCondition="bSimulationMode"))
	float SimMouseSensitivity = 1.0f;

	// ─── 시뮬레이션 신체 오프셋 (카메라 기준 상대 위치, cm) ─────────────────

	// 착석 자세 기준:
	// 카메라(머리)로부터의 상대 오프셋으로 각 트래커 위치를 시뮬레이션
	// X = 전방(+)/후방(-), Y = 우측(+)/좌측(-), Z = 위(+)/아래(-)

	// Waist: 머리 아래 약 40cm, 약간 뒤쪽
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation|Body Offsets",
		meta=(EditCondition="bSimulationMode"))
	FVector SimOffset_Waist = FVector(-15.0f, 0.0f, -40.0f);

	// Left Knee: 허리 아래 약 45cm, 전방으로 나옴, 왼쪽으로 15cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation|Body Offsets",
		meta=(EditCondition="bSimulationMode"))
	FVector SimOffset_LeftKnee = FVector(20.0f, -15.0f, -65.0f);

	// Right Knee: 허리 아래 약 45cm, 전방으로 나옴, 오른쪽으로 15cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation|Body Offsets",
		meta=(EditCondition="bSimulationMode"))
	FVector SimOffset_RightKnee = FVector(20.0f, 15.0f, -65.0f);

	// Left Foot: 무릎 아래 약 40cm, 전방으로 더 나옴, 왼쪽
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation|Body Offsets",
		meta=(EditCondition="bSimulationMode"))
	FVector SimOffset_LeftFoot = FVector(40.0f, -15.0f, -100.0f);

	// Right Foot: 무릎 아래 약 40cm, 전방으로 더 나옴, 오른쪽
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation|Body Offsets",
		meta=(EditCondition="bSimulationMode"))
	FVector SimOffset_RightFoot = FVector(40.0f, 15.0f, -100.0f);

	// ─── 시뮬레이션 무릎 조작 (키보드) ──────────────────────────────────────

	// 무릎 추가 오프셋 — 런타임에 키보드로 조절됨
	// NumPad 4/6: 왼쪽 무릎 좌우, NumPad 8/2: 왼쪽 무릎 전후
	// Arrow Keys: 오른쪽 무릎 좌우/전후
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Simulation|Runtime")
	FVector SimKneeAdjust_Left = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Simulation|Runtime")
	FVector SimKneeAdjust_Right = FVector::ZeroVector;

	// 무릎 조절 속도 (cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation|Movement",
		meta=(ClampMin=5.0f, ClampMax=100.0f, EditCondition="bSimulationMode"))
	float SimKneeAdjustSpeed = 30.0f;

	// ─── 시뮬레이션 입력 에셋 (Enhanced Input) ────────────────────────────────
	// BP_VTC_TrackerPawn Details > "VTC|Simulation|Input" 에서 각 에셋을 연결하세요.
	// 에셋 생성: Content Browser 우클릭 → Input → Input Action / Input Mapping Context

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation|Input",
		meta=(DisplayName="Sim Input Mapping Context"))
	TObjectPtr<UInputMappingContext> SimInputMappingContext;

	// Axis2D — X: 전후(W/S), Y: 좌우(A/D)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation|Input")
	TObjectPtr<UInputAction> IA_Move;

	// Axis2D — X: Yaw(마우스X), Y: Pitch(마우스Y)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation|Input")
	TObjectPtr<UInputAction> IA_Look;

	// Digital — Backspace: 시뮬레이션 모드 토글
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation|Input")
	TObjectPtr<UInputAction> IA_ToggleSim;

	// Digital — R: 무릎 오프셋 초기화
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation|Input")
	TObjectPtr<UInputAction> IA_ResetKnees;

	// Axis2D — X: 좌우(NumPad4/6), Y: 전후(NumPad2/8)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation|Input")
	TObjectPtr<UInputAction> IA_AdjustLeftKnee;

	// Axis2D — X: 좌우(ArrowLeft/Right), Y: 전후(ArrowDown/Up)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Simulation|Input")
	TObjectPtr<UInputAction> IA_AdjustRightKnee;

	// ─── 시뮬레이션 제어 함수 ───────────────────────────────────────────────

	// 시뮬레이션 모드 토글 (런타임에서 Backspace 키로 전환)
	UFUNCTION(BlueprintCallable, Category = "VTC|Simulation")
	void ToggleSimulationMode();

	// 무릎 위치를 기본값으로 리셋 (R 키)
	UFUNCTION(BlueprintCallable, Category = "VTC|Simulation")
	void ResetKneeAdjustments();

	// 현재 시뮬레이션 모드 여부 조회
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Simulation")
	bool IsInSimulationMode() const { return bSimulationMode; }

private:
	// TrackerRole → TrackerData 캐시 맵
	TMap<EVTCTrackerRole, FVTCTrackerData> TrackerDataMap;

	// 매 Tick: 5개 트래커 데이터 갱신
	void UpdateAllTrackers();
	void UpdateTracker(EVTCTrackerRole TrackerRole, UMotionControllerComponent* MC);

	UMotionControllerComponent* GetMotionController(EVTCTrackerRole TrackerRole) const;

	// ─── 시뮬레이션 Private ─────────────────────────────────────────────────

	// 시뮬레이션 모드용 이동 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "VTC|Simulation")
	TObjectPtr<UFloatingPawnMovement> SimMovement;

	// 시뮬레이션 모드 Tick: 카메라 기준 상대 좌표로 트래커 위치 계산
	void UpdateSimulatedTrackers();

	// 시뮬레이션 모드에서 단일 트래커 데이터 갱신
	void UpdateSimulatedTracker(EVTCTrackerRole TrackerRole, const FVector& WorldLocation);

	// 카메라 기준 상대 오프셋을 월드 좌표로 변환
	FVector SimOffsetToWorld(const FVector& LocalOffset) const;

	// 마우스 룩 입력
	float SimYawInput = 0.0f;
	float SimPitchInput = 0.0f;

	// 시뮬레이션 입력 바인딩용 함수 (Enhanced Input)
	void SimMove(const FInputActionValue& Value);
	void SimLook(const FInputActionValue& Value);
	void SimAdjustLeftKnee(const FInputActionValue& Value);
	void SimAdjustRightKnee(const FInputActionValue& Value);

	// HMD 감지 확인
	bool DetectHMDPresence() const;
};
