// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_SetupWidget.h — Level 1 설정 화면 위젯
//
// [BP 연결 필수 위젯 이름 목록]
//   피실험자 정보:
//     TB_SubjectID          EditableTextBox
//     TB_Height             EditableTextBox
//   모드:
//     Toggle_VRMode         CheckBox  (Checked=VR / Unchecked=Simulation, 기본 Checked)
//   Mount Offsets (트래커별 X/Y/Z):
//     TB_Offset_Waist_X/Y/Z      EditableTextBox
//     TB_Offset_LKnee_X/Y/Z     EditableTextBox
//     TB_Offset_RKnee_X/Y/Z     EditableTextBox
//     TB_Offset_LFoot_X/Y/Z     EditableTextBox
//     TB_Offset_RFoot_X/Y/Z     EditableTextBox
//   Vehicle Hip Reference:
//     TB_HipRef_X/Y/Z            EditableTextBox
//   가시성:
//     CB_ShowCollisionSpheres   CheckBox
//     CB_ShowTrackerMesh        CheckBox
//   버튼:
//     Btn_SaveConfig            Button
//     Btn_LoadConfig            Button
//     Btn_StartSession          Button

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VTC_SessionConfig.h"
#include "VTC_SetupWidget.generated.h"

class UEditableTextBox;
class UCheckBox;
class UButton;

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "VTC Setup Widget"))
class VRTRACKERCOLLISION_API UVTC_SetupWidget : public UUserWidget
{
  GENERATED_BODY()

  // ─── 피실험자 기본 정보 ─────────────────────────────────────────────────
public:
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_SubjectID;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Height;

  // ─── 실행 모드 (Toggle: Checked=VR, Unchecked=Simulation) ──────────────
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UCheckBox> Toggle_VRMode;

  // ─── Mount Offset ────────────────────────────────────────────────────────
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Offset_Waist_X;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Offset_Waist_Y;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Offset_Waist_Z;

  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Offset_LKnee_X;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Offset_LKnee_Y;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Offset_LKnee_Z;

  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Offset_RKnee_X;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Offset_RKnee_Y;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Offset_RKnee_Z;

  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Offset_LFoot_X;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Offset_LFoot_Y;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Offset_LFoot_Z;

  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Offset_RFoot_X;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Offset_RFoot_Y;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Offset_RFoot_Z;

  // ─── Vehicle Hip Reference Position ─────────────────────────────────────
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_HipRef_X;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_HipRef_Y;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_HipRef_Z;

  // ─── 가시성 ─────────────────────────────────────────────────────────────
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UCheckBox> CB_ShowCollisionSpheres;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UCheckBox> CB_ShowTrackerMesh;

  // ─── 버튼 ────────────────────────────────────────────────────────────────
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_SaveConfig;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_LoadConfig;
  UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Btn_StartSession;

  // ─── Blueprint 호출 가능 함수 ─────────────────────────────────────────────

  // 현재 위젯 입력값 → FVTCSessionConfig 로 변환
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Setup")
  FVTCSessionConfig BuildConfigFromInputs() const;

  // FVTCSessionConfig → 위젯 입력란 채우기 (Load 후 화면 반영)
  UFUNCTION(BlueprintCallable, Category = "VTC|Setup")
  void PopulateFromConfig(const FVTCSessionConfig& Config);

  // SubjectID 비어있지 않고 Height > 0 인지 확인
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Setup")
  bool IsInputValid() const;

protected:
  virtual void NativeConstruct() override;

private:
  // 버튼 클릭 핸들러
  UFUNCTION() void OnSaveConfigClicked();
  UFUNCTION() void OnLoadConfigClicked();
  UFUNCTION() void OnStartSessionClicked();


  // EditableTextBox → float 파싱 (실패 시 Default 반환)
  static float ParseFloat(const UEditableTextBox* TB, float Default = 0.0f);

  // float → EditableTextBox 텍스트 설정
  static void SetFloat(UEditableTextBox* TB, float Value);

  // FVector 3개 TextBox → FVector
  static FVector ParseVector(const UEditableTextBox* TX,
                              const UEditableTextBox* TY,
                              const UEditableTextBox* TZ);

  // FVector → 3개 TextBox
  static void SetVector(UEditableTextBox* TX, UEditableTextBox* TY,
                        UEditableTextBox* TZ, const FVector& V);
};
