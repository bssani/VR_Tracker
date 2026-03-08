// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_ProfileManagerWidget.h — 피실험자 + 차량별 프로파일 관리 Utility Editor
//
// [역할]
//   Saved/VTCProfiles/ 에 SessionConfig 프로파일을 저장/불러오기/삭제.
//   Level 1 없이 VRTestLevel 직접 실행 시 이 위젯에서 사전 설정 후 저장.
//
// [BP 연결 필수 위젯 이름]
//   프로파일 관리:
//     Combo_ProfileSelect    ComboBoxString  — 저장된 프로파일 목록
//     TB_ProfileName         EditableTextBox — 저장할 프로파일 이름
//     Btn_NewProfile         Button          — 입력란 초기화 (새 프로파일 준비)
//     Btn_LoadProfile        Button          — 선택한 프로파일 불러오기
//     Btn_SaveProfile        Button          — 현재 입력값 저장
//     Btn_DeleteProfile      Button          — 선택한 프로파일 삭제
//   피실험자 정보:
//     TB_SubjectID           EditableTextBox
//     TB_Height              EditableTextBox
//   모드:
//   Mount Offsets (트래커별 X/Y/Z):
//     TB_Offset_Waist_X/Y/Z
//     TB_Offset_LKnee_X/Y/Z
//     TB_Offset_RKnee_X/Y/Z
//     TB_Offset_LFoot_X/Y/Z
//     TB_Offset_RFoot_X/Y/Z
//   Vehicle Hip Reference:
//     TB_HipRef_X/Y/Z
//   거리 임계값:
//     Slider_Warning         Slider     (범위 3~50 cm, 기본 10)
//     Slider_Collision       Slider     (범위 1~20 cm, 기본 3)
//     Txt_WarningVal         TextBlock
//     Txt_CollisionVal       TextBlock
//   차종 프리셋:
//     Combo_VehiclePreset    ComboBoxString
//   가시성:
//     CB_ShowCollisionSpheres  CheckBox
//     CB_ShowTrackerMesh       CheckBox
//   상태:
//     Txt_Status             TextBlock  — 저장/불러오기 결과 메시지

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "VTC_SessionConfig.h"
#include "VTC_ProfileManagerWidget.generated.h"

class UEditableTextBox;
class UCheckBox;
class UButton;
class USlider;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "VTC Profile Manager Widget"))
class VRTRACKERCOLLISION_API UVTC_ProfileManagerWidget : public UUserWidget
{
	GENERATED_BODY()

	// ─── 프로파일 관리 ──────────────────────────────────────────────────────
public:
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UComboBoxString>  Combo_ProfileSelect;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UEditableTextBox> TB_ProfileName;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UButton>          Btn_NewProfile;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UButton>          Btn_LoadProfile;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UButton>          Btn_SaveProfile;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UButton>          Btn_DeleteProfile;

	// ─── 피실험자 정보 ──────────────────────────────────────────────────────
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_SubjectID;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_Height;

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

	// ─── Vehicle Hip Reference ───────────────────────────────────────────────
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_HipRef_X;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_HipRef_Y;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_HipRef_Z;

	// ─── 임계값 슬라이더 ────────────────────────────────────────────────────
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider>    Slider_Warning;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider>    Slider_Collision;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_WarningVal;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_CollisionVal;

	// ─── 차종 프리셋 ────────────────────────────────────────────────────────
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UComboBoxString> Combo_VehiclePreset;

	// ─── 가시성 ─────────────────────────────────────────────────────────────
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCheckBox> CB_ShowCollisionSpheres;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCheckBox> CB_ShowTrackerMesh;

	// ─── 상태 메시지 ────────────────────────────────────────────────────────
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Status;

	// ─── Blueprint 호출 가능 함수 ──────────────────────────────────────────

	// 현재 위젯 입력값 → FVTCSessionConfig
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Profile")
	FVTCSessionConfig BuildConfigFromInputs() const;

	// FVTCSessionConfig → 위젯 입력란 채우기
	UFUNCTION(BlueprintCallable, Category = "VTC|Profile")
	void PopulateFromConfig(const FVTCSessionConfig& Config);

	// 프로파일 목록 콤보박스 갱신 (외부에서도 호출 가능)
	UFUNCTION(BlueprintCallable, Category = "VTC|Profile")
	void RefreshProfileComboBox();

protected:
	virtual void NativeConstruct() override;

private:
	// 버튼 핸들러
	UFUNCTION() void OnNewProfileClicked();
	UFUNCTION() void OnLoadProfileClicked();
	UFUNCTION() void OnSaveProfileClicked();
	UFUNCTION() void OnDeleteProfileClicked();

	// 슬라이더 콜백
	UFUNCTION() void OnWarningSliderChanged(float Value);
	UFUNCTION() void OnCollisionSliderChanged(float Value);

	// 차종 프리셋 콤보 변경
	UFUNCTION() void OnPresetSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	void RefreshVehiclePresetComboBox();

	// 상태 메시지 표시 (성공=흰색, 실패=빨강)
	void ShowStatus(const FString& Message, bool bSuccess = true);

	// 입력 파싱 헬퍼
	static float   ParseFloat(const UEditableTextBox* TB, float Default = 0.0f);
	static void    SetFloat(UEditableTextBox* TB, float Value);
	static FVector ParseVector(const UEditableTextBox* TX,
	                            const UEditableTextBox* TY,
	                            const UEditableTextBox* TZ);
	static void    SetVector(UEditableTextBox* TX, UEditableTextBox* TY,
	                          UEditableTextBox* TZ, const FVector& V);
};
