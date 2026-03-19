// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_SubjectInfoWidget.h — 피실험자 정보 입력 위젯
//
// [사용법]
//   1. 이 클래스를 부모로 Widget Blueprint 생성 (WBP_VTC_SubjectInfo 권장)
//   2. 위젯 안에 아래 이름과 정확히 일치하는 위젯 추가:
//        - TB_SubjectID   (EditableTextBox) — 피실험자 ID
//        - TB_Height      (EditableTextBox) — 키(cm)
//        - Btn_StartSession (Button)        — 시작 버튼
//   3. Add to Viewport 후 OnSessionStartRequested 델리게이트에 바인딩
//      → SubjectID, Height_cm 를 받아 SessionManager->StartSessionWithHeight() 호출

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VTC_SubjectInfoWidget.generated.h"

class UEditableTextBox;
class UButton;

// 시작 버튼 클릭 시 브로드캐스트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVTCSessionStartRequested,
	const FString&, SubjectID,
	float, Height_cm);

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="VTC Subject Info Widget"))
class VRTRACKERCOLLISION_API UVTC_SubjectInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ─── BindWidget — BP에서 동일한 이름의 위젯과 자동 연결 ────────────────
	// 이름이 다르면 컴파일 에러: 반드시 BP 위젯 이름을 아래와 동일하게 지정

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UEditableTextBox> TB_SubjectID;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UEditableTextBox> TB_Height;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> Btn_StartSession;

	// ─── 델리게이트 ──────────────────────────────────────────────────────────

	// 시작 버튼 클릭 시 발생 — BP 또는 Level Blueprint에서 SessionManager와 연결
	UPROPERTY(BlueprintAssignable, Category = "VTC|Widget|Events")
	FOnVTCSessionStartRequested OnSessionStartRequested;

	// ─── Blueprint 호출 가능 함수 ─────────────────────────────────────────

	// 높이 입력값 반환 (파싱 실패 시 0)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Widget")
	float GetEnteredHeight() const;

	// SubjectID 입력값 반환
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Widget")
	FString GetEnteredSubjectID() const;

	// 입력 유효성 검사: SubjectID 비어있지 않고 Height > 0
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Widget")
	bool IsInputValid() const;

	// 입력 초기화
	UFUNCTION(BlueprintCallable, Category = "VTC|Widget")
	void ClearInputs();

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void OnStartSessionClicked();
};
