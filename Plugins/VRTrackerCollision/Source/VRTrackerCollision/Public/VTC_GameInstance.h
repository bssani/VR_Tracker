// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_GameInstance.h — 세션 설정 보관 + INI 저장/불러오기
//
// [역할]
//   FVTCSessionConfig를 보관하고 INI로 영속 저장/불러오기한다.
//   VR 맵은 직접 실행 → Init()에서 INI 자동 로드 → OperatorController 적용.
//   레벨 전환 기능은 없음.
//
// [사용법]
//   1. Project Settings → Maps & Modes → Game Instance Class = BP_VTC_GameInstance
//   2. 설정 위젯에서 [Save Config] → INI 저장
//   3. VR 맵 직접 실행 → Init() 자동 로드 → 모든 설정 복원

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
  // ─── 현재 세션 설정 ────────────────────────────────────────────────────────
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Session")
  FVTCSessionConfig SessionConfig;

  // ─── 라이프사이클 ──────────────────────────────────────────────────────────

  // 게임 인스턴스 초기화 시 INI 자동 로드.
  virtual void Init() override;

  // ─── INI 저장 / 불러오기 ──────────────────────────────────────────────────

  // 현재 SessionConfig → Config/VTCSettings.ini 저장
  UFUNCTION(BlueprintCallable, Category = "VTC|Config")
  void SaveConfigToINI();

  // Config/VTCSettings.ini → SessionConfig 불러오기 (preset JSON 재로드 포함)
  UFUNCTION(BlueprintCallable, Category = "VTC|Config")
  void LoadConfigFromINI();

private:
  // INI 섹션명 상수
  static constexpr const TCHAR* INI_SECTION = TEXT("VTCSettings");

  // INI 파일 절대 경로 반환 (ProjectDir/Config/VTCSettings.ini)
  FString GetINIPath() const;
};
