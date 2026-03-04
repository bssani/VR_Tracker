// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_GameInstance.h — 레벨 간 설정 전달 + INI 저장/불러오기
//
// [역할]
//   Level 1 (Setup)에서 작성한 FVTCSessionConfig를 보관하고,
//   Level 2 (Test)가 로드된 뒤 OperatorController / BodyActor / TrackerPawn에
//   설정을 일괄 적용한다.
//   INI 파일(Config/VTCSettings.ini)로 설정을 영속 저장/불러오기한다.
//
// [사용법]
//   1. 프로젝트 Settings > Maps & Modes > Game Instance Class =
//      BP_VTC_GameInstance (또는 VTC_GameInstance 직접)
//   2. Level 1 위젯 → GetGameInstance<UVTC_GameInstance>()->SessionConfig 채우기
//   3. Level 2 로드 후 OperatorController::BeginPlay에서 ApplyConfigToWorld() 호출

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
  // Level 1에서 채운 뒤 Level 2에서 읽는다.
  UPROPERTY(BlueprintReadWrite, Category = "VTC|Session")
  FVTCSessionConfig SessionConfig;

  // ─── 라이프사이클 ──────────────────────────────────────────────────────────

  // 게임 인스턴스 초기화 시 INI 자동 로드.
  // Level 1 없이 VR/Sim 레벨을 직접 실행할 때도 이전 설정을 사용할 수 있다.
  virtual void Init() override;

  // ─── 레벨 전환 ─────────────────────────────────────────────────────────────

  // 테스트 레벨 열기.
  // RunMode에 따라 VRTestLevelName 또는 SimTestLevelName으로 자동 분기한다.
  UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VTC|Session")
  void OpenTestLevel();

  // Level 1 (설정 레벨) 로 돌아가기
  UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VTC|Session")
  void OpenSetupLevel();

  // ─── INI 저장 / 불러오기 ──────────────────────────────────────────────────

  // 현재 SessionConfig → Config/VTCSettings.ini 저장
  // (SubjectID / Height 제외 — 매 세션마다 입력하는 값)
  UFUNCTION(BlueprintCallable, Category = "VTC|Config")
  void SaveConfigToINI();

  // Config/VTCSettings.ini → SessionConfig 불러오기
  // (SubjectID / Height는 건드리지 않음)
  UFUNCTION(BlueprintCallable, Category = "VTC|Config")
  void LoadConfigFromINI();

  // ─── 레벨 이름 (BP에서 Override해 실제 레벨 이름 지정) ──────────────────

  // VR 전용 테스트 레벨 이름 (RunMode = VR 일 때 OpenTestLevel에서 사용)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Session",
            meta = (DisplayName = "VR Test Level Name"))
  FString VRTestLevelName = TEXT("VTC_VRTestLevel");

  // 시뮬레이션 전용 테스트 레벨 이름 (RunMode = Simulation 일 때 사용)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Session",
            meta = (DisplayName = "Sim Test Level Name"))
  FString SimTestLevelName = TEXT("VTC_SimTestLevel");

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Session",
            meta = (DisplayName = "Setup Level Name"))
  FString SetupLevelName = TEXT("VTC_SetupLevel");

private:
  // INI 섹션명 상수
  static constexpr const TCHAR* INI_SECTION = TEXT("VTC/Settings");

  // INI 파일 절대 경로 반환 (ProjectDir/Config/VTCSettings.ini)
  FString GetINIPath() const;

  // FVector → INI 읽기/쓰기 헬퍼
  void SaveVector(const TCHAR* Key, const FVector& V, const FString& Path) const;
  FVector LoadVector(const TCHAR* Key, const FVector& Default, const FString& Path) const;
};
