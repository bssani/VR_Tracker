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

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "Tracker/VTC_TrackerInterface.h"
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

	// ─── Dropout 보간 (Feature C) ───────────────────────────────────────────

	// 트래커 추적 실패(dropout) 시 보간을 유지할 최대 프레임 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Tracker|Dropout",
		meta=(ClampMin=0, ClampMax=30))
	int32 MaxDropoutFrames = 5;

	// ─── Movement Phase 감지 (Feature E) ────────────────────────────────────

	// Hip Z 속도 임계값 (cm/s) — 이 이상 하강하면 Entering, 상승하면 Exiting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Tracker|Phase",
		meta=(ClampMin=1.0f, ClampMax=50.0f))
	float PhaseVelocityThreshold = 5.0f;

	// 현재 Movement Phase
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VTC|Tracker|Phase")
	EVTCMovementPhase CurrentPhase = EVTCMovementPhase::Unknown;

	// Phase 변경 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "VTC|Tracker|Events")
	FOnVTCPhaseChanged OnPhaseChanged;

	// ─── Delegates ──────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "VTC|Tracker|Events")
	FOnVTCTrackerUpdated OnTrackerUpdated;

	UPROPERTY(BlueprintAssignable, Category = "VTC|Tracker|Events")
	FOnVTCAllTrackersUpdated OnAllTrackersUpdated;

	// ─── Tracker Mesh 가시성 ────────────────────────────────────────────────

	// Vive Tracker 하드웨어 3D 메시(DeviceModel) 표시 여부.
	// VRTestLevel 시작 시 GameInstance 설정에 따라 자동 적용된다.
	UFUNCTION(BlueprintCallable, Category = "VTC|Trackers")
	void SetTrackerMeshVisible(bool bVisible);

	// ─── Debug ──────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Debug")
	bool bShowDebugSpheres = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Debug")
	float DebugSphereRadius = 5.0f;

	// ─── 착석 정렬 ──────────────────────────────────────────────────────────

	// true면 BeginPlay 시 SeatHipWorldPosition으로 자동 스냅
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Seating")
	bool bAutoSnapOnBeginPlay = false;

	// 차량 Hip의 월드 좌표. 에디터 Details에서 직접 입력.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Seating",
		meta=(EditCondition="bAutoSnapOnBeginPlay"))
	FVector SeatHipWorldPosition = FVector::ZeroVector;

	// Waist tracker가 WorldPos에 오도록 Pawn 전체를 이동.
	UFUNCTION(BlueprintCallable, Category = "VTC|Seating")
	void SnapWaistTo(const FVector& WorldPos);

	// Waist tracker가 아직 비활성 상태인 경우 RetryInterval(초)마다 재시도.
	UFUNCTION(BlueprintCallable, Category = "VTC|Seating")
	void SnapWaistToWithRetry(const FVector& WorldPos,
	                          float RetryInterval = 0.5f,
	                          int32 MaxRetries    = 20);

private:
	// TrackerRole → TrackerData 캐시 맵
	TMap<EVTCTrackerRole, FVTCTrackerData> TrackerDataMap;

	// ─── Dropout 보간 내부 상태 (Feature C) ─────────────────────────────────
	TMap<EVTCTrackerRole, TArray<FVector>> TrackerLocationHistory;
	TMap<EVTCTrackerRole, int32> DropoutFrameCount;

	// ─── Phase 감지 내부 상태 (Feature E) ───────────────────────────────────
	FVector PreviousHipLocation = FVector::ZeroVector;
	bool bHasPreviousHipLocation = false;

	void DetectMovementPhase(float DeltaTime);

	// HipSnap 타이머 핸들 (호환성)
	FTimerHandle HipSnapRetryTimer;

	// 매 Tick: 5개 트래커 데이터 갱신
	void UpdateAllTrackers();
	void UpdateTracker(EVTCTrackerRole TrackerRole, UMotionControllerComponent* MC);

	UMotionControllerComponent* GetMotionController(EVTCTrackerRole TrackerRole) const;
};
