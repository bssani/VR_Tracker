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
	if (!PostProcessVolume) return;

	APostProcessVolume* PPV = Cast<APostProcessVolume>(PostProcessVolume);
	if (!PPV) return;

	// ── Vignette 강도 ─────────────────────────────────────────────────────
	PPV->Settings.bOverride_VignetteIntensity = true;
	PPV->Settings.VignetteIntensity = Intensity;

	// ── 씬 색상 틴트 (Color 파라미터를 실제로 화면에 반영) ────────────────
	// Intensity 0이면 화이트(원래 색), 1에 가까울수록 경고 색상으로 블렌드.
	// 강도를 0.3 계수로 낮춰 너무 강한 색상 왜곡을 방지한다.
	PPV->Settings.bOverride_SceneColorTint = true;
	if (Intensity > 0.f)
	{
		const float TintStrength = Intensity * 0.3f;
		PPV->Settings.SceneColorTint = FLinearColor(
			1.f + (Color.R - 1.f) * TintStrength,   // R: 1 → Color.R 방향으로 이동
			1.f + (Color.G - 1.f) * TintStrength,   // G
			1.f + (Color.B - 1.f) * TintStrength,   // B
			1.f);
	}
	else
	{
		PPV->Settings.SceneColorTint = FLinearColor::White;  // 원래 색으로 복원
	}
}

void UVTC_WarningFeedback::ResetFeedback()
{
	CurrentLevel = EVTCWarningLevel::Safe;
	ApplySafeState();
}
