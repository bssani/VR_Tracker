// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_TrackerManager.h — 5개 Vive Tracker 통합 관리 Actor

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MotionControllerComponent.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "Tracker/VTC_TrackerInterface.h"
#include "VTC_TrackerManager.generated.h"

// Tracker 데이터 갱신 시 브로드캐스트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVKCTrackerUpdated, const FVTCTrackerData&, TrackerData);

// 모든 Tracker 갱신 완료 시 브로드캐스트 (매 Tick)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVKCAllTrackersUpdated);

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="VKC Tracker Manager"))
class VRTRACKERCOLLISION_API AVTC_TrackerManager : public AActor, public IVTC_TrackerInterface
{
	GENERATED_BODY()

public:
	AVTC_TrackerManager();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ─── Motion Controller Components (에디터에서 MotionSource 설정 가능) ───

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Trackers")
	UMotionControllerComponent* MC_Waist;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Trackers")
	UMotionControllerComponent* MC_LeftKnee;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Trackers")
	UMotionControllerComponent* MC_RightKnee;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Trackers")
	UMotionControllerComponent* MC_LeftFoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Trackers")
	UMotionControllerComponent* MC_RightFoot;

	// Root
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VKC|Trackers")
	USceneComponent* RootSceneComponent;

public:
	// ─── SteamVR MotionSource 이름 (에디터에서 변경 가능) ───────────────────
	// SteamVR에서 Tracker Role 할당 후, 여기에 해당 MotionSource 이름 입력
	// 예: "Special_1", "Special_2", ... 또는 "LeftFoot", "RightFoot" 등

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

	// ─── Tracker 데이터 조회 ─────────────────────────────────────────────────

	// ─── IVTC_TrackerInterface 구현 ─────────────────────────────────────────

	virtual FVTCTrackerData GetTrackerData(EVTCTrackerRole TrackerRole) const override;
	virtual FVector         GetTrackerLocation(EVTCTrackerRole TrackerRole) const override;
	virtual bool            IsTrackerActive(EVTCTrackerRole TrackerRole) const override;
	virtual bool            AreAllTrackersActive() const override;
	virtual int32           GetActiveTrackerCount() const override;

	// Blueprint 접근용 (UFUNCTION은 virtual override에 직접 못 씀)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Tracker")
	FVTCTrackerData BP_GetTrackerData(EVTCTrackerRole TrackerRole) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Tracker")
	bool BP_IsTrackerActive(EVTCTrackerRole TrackerRole) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Tracker")
	FVector BP_GetTrackerLocation(EVTCTrackerRole TrackerRole) const;

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

	// Tracker 위치를 에디터/게임에서 시각화할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Debug")
	bool bShowDebugSpheres = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Debug")
	float DebugSphereRadius = 5.0f;

private:
	// 내부 데이터 캐시 (역할 → TrackerData 맵)
	TMap<EVTCTrackerRole, FVTCTrackerData> TrackerDataMap;

	// MotionController 컴포넌트 가져오기 (역할 → MC 매핑)
	UMotionControllerComponent* GetMotionController(EVTCTrackerRole TrackerRole) const;

	// 매 Tick: 모든 Tracker 위치 갱신
	void UpdateAllTrackers();

	// 단일 Tracker 갱신
	void UpdateTracker(EVTCTrackerRole TrackerRole, UMotionControllerComponent* MC);
};
