// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_ProfileManagerWidget.h — 피실험자 + 차량별 프로파일 관리 Editor Utility Widget
//
// [역할]
//   탭 1 (Vehicle Preset): 차량별 Hip Position을 Saved/VTCPresets/<Name>.json으로 저장/삭제.
//   탭 2 (Profile):        피실험자+차량 조합 설정을 Saved/VTCProfiles/<Name>.json으로 저장/불러오기.
//   [Set as Active] →      Saved/VTCConfig/ActiveProfile.txt에 선택한 프로파일 이름 기록.
//   VRTestLevel BeginPlay가 ActiveProfile.txt를 읽어 자동 적용.
//
// ─────────────────────────────────────────────────────────────────────────────
// [탭 1: Vehicle Preset — 필수 BindWidget]
//   Combo_VehiclePresetList    ComboBoxString  — 등록된 차량 목록 (삭제 대상 선택)
//   TB_VehicleName             EditableTextBox — 새로 등록할 차량 이름
//   TB_HipPos_X                EditableTextBox — 차량 Hip X (cm, 월드 좌표)
//   TB_HipPos_Y                EditableTextBox — 차량 Hip Y (cm)
//   TB_HipPos_Z                EditableTextBox — 차량 Hip Z (cm)
//   Btn_SaveVehiclePreset      Button          — 등록 실행
//   Btn_DeleteVehiclePreset    Button          — 선택한 차량 삭제
//   Txt_VehicleStatus          TextBlock       — 결과 메시지 (BindWidgetOptional)
//
// ─────────────────────────────────────────────────────────────────────────────
// [탭 2: Profile — 필수 BindWidget]
//   프로파일 관리:
//     Combo_ProfileSelect       ComboBoxString  — 저장된 프로파일 목록
//     TB_ProfileName            EditableTextBox — 저장할 프로파일 이름
//     Btn_NewProfile            Button          — 입력란 초기화 (새 프로파일 준비)
//     Btn_LoadProfile           Button          — 선택한 프로파일 불러오기
//     Btn_SaveProfile           Button          — 현재 입력값 저장
//     Btn_DeleteProfile         Button          — 선택한 프로파일 삭제
//   Active 지정:
//     Btn_SetAsActive           Button          — 선택 프로파일을 Active로 지정
//     Txt_ActiveProfile         TextBlock       — 현재 Active 이름 표시
//   피실험자 정보:
//     TB_SubjectID              EditableTextBox
//     TB_Height                 EditableTextBox
//   Mount Offsets (트래커별 X/Y/Z):
//     TB_Offset_Waist_X/Y/Z
//     TB_Offset_LKnee_X/Y/Z
//     TB_Offset_RKnee_X/Y/Z
//     TB_Offset_LFoot_X/Y/Z
//     TB_Offset_RFoot_X/Y/Z
//   차종 선택 (→ TB_HipRef 자동입력):
//     Combo_VehiclePresetSelect ComboBoxString
//   Vehicle Hip Reference (자동입력, 수동 수정 가능):
//     TB_HipRef_X / TB_HipRef_Y / TB_HipRef_Z
//   거리 임계값:
//     Slider_Warning            Slider     (범위 3~50 cm, 기본 10)
//     Slider_Collision          Slider     (범위 1~20 cm, 기본 3)
//     Txt_WarningVal            TextBlock
//     Txt_CollisionVal          TextBlock
//   가시성:
//     CB_ShowCollisionSpheres   CheckBox
//     CB_ShowTrackerMesh        CheckBox
//   상태:
//     Txt_Status                TextBlock  — 저장/불러오기 결과 메시지 (BindWidgetOptional)

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

	// ─── [탭 1] Vehicle Preset 관리 ─────────────────────────────────────────
public:
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UComboBoxString>  Combo_VehiclePresetList;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UEditableTextBox> TB_VehicleName;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UEditableTextBox> TB_HipPos_X;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UEditableTextBox> TB_HipPos_Y;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UEditableTextBox> TB_HipPos_Z;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UButton>          Btn_SaveVehiclePreset;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UButton>          Btn_DeleteVehiclePreset;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock>       Txt_VehicleStatus;

	// ─── [탭 2] 프로파일 관리 ───────────────────────────────────────────────
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UComboBoxString>  Combo_ProfileSelect;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UEditableTextBox> TB_ProfileName;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UButton>          Btn_NewProfile;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UButton>          Btn_LoadProfile;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UButton>          Btn_SaveProfile;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UButton>          Btn_DeleteProfile;

	// ─── Active Profile 지정 ────────────────────────────────────────────────
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UButton>          Btn_SetAsActive;
	UPROPERTY(meta = (BindWidget))         TObjectPtr<UTextBlock>       Txt_ActiveProfile;

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

	// ─── Vehicle Hip Reference (차종 선택 시 자동입력) ───────────────────────
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_HipRef_X;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_HipRef_Y;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UEditableTextBox> TB_HipRef_Z;

	// ─── 임계값 슬라이더 ────────────────────────────────────────────────────
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider>    Slider_Warning;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider>    Slider_Collision;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_WarningVal;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> Txt_CollisionVal;

	// ─── 차종 프리셋 선택 (Profile 탭) ──────────────────────────────────────
	// 선택 시 OnPresetSelectionChanged → TB_HipRef_X/Y/Z 자동입력
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UComboBoxString> Combo_VehiclePresetSelect;

	// ─── 가시성 ─────────────────────────────────────────────────────────────
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCheckBox> CB_ShowCollisionSpheres;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCheckBox> CB_ShowTrackerMesh;

	// ─── 상태 메시지 (Profile 탭) ────────────────────────────────────────────
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
	// ─── [탭 1] Vehicle Preset 버튼 핸들러 ──────────────────────────────────
	UFUNCTION() void OnSaveVehiclePresetClicked();
	UFUNCTION() void OnDeleteVehiclePresetClicked();
	void RefreshVehiclePresetListComboBox();   // 탭 1의 관리용 목록 갱신
	void ShowVehicleStatus(const FString& Message, bool bSuccess = true);

	// ─── [탭 2] 프로파일 버튼 핸들러 ────────────────────────────────────────
	UFUNCTION() void OnNewProfileClicked();
	UFUNCTION() void OnLoadProfileClicked();
	UFUNCTION() void OnSaveProfileClicked();
	UFUNCTION() void OnDeleteProfileClicked();

	// Active Profile 지정
	UFUNCTION() void OnSetAsActiveClicked();

	// 슬라이더 콜백
	UFUNCTION() void OnWarningSliderChanged(float Value);
	UFUNCTION() void OnCollisionSliderChanged(float Value);

	// 차종 프리셋 콤보 변경 (Profile 탭 Combo_VehiclePresetSelect)
	UFUNCTION() void OnPresetSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	void RefreshVehiclePresetComboBox();   // Profile 탭의 선택용 콤보 갱신

	// 상태 메시지 표시 (Profile 탭 Txt_Status)
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
