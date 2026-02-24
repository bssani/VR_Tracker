// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_SessionManager.h — 전체 시스템 조율, 세션 상태 머신

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "Tracker/VTC_TrackerInterface.h"
#include "VTC_SessionManager.generated.h"

class AVTC_BodyActor;
class UVTC_CollisionDetector;
class UVTC_WarningFeedback;
class UVTC_DataLogger;

// 세션 상태 변경 시 브로드캐스트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVKCSessionStateChanged,
	EVTCSessionState, OldState, EVTCSessionState, NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVKCSessionExported, const FString&, FilePath);

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="VKC Session Manager"))
class VRTRACKERCOLLISION_API AVTC_SessionManager : public AActor
{
	GENERATED_BODY()

public:
	AVTC_SessionManager();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// ─── 시스템 컴포넌트 참조 ─────────────────────────────────────────────────

	// Tracker 공급자 — TrackerPawn 또는 TrackerManager 모두 할당 가능
	// 비워두면 BeginPlay에서 자동 탐색 (TrackerPawn 우선, 없으면 TrackerManager)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Session|Systems")
	TScriptInterface<IVTC_TrackerInterface> TrackerManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Session|Systems")
	TObjectPtr<AVTC_BodyActor> BodyActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Session|Systems")
	TObjectPtr<UVTC_CollisionDetector> CollisionDetector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Session|Systems")
	TObjectPtr<UVTC_WarningFeedback> WarningFeedback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VKC|Session|Systems")
	TObjectPtr<UVTC_DataLogger> DataLogger;

	// ─── 상태 ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VKC|Session")
	EVTCSessionState CurrentState = EVTCSessionState::Idle;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VKC|Session")
	FString CurrentSubjectID;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VKC|Session")
	float SessionElapsedTime = 0.0f;

	// ─── 세션 제어 함수 ──────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "VKC|Session")
	void StartSession(const FString& SubjectID);

	UFUNCTION(BlueprintCallable, Category = "VKC|Session")
	void StartTestingDirectly();

	UFUNCTION(BlueprintCallable, Category = "VKC|Session")
	void StopSession();

	UFUNCTION(BlueprintCallable, Category = "VKC|Session")
	void RequestReCalibration();

	UFUNCTION(BlueprintCallable, Category = "VKC|Session")
	FString ExportAndEnd();

	// ─── 상태 조회 ───────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Session")
	bool IsTesting() const { return CurrentState == EVTCSessionState::Testing; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Session")
	bool IsCalibrating() const { return CurrentState == EVTCSessionState::Calibrating; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Session")
	FVTCBodyMeasurements GetCurrentBodyMeasurements() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VKC|Session")
	float GetSessionMinDistance() const;

	// ─── Delegates ──────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "VKC|Session|Events")
	FOnVKCSessionStateChanged OnSessionStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "VKC|Session|Events")
	FOnVKCSessionExported OnSessionExported;

private:
	void TransitionToState(EVTCSessionState NewState);
	void AutoFindSystems();

	float LogTimer = 0.0f;

	UFUNCTION()
	void OnCalibrationComplete(const FVTCBodyMeasurements& Measurements);

	UFUNCTION()
	void OnCalibrationFailed(const FString& Reason);

	UFUNCTION()
	void OnWarningLevelChanged(EVTCTrackerRole BodyPart, FString PartName, EVTCWarningLevel Level);
};
