// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_OperatorController.h — Level 2 테스트 진행용 PlayerController
//
// [역할]
//   - GameInstance에서 세션 설정을 읽어 TrackerPawn/BodyActor에 적용
//   - 1/2/3 키로만 세션 제어 (버튼 없음)
//   - 3 키로 CSV 저장 후 게임 종료 (레벨 전환 없음)
//   - StatusActor(월드 3D 위젯)에 현재 상태 + 키 안내 메시지 표시
//   - OperatorMonitorWidget(Screen Space)에 거리 데이터 + 상태 표시 (운영자
//   데스크탑용)
//   - Tick에서 1초마다 TrackerStatus + 경과 시간 갱신
//   - VehicleHipPosition ReferencePoint를 레벨에 동적 스폰
//
// [단축키]
//   1      : 캘리브레이션 시작 (GameInstance의 SubjectID/Height 사용)
//   2      : 테스트 시작 (캘리브레이션 건너뜀)
//   3      : CSV 저장 + 게임 종료 (QuitGame)
//   P      : INI에서 설정 재로드 + 전체 적용 (Level 1 설정 반영)

#pragma once

#include "CoreMinimal.h"
#include "Data/VTC_SessionManager.h"
#include "GameFramework/PlayerController.h"
#include "VTC_SessionConfig.h"
#include "Vehicle/VTC_ReferencePoint.h"
#include "VTC_OperatorController.generated.h"


class AVTC_StatusActor;
class AVTC_OperatorViewActor;
class UVTC_OperatorMonitorWidget;

UCLASS(BlueprintType, Blueprintable,
       meta = (DisplayName = "VTC Operator Controller"))
class VRTRACKERCOLLISION_API AVTC_OperatorController
    : public APlayerController {
  GENERATED_BODY()

public:
  AVTC_OperatorController();

  // ─── 수동 연결 (없으면 BeginPlay에서 자동 탐색) ──────────────────────────
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Operator")
  TObjectPtr<AVTC_SessionManager> SessionManager;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Operator")
  TObjectPtr<AVTC_StatusActor> StatusActor;

  // ─── 운영자 데스크탑 모니터링 위젯 (Screen Space) ─────────────────────────
  // WBP_VTC_OperatorMonitor (VTC_OperatorMonitorWidget 기반) 를 할당하세요.
  // 없으면 모니터링 위젯 없이 진행 (StatusActor 3D 위젯만 표시).
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Operator|Monitor")
  TSubclassOf<UVTC_OperatorMonitorWidget> OperatorMonitorWidgetClass;

  UPROPERTY(BlueprintReadOnly, Category = "VTC|Operator|Monitor")
  TObjectPtr<UVTC_OperatorMonitorWidget> OperatorMonitorWidget;

  // ─── Blueprint 호출 가능 ──────────────────────────────────────────────────
  UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
            Category = "VTC|Operator|Session")
  void StartCalibration();

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
            Category = "VTC|Operator|Session")
  void StartTest();

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
            Category = "VTC|Operator|Session")
  void StopAndExport();

protected:
  virtual void BeginPlay() override;
  virtual void Tick(float DeltaTime) override;
  virtual void SetupInputComponent() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  // Pawn이 possess된 직후 호출 — BeginPlay에서 GetPawn()이 null일 경우 여기서
  // 설정 적용
  virtual void OnPossess(APawn *InPawn) override;

private:
  void Input_One();
  void Input_Two();
  void Input_Three();
  void Input_P();

  // GameInstance 설정 → TrackerPawn + BodyActor + CollisionDetector 에 일괄
  // 적용
  void ApplyGameInstanceConfig();

  void AutoFindSessionManager();
  void AutoFindStatusActor();
  void AutoFindOperatorViewActor();

  // VehicleHipPosition을 위해 동적 스폰한 ReferencePoint
  UPROPERTY()
  TObjectPtr<AVTC_ReferencePoint> SpawnedHipRefPoint;

  // 차종 프리셋에서 스폰한 추가 ReferencePoint 목록 (Feature B)
  UPROPERTY()
  TArray<TObjectPtr<AVTC_ReferencePoint>> SpawnedPresetRefPoints;

  // Operator View Actor 참조 (Feature I)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Operator|View",
            meta = (AllowPrivateAccess = "true"))
  TObjectPtr<AVTC_OperatorViewActor> OperatorViewActor;

  // TrackerStatus + 경과시간 갱신 타이머 (1초마다 업데이트)
  float TrackerStatusTimer = 0.0f;
  static constexpr float TrackerStatusInterval = 1.0f;

  // SessionManager 상태 변경 → StatusWidget + OperatorMonitorWidget 갱신
  UFUNCTION()
  void OnSessionStateChanged(EVTCSessionState OldState,
                             EVTCSessionState NewState);

  // CollisionDetector 거리 측정 결과 → OperatorMonitorWidget Row 갱신
  UFUNCTION()
  void OnDistanceUpdated(const FVTCDistanceResult &Result);
};
