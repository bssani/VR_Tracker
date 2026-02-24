// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_TrackerManager.h — 5개 Vive Tracker 통합 관리 Actor

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MotionControllerComponent.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "VTC_TrackerManager.generated.h"

// Tracker 데이터 갱신 시 브로드캐스트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVKCTrackerUpdated, const FVTCTrackerData&, TrackerData);

// 모든 Tracker 갱신 완료 시 브로드캐스트 (매 Tick)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVKCAllTrackersUpdated);

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="VKC Tracker Manager"))
class VRTRACKERCOLLISION_API AVTC_TrackerManager : public AActor
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

	// 특정 역할의 Tracker 데이터 반환
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Tracker")
	FVTCTrackerData GetTrackerData(EVTCTrackerRole TrackerRole) const;

	// 특정 역할의 Tracker가 현재 추적 중인지
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Tracker")
	bool IsTrackerActive(EVTCTrackerRole TrackerRole) const;

	// 특정 역할의 Tracker 월드 위치 반환 (빠른 접근용)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Tracker")
	FVector GetTrackerLocation(EVTCTrackerRole TrackerRole) const;

	// 모든 Tracker가 활성 상태인지 확인
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Tracker")
	bool AreAllTrackersActive() const;

	// 활성 Tracker 수 반환
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Tracker")
	int32 GetActiveTrackerCount() const;

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
