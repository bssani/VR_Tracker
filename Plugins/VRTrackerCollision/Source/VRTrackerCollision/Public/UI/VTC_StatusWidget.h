// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_StatusWidget.h — Level 2 월드 배치 상태 표시 위젯 (3D World Space)
//
// [BP 연결 필수 위젯 이름]
//   Txt_State        TextBlock — 현재 세션 상태 (Idle / Calibrating / Testing / Reviewing)
//   Txt_Prompt       TextBlock — 키 입력 안내 메시지 ("Press 1 to Calibrate" 등)
//   Txt_SubjectInfo  TextBlock — 피실험자 ID + 키 정보
//   Txt_TrackerStatus TextBlock — 트래커 연결 상태 요약
//
// [BP 연결 선택 위젯 이름 — VR 실시간 데이터 표시용]
//   Txt_PresetInfo   TextBlock — 적용된 차종 프리셋 이름
//   Txt_CalibResult  TextBlock — 캘리브레이션 결과 (OK / FAILED)
//   Txt_ElapsedTime  TextBlock — 경과 시간 ("02:34")
//   Txt_MinDistance  TextBlock — 세션 최소 거리 ("Min: 4.2 cm")
//   VB_DistanceList  VerticalBox — 기준점별 거리 Row 자동 생성 영역
//
// [사용법]
//   VTC_StatusActor에 부착된 WidgetComponent의 Widget Class로 지정.
//   VTC_OperatorController가 세션 상태 변경 및 거리 측정 결과를 push한다.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Data/VTC_SessionManager.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "VTC_StatusWidget.generated.h"


class UTextBlock;
class UVerticalBox;

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "VTC Status Widget"))
class VRTRACKERCOLLISION_API UVTC_StatusWidget : public UUserWidget {
  GENERATED_BODY()

public:
  // ─── 필수 BindWidget ─────────────────────────────────────────────────────
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_State;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_Prompt;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_SubjectInfo;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_TrackerStatus;

  // ─── 선택 BindWidget (VR 실시간 데이터 — 없어도 컴파일 오류 없음) ────────
  UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock>   Txt_PresetInfo;
  UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock>   Txt_CalibResult;
  UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock>   Txt_ElapsedTime;
  UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock>   Txt_MinDistance;
  UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> VB_DistanceList;

  // ─── 외부에서 호출하는 업데이트 함수 ─────────────────────────────────────

  UFUNCTION(BlueprintCallable, Category = "VTC|Status")
  void UpdateState(EVTCSessionState NewState);

  UFUNCTION(BlueprintCallable, Category = "VTC|Status")
  void UpdateSubjectInfo(const FString &SubjectID, float Height_cm);

  UFUNCTION(BlueprintCallable, Category = "VTC|Status")
  void UpdateTrackerStatus(int32 ConnectedCount, int32 TotalCount);

  // ─── VR 실시간 데이터 업데이트 (선택 위젯이 없으면 no-op) ─────────────────

  // 적용된 프리셋 이름 표시 (없으면 "No Preset")
  UFUNCTION(BlueprintCallable, Category = "VTC|Status")
  void UpdatePresetInfo(bool bUsePreset, const FString& PresetName);

  // 캘리브레이션 결과 표시 (Reason 빈 문자열 = 텍스트 클리어)
  UFUNCTION(BlueprintCallable, Category = "VTC|Status")
  void UpdateCalibrationResult(bool bSuccess, const FString& Reason);

  // 경과 시간 갱신 (MM:SS 포맷)
  UFUNCTION(BlueprintCallable, Category = "VTC|Status")
  void UpdateElapsedTime(float ElapsedSeconds);

  // 세션 최소 거리 갱신
  UFUNCTION(BlueprintCallable, Category = "VTC|Status")
  void UpdateMinDistance(float MinDist_cm);

  // 기준점별 거리 Row 갱신 — OperatorMonitorWidget과 동일한 Map 재사용 방식
  UFUNCTION(BlueprintCallable, Category = "VTC|Status")
  void UpdateDistanceRow(const FVTCDistanceResult &Result);

  // 거리 목록 초기화 (Testing 시작 시 호출)
  UFUNCTION(BlueprintCallable, Category = "VTC|Status")
  void ClearDistanceList();

private:
  // (BodyPart + "_" + VehiclePartName) → TextBlock 재사용 맵
  TMap<FString, TObjectPtr<UTextBlock>> DistanceRowMap;

  static FString MakeRowKey(EVTCTrackerRole BodyPart, const FString &VehiclePartName);
  static FString GetBodyPartLabel(EVTCTrackerRole Role);
  static FString GetPromptForState(EVTCSessionState State);
  static FString GetStateLabel(EVTCSessionState State);
};
