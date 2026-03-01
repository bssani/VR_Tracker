// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_StatusWidget.h — Level 2 월드 배치 상태 표시 위젯 (3D World Space)
//
// [BP 연결 필수 위젯 이름]
//   Txt_State        TextBlock — 현재 세션 상태 (Idle / Calibrating / Testing / Reviewing)
//   Txt_Prompt       TextBlock — 키 입력 안내 메시지 ("Press F1 to Calibrate" 등)
//   Txt_SubjectInfo  TextBlock — 피실험자 ID + 키 정보
//   Txt_TrackerStatus TextBlock — 트래커 연결 상태 요약
//
// [사용법]
//   VTC_StatusActor에 부착된 WidgetComponent의 Widget Class로 지정.
//   VTC_OperatorController가 세션 상태 변경 시 UpdateStatus()를 호출한다.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/VTC_SessionManager.h"
#include "VTC_StatusWidget.generated.h"

class UTextBlock;

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "VTC Status Widget"))
class VRTRACKERCOLLISION_API UVTC_StatusWidget : public UUserWidget
{
  GENERATED_BODY()

public:
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_State;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_Prompt;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_SubjectInfo;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_TrackerStatus;

  // ─── 외부에서 호출하는 업데이트 함수 ─────────────────────────────────────

  // 세션 상태가 바뀔 때 호출 — 상태 텍스트 + 키 안내 메시지 갱신
  UFUNCTION(BlueprintCallable, Category = "VTC|Status")
  void UpdateState(EVTCSessionState NewState);

  // 피실험자 정보 표시 갱신
  UFUNCTION(BlueprintCallable, Category = "VTC|Status")
  void UpdateSubjectInfo(const FString& SubjectID, float Height_cm);

  // 트래커 연결 상태 텍스트 갱신 (예: "5/5 Connected")
  UFUNCTION(BlueprintCallable, Category = "VTC|Status")
  void UpdateTrackerStatus(int32 ConnectedCount, int32 TotalCount);

private:
  // 세션 상태별 키 안내 메시지 반환
  static FString GetPromptForState(EVTCSessionState State);

  // 세션 상태 → 표시 문자열
  static FString GetStateLabel(EVTCSessionState State);
};
