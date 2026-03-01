// Copyright GMTCK PQDQ Team. All Rights Reserved.

#include "World/VTC_StatusActor.h"
#include "UI/VTC_StatusWidget.h"
#include "Components/WidgetComponent.h"

AVTC_StatusActor::AVTC_StatusActor()
{
  PrimaryActorTick.bCanEverTick = false;

  StatusWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("StatusWidgetComponent"));
  RootComponent = StatusWidgetComponent;

  // WorldSpace 렌더링 — 3D 공간에 물리적으로 배치됨 (VR/데스크탑 둘 다 보임)
  StatusWidgetComponent->SetWidgetSpace(EWidgetSpace::World);

  // 기본 크기 (cm) — 레벨 배치 후 조정 가능
  StatusWidgetComponent->SetDrawSize(FVector2D(800.f, 400.f));

  // 위젯이 항상 카메라를 향하지 않도록 설정 (고정 월드 배치)
  StatusWidgetComponent->SetTwoSided(true);
}

void AVTC_StatusActor::BeginPlay()
{
  Super::BeginPlay();

  if (StatusWidgetClass)
    StatusWidgetComponent->SetWidgetClass(StatusWidgetClass);
}

UVTC_StatusWidget* AVTC_StatusActor::GetStatusWidget() const
{
  return Cast<UVTC_StatusWidget>(StatusWidgetComponent->GetUserWidgetObject());
}
