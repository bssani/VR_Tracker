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

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="VKC Tracker Pawn"))
class VRTRACKERCOLLISION_API AVTC_TrackerPawn : public APawn, public IVTC_TrackerInterface
{
	GENERATED_BODY()

public:
	AVTC_TrackerPawn();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override {}

public:
	// ─── VR 카메라 ───────────────────────────────────────────────────────────

	// VR 트래킹 공간의 원점 (방 기준 좌표계 시작점 = 바닥)
	// 이 컴포넌트 하위에 Camera와 모든 MotionController를 붙인다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|VR")
	TObjectPtr<USceneComponent> VROrigin;

	// HMD 위치와 방향을 자동으로 추적하는 카메라
	// OpenXR Plugin이 매 프레임 HMD 자세를 이 컴포넌트에 반영한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|VR")
	TObjectPtr<UCameraComponent> Camera;

	// ─── Tracker MotionController (모두 VROrigin 하위) ──────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Trackers")
	TObjectPtr<UMotionControllerComponent> MC_Waist;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Trackers")
	TObjectPtr<UMotionControllerComponent> MC_LeftKnee;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Trackers")
	TObjectPtr<UMotionControllerComponent> MC_RightKnee;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Trackers")
	TObjectPtr<UMotionControllerComponent> MC_LeftFoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Trackers")
	TObjectPtr<UMotionControllerComponent> MC_RightFoot;

	// ─── MotionSource 이름 (SteamVR Tracker Role과 일치시켜야 함) ────────────
	// SteamVR → Settings → Controllers → Manage Trackers에서 할당한 Role과 맞춰야 한다.
	// 기본값: Special_1 ~ Special_5

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Tracker Config")
	FName MotionSource_Waist = FName("Special_1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Tracker Config")
	FName MotionSource_LeftKnee = FName("Special_2");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Tracker Config")
	FName MotionSource_RightKnee = FName("Special_3");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Tracker Config")
	FName MotionSource_LeftFoot = FName("Special_4");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Tracker Config")
	FName MotionSource_RightFoot = FName("Special_5");

	// ─── IVTC_TrackerInterface 구현 ─────────────────────────────────────────

	virtual FVTCTrackerData GetTrackerData(EVTCTrackerRole TrackerRole) const override;
	virtual FVector         GetTrackerLocation(EVTCTrackerRole TrackerRole) const override;
	virtual bool            IsTrackerActive(EVTCTrackerRole TrackerRole) const override;
	virtual bool            AreAllTrackersActive() const override;
	virtual int32           GetActiveTrackerCount() const override;

	// Blueprint에서도 호출 가능하도록 래핑 (인터페이스 메서드는 BP에서 직접 호출 불가)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Tracker")
	FVTCTrackerData BP_GetTrackerData(EVTCTrackerRole TrackerRole) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Tracker")
	FVector BP_GetTrackerLocation(EVTCTrackerRole TrackerRole) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Tracker")
	bool BP_IsTrackerActive(EVTCTrackerRole TrackerRole) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Tracker")
	bool BP_AreAllTrackersActive() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Tracker")
	int32 BP_GetActiveTrackerCount() const;

	// ─── Delegates ──────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "VKC|Tracker|Events")
	FOnVKCTrackerUpdated OnTrackerUpdated;

	UPROPERTY(BlueprintAssignable, Category = "VKC|Tracker|Events")
	FOnVKCAllTrackersUpdated OnAllTrackersUpdated;

	// ─── Debug ──────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Debug")
	bool bShowDebugSpheres = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Debug")
	float DebugSphereRadius = 5.0f;

private:
	// TrackerRole → TrackerData 캐시 맵
	TMap<EVTCTrackerRole, FVTCTrackerData> TrackerDataMap;

	// 매 Tick: 5개 트래커 데이터 갱신
	void UpdateAllTrackers();
	void UpdateTracker(EVTCTrackerRole TrackerRole, UMotionControllerComponent* MC);

	UMotionControllerComponent* GetMotionController(EVTCTrackerRole TrackerRole) const;
};
