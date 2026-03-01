// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_StatusActor.h — Level 2에 배치하는 3D 상태 표시 액터
//
// [역할]
//   UWidgetComponent(WorldSpace)를 통해 VR/데스크탑 모두에서
//   현재 세션 상태와 키 입력 안내를 3D 공간에 표시한다.
//
// [사용법]
//   1. Level 2에 이 액터(또는 BP_VTC_StatusActor)를 배치
//   2. BP Details > StatusWidgetClass = WBP_VTC_StatusWidget 지정
//   3. OperatorController가 BeginPlay에서 이 액터를 자동으로 찾아 연결한다.
//
// [크기/위치]
//   차량 운전석 옆 혹은 전면 대시보드 위에 배치 권장.
//   WidgetComponent DrawSize = 800×400 (cm 단위로 WorldSpace 렌더링).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VTC_StatusActor.generated.h"

class UWidgetComponent;
class UVTC_StatusWidget;

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "VTC Status Actor"))
class VRTRACKERCOLLISION_API AVTC_StatusActor : public AActor
{
  GENERATED_BODY()

public:
  AVTC_StatusActor();

  // ─── 컴포넌트 ───────────────────────────────────────────────────────────
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VTC|Status")
  TObjectPtr<UWidgetComponent> StatusWidgetComponent;

  // ─── 설정 ────────────────────────────────────────────────────────────────
  // BP에서 WBP_VTC_StatusWidget 지정
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Status",
            meta = (DisplayName = "Status Widget Class"))
  TSubclassOf<UVTC_StatusWidget> StatusWidgetClass;

  // ─── 런타임 접근 ─────────────────────────────────────────────────────────
  // OperatorController가 이 함수로 StatusWidget에 접근해 업데이트
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Status")
  UVTC_StatusWidget* GetStatusWidget() const;

protected:
  virtual void BeginPlay() override;
};
