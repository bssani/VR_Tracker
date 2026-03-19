// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_GameInstance.h — INI 기반 세션 설정 관리
//
// [역할]
//   FVTCSessionConfig를 보관하고, Init()에서 INI 파일을 자동 로드한다.
//   OperatorController::BeginPlay에서 SessionConfig를 읽어
//   TrackerPawn / BodyActor / CollisionDetector에 설정을 일괄 적용한다.
//   INI 파일(Config/VTCSettings.ini)로 설정을 영속 저장/불러오기한다.
//
// [사용법]
//   1. 프로젝트 Settings > Maps & Modes > Game Instance Class =
//      BP_VTC_GameInstance (또는 VTC_GameInstance 직접)
//   2. OperatorController::BeginPlay에서 ApplyConfigToWorld() 호출

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "VTC_SessionConfig.h"
#include "VTC_GameInstance.generated.h"

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "VTC Game Instance"))
class VRTRACKERCOLLISION_API UVTC_GameInstance : public UGameInstance
{
  GENERATED_BODY()

public:
  // 엔진 Init 시 INI 자동 로드
  virtual void Init() override;

  // ─── 현재 세션 설정 ────────────────────────────────────────────────────────
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Session")
  FVTCSessionConfig SessionConfig;

  // ─── INI 저장 / 불러오기 ──────────────────────────────────────────────────

  // 현재 SessionConfig → Config/VTCSettings.ini 저장
  // (SubjectID / Height 제외 — 매 세션마다 입력하는 값)
  UFUNCTION(BlueprintCallable, Category = "VTC|Config")
  void SaveConfigToINI();

  // Config/VTCSettings.ini → SessionConfig 불러오기
  // (SubjectID / Height는 건드리지 않음)
  UFUNCTION(BlueprintCallable, Category = "VTC|Config")
  void LoadConfigFromINI();

private:
  // INI 섹션명 상수
  static constexpr const TCHAR* INI_SECTION = TEXT("VTC/Settings");

  // INI 파일 절대 경로 반환 (ProjectDir/Config/VTCSettings.ini)
  FString GetINIPath() const;

  // FVector → INI 읽기/쓰기 헬퍼
  void SaveVector(const TCHAR* Key, const FVector& V, const FString& Path) const;
  FVector LoadVector(const TCHAR* Key, const FVector& Default, const FString& Path) const;
};
