// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_OperatorController.h — Level 2 테스트 진행용 PlayerController
//
// [역할]
//   - GameInstance에서 세션 설정을 읽어 TrackerPawn/BodyActor에 적용
//   - F1/F2/F3 키로만 세션 제어 (버튼 없음)
//   - Escape 키로 Level 1 (Setup)으로 복귀
//   - StatusActor(월드 3D 위젯)에 현재 상태 + 키 안내 메시지 표시
//   - Tick에서 1초마다 TrackerStatus 갱신
//   - VehicleHipPosition ReferencePoint를 레벨에 동적 스폰
//
// [단축키]
//   F1     : 캘리브레이션 시작 (GameInstance의 SubjectID/Height 사용)
//   F2     : 테스트 시작 (캘리브레이션 건너뜀)
//   F3     : 세션 종료 + CSV 내보내기
//   Escape : Level 1 (Setup 레벨) 로 복귀

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Data/VTC_SessionManager.h"
#include "Vehicle/VTC_ReferencePoint.h"
#include "VTC_SessionConfig.h"
#include "VTC_OperatorController.generated.h"

class UVTC_SubjectInfoWidget;
class AVTC_StatusActor;

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "VTC Operator Controller"))
class VRTRACKERCOLLISION_API AVTC_OperatorController : public APlayerController
{
  GENERATED_BODY()

public:
  AVTC_OperatorController();

  // ─── 수동 연결 (없으면 BeginPlay에서 자동 탐색) ──────────────────────────
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Operator")
  TObjectPtr<AVTC_SessionManager> SessionManager;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Operator")
  TObjectPtr<AVTC_StatusActor> StatusActor;

  // Level 2에서는 사용 안 하지만 이전 호환을 위해 유지
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Operator|UI")
  TSubclassOf<UVTC_SubjectInfoWidget> SubjectInfoWidgetClass;

  UPROPERTY(BlueprintReadOnly, Category = "VTC|Operator|UI")
  TObjectPtr<UVTC_SubjectInfoWidget> SubjectInfoWidget;

  // ─── Blueprint 호출 가능 ──────────────────────────────────────────────────
  UFUNCTION(BlueprintCallable, Category = "VTC|Operator|UI")
  void ShowSubjectInfoWidget();

  UFUNCTION(BlueprintCallable, Category = "VTC|Operator|UI")
  void HideSubjectInfoWidget();

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VTC|Operator|Session")
  void StartCalibration();

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VTC|Operator|Session")
  void StartTest();

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VTC|Operator|Session")
  void StopAndExport();

  // Level 1 (Setup 레벨)으로 즉시 복귀
  UFUNCTION(BlueprintCallable, Category = "VTC|Operator|Session")
  void ReturnToSetupLevel();

protected:
  virtual void BeginPlay() override;
  virtual void Tick(float DeltaTime) override;
  virtual void SetupInputComponent() override;

  // Pawn이 possess된 직후 호출 — BeginPlay에서 GetPawn()이 null일 경우 여기서 설정 적용
  virtual void OnPossess(APawn* InPawn) override;

private:
  void Input_F1();
  void Input_F2();
  void Input_F3();
  void Input_Escape();

  // GameInstance 설정 → TrackerPawn + BodyActor + CollisionDetector 에 일괄 적용
  void ApplyGameInstanceConfig();

  void AutoFindSessionManager();
  void AutoFindStatusActor();

  // VehicleHipPosition을 위해 동적 스폰한 ReferencePoint
  // ApplyGameInstanceConfig에서 생성, BeginDestroy 시 자동 제거
  UPROPERTY()
  TObjectPtr<AVTC_ReferencePoint> SpawnedHipRefPoint;

  // TrackerStatus 갱신 타이머 (1초마다 업데이트)
  float TrackerStatusTimer = 0.0f;
  static constexpr float TrackerStatusInterval = 1.0f;

  // BeginPlay와 OnPossess 중 먼저 호출된 쪽에서만 설정 적용 (중복 방지)
  bool bConfigApplied = false;

  // SessionManager 상태 변경 → StatusWidget 갱신
  UFUNCTION()
  void OnSessionStateChanged(EVTCSessionState OldState, EVTCSessionState NewState);

  // (Level 1 호환용, Level 2에서는 사용 안 함)
  UFUNCTION()
  void OnSessionStartRequested(const FString& SubjectID, float Height_cm);
};
