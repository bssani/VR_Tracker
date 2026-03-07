// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_OperatorMonitorWidget.h — 운영자 데스크탑 모니터링 대시보드 (Screen Space)
//
// [역할]
//   Level 2에서 운영자(데스크탑)가 볼 수 있는 Screen Space UI.
//   세션 상태 + 피실험자 정보 + 거리 측정값 + 최소 거리 + 경과 시간을 실시간 표시.
//   프로파일 드롭다운으로 피실험자+차량 조합을 선택해 즉시 적용 가능.
//
// [BP 연결 필수 위젯 이름]
//   Txt_State         TextBlock  — 세션 상태 (● TESTING 등)
//   Txt_SubjectInfo   TextBlock  — Subject ID + Height
//   Txt_TrackerStatus TextBlock  — 트래커 연결 수
//   Txt_ElapsedTime   TextBlock  — 경과 시간 (MM:SS)
//   Txt_MinDistance   TextBlock  — 세션 최솟값 거리 (Min: 4.2 cm)
//   VB_DistanceList   VerticalBox — 거리 Row (TextBlock, Map 재사용)
//
// [BP 연결 선택 위젯 이름]
//   Txt_PresetInfo         TextBlock      — 적용된 차종 프리셋 이름
//   Txt_HipWaistDistance   TextBlock      — Hip ↔ Waist 실시간 거리
//   Combo_ProfileSelect    ComboBoxString — VRTestLevel 프로파일 드롭다운
//   Btn_ApplyProfile       Button         — 선택한 프로파일 즉시 적용
//   CB_TrackerMeshVisible  CheckBox       — Tracker 3D 메시 표시/숨김 토글
//
// [사용법]
//   VTC_OperatorController가 BeginPlay에서 생성 → AddToViewport.
//   CollisionDetector.OnDistanceUpdated, SessionManager.OnSessionStateChanged에 바인딩됨.
//   Btn_ApplyProfile 클릭 → OnProfileApplied 델리게이트 → OperatorController.ApplyGameInstanceConfig 재실행.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "Data/VTC_SessionManager.h"
#include "VTC_OperatorMonitorWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UButton;
class UCheckBox;

// 프로파일 적용 완료 시 OperatorController에 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVTCProfileApplied);

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "VTC Operator Monitor Widget"))
class VRTRACKERCOLLISION_API UVTC_OperatorMonitorWidget : public UUserWidget
{
  GENERATED_BODY()

public:
  // ─── BindWidget (Blueprint에서 이름 정확히 일치) ──────────────────────────

  UPROPERTY(meta = (BindWidget))         TObjectPtr<UTextBlock>    Txt_State;
  UPROPERTY(meta = (BindWidget))         TObjectPtr<UTextBlock>    Txt_SubjectInfo;
  UPROPERTY(meta = (BindWidget))         TObjectPtr<UTextBlock>    Txt_TrackerStatus;
  UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock>    Txt_PresetInfo;
  UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock>    Txt_HipWaistDistance;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock>    Txt_ElapsedTime;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock>    Txt_MinDistance;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UVerticalBox>  VB_DistanceList;

  // ─── 프로파일 선택 (VRTestLevel 전용) ─────────────────────────────────────
  UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> Combo_ProfileSelect;
  UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton>         Btn_ApplyProfile;

  // ─── Tracker 메시 가시성 토글 ─────────────────────────────────────────────
  UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> CB_TrackerMeshVisible;

  // ─── 프로파일 적용 완료 델리게이트 (OperatorController가 바인딩) ───────────
  UPROPERTY(BlueprintAssignable, Category = "VTC|Profile")
  FOnVTCProfileApplied OnProfileApplied;

  // 프로파일 목록 갱신 (BeginPlay + 외부 갱신 시 호출)
  UFUNCTION(BlueprintCallable, Category = "VTC|Profile")
  void RefreshProfileComboBox();

  // ─── 외부에서 호출하는 업데이트 함수 (OperatorController에서 호출) ──────────

  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void UpdateState(EVTCSessionState NewState);

  // 적용된 프리셋 이름 표시 (Txt_PresetInfo 있을 때만 작동)
  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void UpdatePresetInfo(bool bUsePreset, const FString& PresetName);

  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void UpdateSubjectInfo(const FString& SubjectID, float Height_cm);

  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void UpdateTrackerStatus(int32 ConnectedCount, int32 TotalCount);

  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void UpdateElapsedTime(float ElapsedSeconds);

  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void UpdateMinDistance(float MinDist_cm);

  // Hip ReferencePoint ↔ Waist 실시간 거리 (경고 이벤트 없음)
  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void UpdateHipWaistDistance(float Distance_cm);

  // 거리 측정 결과 1건 갱신 (30Hz OnDistanceUpdated에서 호출)
  // Row TextBlock을 Map으로 재사용 — ClearChildren 없음
  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void UpdateDistanceRow(const FVTCDistanceResult& Result);

  // 상태 전환 시 거리 목록 초기화 (Testing 시작 전 등)
  UFUNCTION(BlueprintCallable, Category = "VTC|Monitor")
  void ClearDistanceList();

protected:
  virtual void NativeConstruct() override;

private:
  // 프로파일 Apply 버튼 핸들러
  UFUNCTION() void OnApplyProfileClicked();
  // TrackerMesh 체크박스 변경 핸들러
  UFUNCTION() void OnTrackerMeshVisibilityChanged(bool bIsChecked);

  // (BodyPart + "_" + VehiclePartName) → TextBlock 재사용 맵
  TMap<FString, TObjectPtr<UTextBlock>> DistanceRowMap;

  // 같은 Key → 거리 원본 값 저장 (최솟값 계산용)
  TMap<FString, float> DistanceValueMap;

  // DistanceValueMap에서 최솟값을 구해 Txt_MinDistance를 갱신
  void UpdateMinDistanceFromMap();

  static FString MakeRowKey(EVTCTrackerRole BodyPart, const FString& VehiclePartName);
  static FString GetBodyPartLabel(EVTCTrackerRole Role);
  static FString GetStateLabel(EVTCSessionState State);
};
