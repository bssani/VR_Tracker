// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "Collision/VTC_WarningFeedback.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/PostProcessVolume.h"

UVTC_WarningFeedback::UVTC_WarningFeedback()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UVTC_WarningFeedback::BeginPlay()
{
	Super::BeginPlay();
}

void UVTC_WarningFeedback::OnWarningLevelChanged(EVTCTrackerRole BodyPart,
	FString PartName, EVTCWarningLevel NewLevel)
{
	// 더 심각한 방향으로만 업데이트 (Safe로 돌아갈 때는 별도 Reset 필요)
	if ((int32)NewLevel >= (int32)CurrentLevel)
	{
		CurrentLevel = NewLevel;
	}

	switch (NewLevel)
	{
	case EVTCWarningLevel::Safe:
		ApplySafeState();
		break;
	case EVTCWarningLevel::Warning:
		ApplyWarningState();
		break;
	case EVTCWarningLevel::Collision:
		ApplyCollisionState(FVector::ZeroVector);
		break;
	}
}

void UVTC_WarningFeedback::ApplySafeState()
{
	SetPostProcessVignetteColor(FLinearColor::Transparent, 0.0f);
}

void UVTC_WarningFeedback::ApplyWarningState()
{
	// 노란색 화면 테두리
	SetPostProcessVignetteColor(FLinearColor(1.0f, 0.8f, 0.0f, 1.0f), WarningVignetteIntensity);

	// 경고음 재생
	if (WarningSFX)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), WarningSFX);
	}
}

void UVTC_WarningFeedback::ApplyCollisionState(FVector CollisionLocation)
{
	// 빨간색 화면 테두리
	SetPostProcessVignetteColor(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f), CollisionVignetteIntensity);

	// 충돌음 재생 (쿨다운)
	const float Now = GetWorld()->GetTimeSeconds();
	if (CollisionSFX && (Now - LastCollisionSoundTime) > CollisionSoundCooldown)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), CollisionSFX);
		LastCollisionSoundTime = Now;
	}

	// Niagara 이펙트 스폰
	if (!CollisionLocation.IsZero())
	{
		SpawnCollisionFX(CollisionLocation);
	}
}

void UVTC_WarningFeedback::SpawnCollisionFX(FVector Location)
{
	if (CollisionImpactFX && GetWorld())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), CollisionImpactFX, Location,
			FRotator::ZeroRotator, FVector::OneVector,
			true, true, ENCPoolMethod::AutoRelease);
	}
}

void UVTC_WarningFeedback::SetPostProcessVignetteColor(FLinearColor Color, float Intensity)
{
	// PostProcessVolume이 설정된 경우 Material Parameter를 통해 Vignette 색상 제어
	// 실제 구현은 PostProcessVolume의 Material에 따라 다름
	// 여기서는 UE5 PostProcessVolume의 기본 VignetteIntensity를 활용
	if (!PostProcessVolume) return;

	if (APostProcessVolume* PPV = Cast<APostProcessVolume>(PostProcessVolume))
	{
		PPV->Settings.bOverride_VignetteIntensity = true;
		PPV->Settings.VignetteIntensity = Intensity;
	}
}

void UVTC_WarningFeedback::ResetFeedback()
{
	CurrentLevel = EVTCWarningLevel::Safe;
	ApplySafeState();
}
