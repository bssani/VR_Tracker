// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_OperatorMonitorWidget.h — 운영자 데스크탑 모니터링 대시보드 (Screen Space)
//
// [역할]
//   Level 2에서 운영자(데스크탑)가 볼 수 있는 Screen Space UI.
//   세션 상태 + 피실험자 정보 + 거리 측정값 + 최소 거리 + 경과 시간을 실시간 표시.
//
// [BP 연결 필수 위젯 이름]
//   Txt_State         TextBlock  — 세션 상태 (● TESTING 등)
//   Txt_SubjectInfo   TextBlock  — Subject ID + Height
//   Txt_TrackerStatus TextBlock  — 트래커 연결 수
//   Txt_ElapsedTime   TextBlock  — 경과 시간 (MM:SS)
//   Txt_MinDistance   TextBlock  — 세션 최솟값 거리 (Min: 4.2 cm)
//   VB_DistanceList   VerticalBox — 거리 Row (TextBlock, Map 재사용)
//
// [사용법]
//   VTC_OperatorController가 BeginPlay에서 생성 → AddToViewport.
//   CollisionDetector.OnDistanceUpdated, SessionManager.OnSessionStateChanged에 바인딩됨.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "Data/VTC_SessionManager.h"
#include "VTC_OperatorMonitorWidget.generated.h"

class UTextBlock;
class UVerticalBox;

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "VTC Operator Monitor Widget"))
class VRTRACKERCOLLISION_API UVTC_OperatorMonitorWidget : public UUserWidget
{
  GENERATED_BODY()

public:
  // ─── BindWidget (Blueprint에서 이름 정확히 일치) ──────────────────────────

  UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock>    Txt_State;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock>    Txt_SubjectInfo;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock>    Txt_TrackerStatus;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock>    Txt_ElapsedTime;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock>    Txt_MinDistance;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UVerticalBox>  VB_DistanceList;

  // ─── 외부에서 호출하는 업데이트 함수 (OperatorController에서 호출) ──────────

  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void UpdateState(EVTCSessionState NewState);

  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void UpdateSubjectInfo(const FString& SubjectID, float Height_cm);

  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void UpdateTrackerStatus(int32 ConnectedCount, int32 TotalCount);

  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void UpdateElapsedTime(float ElapsedSeconds);

  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void UpdateMinDistance(float MinDist_cm);

  // 거리 측정 결과 1건 갱신 (30Hz OnDistanceUpdated에서 호출)
  // Row TextBlock을 Map으로 재사용 — ClearChildren 없음
  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void UpdateDistanceRow(const FVTCDistanceResult& Result);

  // 상태 전환 시 거리 목록 초기화 (Testing 시작 전 등)
  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void ClearDistanceList();

private:
  // (BodyPart + "_" + VehiclePartName) → TextBlock 재사용 맵
  TMap<FString, TObjectPtr<UTextBlock>> DistanceRowMap;

  static FString MakeRowKey(EVTCTrackerRole BodyPart, const FString& VehiclePartName);
  static FString GetBodyPartLabel(EVTCTrackerRole Role);
  static FString GetStateLabel(EVTCSessionState State);
};
