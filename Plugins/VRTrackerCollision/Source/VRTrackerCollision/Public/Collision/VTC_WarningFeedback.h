// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_WarningFeedback.h — 시각/청각 경고 피드백 (PostProcess, 사운드, Niagara)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "VTC_WarningFeedback.generated.h"

class UPostProcessComponent;
class UAudioComponent;

UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent, DisplayName="VKC Warning Feedback"))
class VRTRACKERCOLLISION_API UVTC_WarningFeedback : public UActorComponent
{
	GENERATED_BODY()

public:
	UVTC_WarningFeedback();

	virtual void BeginPlay() override;

	// ─── 설정 — PostProcess (화면 테두리 빨간색 효과) ──────────────────────

	// VR 레벨에 있는 PostProcessVolume 참조
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Feedback|PostProcess")
	TObjectPtr<AActor> PostProcessVolume;

	// 경고 시 Vignette 강도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Feedback|PostProcess",
		meta=(ClampMin=0.0f, ClampMax=1.0f))
	float WarningVignetteIntensity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Feedback|PostProcess",
		meta=(ClampMin=0.0f, ClampMax=1.0f))
	float CollisionVignetteIntensity = 1.0f;

	// ─── 설정 — 사운드 ────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Feedback|Sound")
	TObjectPtr<USoundBase> WarningSFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Feedback|Sound")
	TObjectPtr<USoundBase> CollisionSFX;

	// ─── 설정 — Niagara 이펙트 ──────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Feedback|FX")
	TObjectPtr<UNiagaraSystem> CollisionImpactFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Feedback|FX")
	TObjectPtr<UNiagaraSystem> WarningPulseFX;

	// ─── 함수 ────────────────────────────────────────────────────────────────

	// 경고 레벨 변경 시 호출 (CollisionDetector의 Delegate에 연결)
	UFUNCTION(BlueprintCallable, Category = "VKC|Feedback")
	void OnWarningLevelChanged(EVTCTrackerRole BodyPart, FString PartName, EVTCWarningLevel NewLevel);

	// 특정 위치에 충돌 이펙트 스폰
	UFUNCTION(BlueprintCallable, Category = "VKC|Feedback")
	void SpawnCollisionFX(FVector Location);

	// 모든 피드백 초기화 (Safe 상태로)
	UFUNCTION(BlueprintCallable, Category = "VKC|Feedback")
	void ResetFeedback();

	// 현재 레벨
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VKC|Feedback")
	EVTCWarningLevel CurrentLevel = EVTCWarningLevel::Safe;

private:
	void ApplySafeState();
	void ApplyWarningState();
	void ApplyCollisionState(FVector CollisionLocation);

	// PostProcess Material의 동적 파라미터 제어
	void SetPostProcessVignetteColor(FLinearColor Color, float Intensity);

	// 마지막 충돌 사운드 재생 시간 (연속 재생 방지)
	float LastCollisionSoundTime = -999.0f;
	float CollisionSoundCooldown = 0.5f;
};
