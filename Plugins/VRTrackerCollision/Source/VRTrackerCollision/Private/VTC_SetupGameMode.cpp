// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "VTC_SetupGameMode.h"
#include "VTC_GameInstance.h"
#include "GameFramework/PlayerController.h"

AVTC_SetupGameMode::AVTC_SetupGameMode()
{
	// Level 1은 Pawn이 필요 없다.
	// 마우스로 UI를 조작하기 위해 DefaultPawn을 스폰하지 않음.
	DefaultPawnClass = nullptr;
}

void AVTC_SetupGameMode::BeginPlay()
{
	Super::BeginPlay();

	// ── 마우스 커서 ON + UI 입력 모드 ────────────────────────────────────────
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		PC->bShowMouseCursor = true;
		FInputModeUIOnly InputMode;
		PC->SetInputMode(InputMode);
	}

	// ── SetupWidget 생성 → 뷰포트에 추가 ────────────────────────────────────
	// SetupWidgetClass는 BP_VTC_SetupGameMode에서 WBP_SetupWidget으로 지정.
	// NativeConstruct에서 INI 자동 로드 + PopulateFromConfig가 실행된다.
	if (SetupWidgetClass && PC)
	{
		SetupWidget = CreateWidget<UVTC_SetupWidget>(PC, SetupWidgetClass);
		if (SetupWidget)
		{
			SetupWidget->AddToViewport(0);
			UE_LOG(LogTemp, Log, TEXT("[VTC] SetupGameMode: SetupWidget added to viewport."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[VTC] SetupGameMode: SetupWidgetClass가 지정되지 않았습니다. "
			     "BP_VTC_SetupGameMode > SetupWidgetClass = WBP_SetupWidget 로 설정하세요."));
	}
}

void AVTC_SetupGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Level 2로 전환 시 SetupWidget을 명시적으로 정리.
	// RemoveFromParent()는 Viewport에서 제거 + GC 가능 상태로 만든다.
	if (SetupWidget)
	{
		SetupWidget->RemoveFromParent();
		SetupWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}
