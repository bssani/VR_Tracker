// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_OperatorViewActor.h — 운영자 모니터링 뷰 (Feature I)
//
// [구조]
//   SceneCaptureComponent2D → UTextureRenderTarget2D → Spectator Screen
//
// [동작]
//   - Top-down 카메라(SceneCapture)가 차량 위에서 실험 장면을 촬영
//   - 렌더링된 텍스처를 UE5 Spectator Screen에 전달
//   - 운영자는 VR HMD를 쓴 피실험자 옆 모니터(Companion Screen)에서 실시간 확인 가능
//
// [Blueprint 설정]
//   1. BP_VTC_OperatorViewActor를 Level 2 레벨에 배치
//   2. CaptureHeight, CaptureOrthoWidth 조정
//   3. VTC_OperatorController.BeginPlay에서 자동으로 Spectator Screen에 연결됨

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VTC_OperatorViewActor.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="VTC Operator View Actor"))
class VRTRACKERCOLLISION_API AVTC_OperatorViewActor : public AActor
{
	GENERATED_BODY()

public:
	AVTC_OperatorViewActor();

protected:
	virtual void BeginPlay() override;

public:
	// ─── SceneCapture ────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|OperatorView")
	TObjectPtr<USceneCaptureComponent2D> SceneCapture;

	// ─── 설정 ────────────────────────────────────────────────────────────────

	// 캡처 렌더 타겟 (BP에서 생성 후 할당, 또는 BeginPlay에서 자동 생성)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|OperatorView")
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;

	// 캡처 해상도 (가로 x 세로, 작을수록 성능 부담 감소)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|OperatorView",
		meta=(ClampMin=128, ClampMax=2048))
	int32 CaptureWidth = 1280;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|OperatorView",
		meta=(ClampMin=128, ClampMax=2048))
	int32 CaptureHeight = 720;

	// 직교 투영 너비 (cm): 차량 크기에 맞게 조정 (Orthographic 모드일 때 적용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|OperatorView",
		meta=(ClampMin=100.0f, ClampMax=5000.0f))
	float CaptureOrthoWidth = 500.0f;

	// true = 직교 투영 (탑다운 평면도), false = 원근 투영
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|OperatorView")
	bool bOrthographic = true;

	// ─── Spectator Screen 연결 ───────────────────────────────────────────────

	// Spectator Screen 업데이트 주기 (Hz)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|OperatorView",
		meta=(ClampMin=1.0f, ClampMax=60.0f))
	float UpdateHz = 30.0f;

	// ─── Blueprint 호출 가능 함수 ─────────────────────────────────────────────

	// RenderTarget을 생성하고 Spectator Screen에 연결
	UFUNCTION(BlueprintCallable, Category = "VTC|OperatorView")
	void SetupSpectatorScreen();

	// Spectator Screen 연결 해제 (레벨 종료 시 호출)
	UFUNCTION(BlueprintCallable, Category = "VTC|OperatorView")
	void DisconnectSpectatorScreen();

	// 캡처 위치를 지정 Actor 위 상공으로 이동 (차량 중심을 기준으로 잡을 때)
	UFUNCTION(BlueprintCallable, Category = "VTC|OperatorView")
	void SetCaptureTarget(AActor* TargetActor, float HeightOffset = 300.0f);

private:
	bool bSpectatorConnected = false;
};
