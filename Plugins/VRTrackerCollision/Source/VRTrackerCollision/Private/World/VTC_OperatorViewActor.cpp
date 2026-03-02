// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "World/VTC_OperatorViewActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "IXRTrackingSystem.h"
#include "IHeadMountedDisplay.h"
#include "ISpectatorScreenController.h"

AVTC_OperatorViewActor::AVTC_OperatorViewActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SetRootComponent(SceneCapture);

	// 기본 설정: 탑다운, 불필요한 패스 비활성화
	SceneCapture->ProjectionType        = ECameraProjectionMode::Orthographic;
	SceneCapture->OrthoWidth            = 500.0f;
	SceneCapture->bCaptureEveryFrame    = true;
	SceneCapture->bCaptureOnMovement    = false;
	SceneCapture->bAlwaysPersistRenderingState = true;

	// 성능 최적화: 불필요한 렌더 패스 비활성화
	SceneCapture->ShowFlags.SetMotionBlur(false);
	SceneCapture->ShowFlags.SetBloom(false);
	SceneCapture->ShowFlags.SetLensFlares(false);
	SceneCapture->ShowFlags.SetAmbientOcclusion(false);
	SceneCapture->ShowFlags.SetDynamicShadows(false);

	// 탑다운: 아래를 향하도록 회전 (-90도 Pitch)
	SceneCapture->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
}

void AVTC_OperatorViewActor::BeginPlay()
{
	Super::BeginPlay();

	// 카메라 설정 적용
	SceneCapture->OrthoWidth    = CaptureOrthoWidth;
	SceneCapture->ProjectionType = bOrthographic
		? ECameraProjectionMode::Orthographic
		: ECameraProjectionMode::Perspective;

	// RenderTarget 자동 생성 (BP에서 미리 할당하지 않은 경우)
	if (!RenderTarget)
	{
		RenderTarget = NewObject<UTextureRenderTarget2D>(this,
			UTextureRenderTarget2D::StaticClass(), TEXT("VTC_OperatorRenderTarget"));
		RenderTarget->InitCustomFormat(CaptureWidth, CaptureHeight,
			PF_B8G8R8A8, false);
		RenderTarget->UpdateResourceImmediate(true);
	}

	SceneCapture->TextureTarget = RenderTarget;

	// Spectator Screen에 자동 연결
	SetupSpectatorScreen();
}

void AVTC_OperatorViewActor::SetupSpectatorScreen()
{
	if (!RenderTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VTC] OperatorViewActor: RenderTarget 없음 — Spectator Screen 연결 불가."));
		return;
	}

	// UE5 XR Spectator Screen API:
	// GetSpectatorScreenController()는 IXRTrackingSystem이 아닌 IHeadMountedDisplay에 있음.
	if (GEngine && GEngine->XRSystem.IsValid())
	{
		IHeadMountedDisplay* HMD = GEngine->XRSystem->GetHMDDevice();
		ISpectatorScreenController* SpectatorController =
			HMD ? HMD->GetSpectatorScreenController() : nullptr;

		if (SpectatorController)
		{
			SpectatorController->SetSpectatorScreenMode(ESpectatorScreenMode::SingleEyeCroppedToFill);
			SpectatorController->SetSpectatorScreenTexture(RenderTarget);
			bSpectatorConnected = true;
			UE_LOG(LogTemp, Log, TEXT("[VTC] OperatorView: Spectator Screen 연결 완료 (%dx%d)."),
				CaptureWidth, CaptureHeight);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[VTC] OperatorView: SpectatorScreenController 없음 (VR 장비 미연결?). "
				     "데스크탑 모드에서는 RenderTarget을 직접 UMG 위젯에 연결하세요."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log,
			TEXT("[VTC] OperatorView: XRSystem 없음 (Simulation 모드). RenderTarget만 활성화."));
	}
}

void AVTC_OperatorViewActor::DisconnectSpectatorScreen()
{
	if (!bSpectatorConnected) return;

	if (GEngine && GEngine->XRSystem.IsValid())
	{
		IHeadMountedDisplay* HMD = GEngine->XRSystem->GetHMDDevice();
		ISpectatorScreenController* SpectatorController =
			HMD ? HMD->GetSpectatorScreenController() : nullptr;

		if (SpectatorController)
		{
			// UE5에 ESpectatorScreenMode::Default 없음 → Disabled 사용
			SpectatorController->SetSpectatorScreenMode(ESpectatorScreenMode::Disabled);
			bSpectatorConnected = false;
			UE_LOG(LogTemp, Log, TEXT("[VTC] OperatorView: Spectator Screen 연결 해제."));
		}
	}
}

void AVTC_OperatorViewActor::SetCaptureTarget(AActor* TargetActor, float HeightOffset)
{
	if (!TargetActor) return;

	const FVector TargetLocation = TargetActor->GetActorLocation();
	SetActorLocation(FVector(TargetLocation.X, TargetLocation.Y,
	                         TargetLocation.Z + HeightOffset));
}
