// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_OperatorController.h — VR 세션 중 데스크탑 오퍼레이터용 PlayerController
//
// [목적]
//   VR HMD를 착용한 피실험자가 움직이는 동안,
//   데스크탑에 앉은 오퍼레이터(실험자)가 마우스로 위젯을 조작하여
//   세션 시작 / 캘리브레이션 / 데이터 내보내기를 제어한다.
//
//   HMD가 있어도 없어도 같은 방식으로 동작하므로
//   VR 세션과 시뮬레이션 세션 모두 이 컨트롤러를 기본값으로 사용해도 된다.
//   (시뮬레이션 이동 키입력이 필요한 경우 VTC_SimPlayerController 사용)
//
// [사용법]
//   1. 이 클래스를 부모로 BP 생성 (BP_VTC_OperatorController 권장)
//   2. BP Details > "VTC|Operator|UI" 에서 SubjectInfoWidgetClass 연결
//      (WBP_VTC_SubjectInfo)
//   3. GameMode > PlayerControllerClass = BP_VTC_OperatorController
//
// [데이터 흐름]
//   SubjectInfoWidget → OnSessionStartRequested → SessionManager->StartSessionWithHeight()
//
// [단축키] (BP에서 추가하거나 여기서 바인딩)
//   F1 : 캘리브레이션 시작
//   F2 : 테스트 시작
//   F3 : 세션 종료 + CSV 내보내기

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VTC_OperatorController.generated.h"

class UVTC_SubjectInfoWidget;
class AVTC_SessionManager;

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "VTC Operator Controller"))
class VRTRACKERCOLLISION_API AVTC_OperatorController : public APlayerController
{
  GENERATED_BODY()

public:
  // ─── BP에서 연결할 에셋 ───────────────────────────────────────────────────

  // 데스크탑 뷰포트에 표시할 위젯 클래스 (WBP_VTC_SubjectInfo)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Operator|UI",
            meta = (DisplayName = "Subject Info Widget Class"))
  TSubclassOf<UVTC_SubjectInfoWidget> SubjectInfoWidgetClass;

  // 세션 제어 대상 — nullptr이면 BeginPlay에서 레벨에서 자동 탐색
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Operator",
            meta = (DisplayName = "Session Manager"))
  TObjectPtr<AVTC_SessionManager> SessionManager;

  // ─── 런타임 참조 (읽기 전용) ──────────────────────────────────────────────

  UPROPERTY(BlueprintReadOnly, Category = "VTC|Operator|UI")
  TObjectPtr<UVTC_SubjectInfoWidget> SubjectInfoWidget;

  // ─── Blueprint 호출 가능 함수 ─────────────────────────────────────────────

  // 위젯 보이기 / 숨기기 (BP 또는 세션 상태 변화 시 호출)
  UFUNCTION(BlueprintCallable, Category = "VTC|Operator|UI")
  void ShowSubjectInfoWidget();

  UFUNCTION(BlueprintCallable, Category = "VTC|Operator|UI")
  void HideSubjectInfoWidget();

  // 단축키 핸들러 — BP에서 Override 가능
  UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VTC|Operator|Session")
  void StartCalibration();

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VTC|Operator|Session")
  void StartTest();

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "VTC|Operator|Session")
  void StopAndExport();

protected:
  virtual void BeginPlay() override;
  virtual void SetupInputComponent() override;

private:
  void Input_F1();
  void Input_F2();
  void Input_F3();

  // SessionManager 자동 탐색 (레벨에서 첫 번째 AVTC_SessionManager를 찾는다)
  void AutoFindSessionManager();

  // 위젯의 OnSessionStartRequested 델리게이트에 바인딩되는 핸들러
  UFUNCTION()
  void OnSessionStartRequested(const FString& SubjectID, float Height_cm);
};
