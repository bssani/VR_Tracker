// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_SetupGameMode.h — Level 1 (Setup) 전용 GameMode
//
// [역할]
//   Level 1에서 SetupWidget을 화면에 추가(AddToViewport)하고
//   마우스 커서를 보여준다. Pawn은 스폰하지 않는다.
//
// [설정 방법]
//   1. BP_VTC_SetupGameMode 생성 → SetupWidgetClass = WBP_SetupWidget 지정
//   2. Level 1 World Settings > GameMode Override = BP_VTC_SetupGameMode
//
// [흐름]
//   BeginPlay → LoadConfigFromINI → AddWidget → 마우스 커서 ON
//   Start Session 버튼 클릭 → GameInstance → OpenTestLevel (Level 2 이동)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UI/VTC_SetupWidget.h"
#include "VTC_SetupGameMode.generated.h"

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "VTC Setup Game Mode"))
class VRTRACKERCOLLISION_API AVTC_SetupGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AVTC_SetupGameMode();

	// ─── 설정 ────────────────────────────────────────────────────────────────
	// BP에서 WBP_SetupWidget 지정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Setup",
		meta = (DisplayName = "Setup Widget Class"))
	TSubclassOf<UVTC_SetupWidget> SetupWidgetClass;

	// ─── 런타임 접근 ─────────────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Setup")
	UVTC_SetupWidget* GetSetupWidget() const { return SetupWidget; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UVTC_SetupWidget> SetupWidget;
};
