// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_DataLogger.h — CSV 데이터 로깅 및 내보내기
//
// [출력 파일 종류]
//   ExportToCSV()       → *_summary.csv : 세션당 1행, Human Factors 분석용 (기본 출력)
//   ExportFrameDataCSV()→ *_frames.csv  : 10Hz 원시 프레임 데이터 (연구자 선택 사용)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tracker/VTC_TrackerTypes.h"
#include "Tracker/VTC_TrackerInterface.h"
#include "VTC_DataLogger.generated.h"

// 프레임 버퍼용 내부 구조체 (ExportFrameDataCSV에서 사용)
USTRUCT(BlueprintType)
struct FVTCLogRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Timestamp;
	UPROPERTY(BlueprintReadOnly) FString SubjectID;
	UPROPERTY(BlueprintReadOnly) float   Height;
	UPROPERTY(BlueprintReadOnly) float   UpperLeftLeg;
	UPROPERTY(BlueprintReadOnly) float   UpperRightLeg;
	UPROPERTY(BlueprintReadOnly) float   LowerLeftLeg;
	UPROPERTY(BlueprintReadOnly) float   LowerRightLeg;

	// 각 신체 부위 위치 (cm, UE 좌표계)
	UPROPERTY(BlueprintReadOnly) FVector HipLocation;
	UPROPERTY(BlueprintReadOnly) FVector LeftKneeLocation;
	UPROPERTY(BlueprintReadOnly) FVector RightKneeLocation;
	UPROPERTY(BlueprintReadOnly) FVector LeftFootLocation;
	UPROPERTY(BlueprintReadOnly) FVector RightFootLocation;

	// 거리 정보 (기준점별 전체)
	UPROPERTY(BlueprintReadOnly) TArray<FVTCDistanceResult> DistanceResults;

	// 충돌 발생 여부
	UPROPERTY(BlueprintReadOnly) bool    bCollisionOccurred;
	UPROPERTY(BlueprintReadOnly) FString CollisionPartName;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVTCLogExported, const FString&, FilePath);

UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent, DisplayName="VTC Data Logger"))
class VRTRACKERCOLLISION_API UVTC_DataLogger : public UActorComponent
{
	GENERATED_BODY()

public:
	UVTC_DataLogger();

	// ─── 설정 ────────────────────────────────────────────────────────────────

	// 로그 저장 폴더 (기본: 프로젝트 Saved/VTCLogs/)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Logger")
	FString LogDirectory = TEXT("");

	// Tracker 위치 공급자 (SessionManager가 BeginPlay에서 연결)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Logger")
	TScriptInterface<IVTC_TrackerInterface> TrackerSource;

	// 로그 샘플링 레이트 (Hz)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VTC|Logger",
		meta=(ClampMin=1.0f, ClampMax=30.0f))
	float LogHz = 10.0f;

	// ─── 상태 ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VTC|Logger")
	bool bIsLogging = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VTC|Logger")
	FString CurrentSubjectID;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "VTC|Logger")
	int32 LoggedRowCount = 0;

	// ─── 함수 ────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "VTC|Logger")
	void StartLogging(const FString& SubjectID);

	UFUNCTION(BlueprintCallable, Category = "VTC|Logger")
	void StopLogging();

	// [메인] Human Factors 요약 CSV (*_summary.csv, 세션당 1행) → 파일 경로 반환
	UFUNCTION(BlueprintCallable, Category = "VTC|Logger")
	FString ExportToCSV();

	// [선택] 10Hz 원시 프레임 CSV (*_frames.csv) → 파일 경로 반환
	UFUNCTION(BlueprintCallable, Category = "VTC|Logger")
	FString ExportFrameDataCSV();

	// 충돌 이벤트 즉시 기록
	UFUNCTION(BlueprintCallable, Category = "VTC|Logger")
	void LogCollisionEvent(const FVTCCollisionEvent& Event);

	// 거리 결과 + 신체 데이터를 한 행으로 기록 (매 LogHz마다 SessionManager가 호출)
	UFUNCTION(BlueprintCallable, Category = "VTC|Logger")
	void LogFrame(const FVTCBodyMeasurements& Measurements,
		const TArray<FVTCDistanceResult>& DistanceResults);

	// 로그 버퍼 초기화
	UFUNCTION(BlueprintCallable, Category = "VTC|Logger")
	void ClearLog();

	UPROPERTY(BlueprintAssignable, Category = "VTC|Logger|Events")
	FOnVTCLogExported OnLogExported;

private:
	TArray<FVTCLogRow>       LogBuffer;
	TArray<FVTCCollisionEvent> CollisionEvents;

	float   LogTimer        = 0.0f;
	FString SessionStartTime;

	// ─── 세션 요약 추적 필드 (StartLogging에서 초기화, LogFrame에서 갱신) ──────

	// 캘리브레이션 측정값 캐시 (세션 내 변하지 않음)
	FVTCBodyMeasurements CachedMeasurements;

	// Hip 위치 누적 (세션 평균 계산용)
	FVector HipPosSum         = FVector::ZeroVector;
	int32   HipPosSampleCount = 0;

	// 신체 부위별 최소 클리어런스 (cm) — 미측정 시 TNumericLimits<float>::Max()
	float MinClearance_Hip   = 0.0f;
	float MinClearance_LKnee = 0.0f;
	float MinClearance_RKnee = 0.0f;

	// 세션 전체 최소 클리어런스 및 발생 정보
	float   MinClearance_Overall    = 0.0f;
	FString MinClearance_BodyPart;   // 어느 신체 부위
	FString MinClearance_RefPoint;   // 어느 기준점
	FVector HipPosAtMinClearance    = FVector::ZeroVector;  // 최악 순간의 Hip 위치
	FString MinClearance_Timestamp;  // 최악 클리어런스 발생 시점

	// 전체 최악 경고 단계
	EVTCWarningLevel OverallWorstStatus = EVTCWarningLevel::Safe;

	// 경고/충돌이 발생한 프레임 수
	int32 WarningFrameCount = 0;

	// ─── 세션 타이밍 (Human Factors 분석용) ────────────────────────────────
	FString TestingStartTime;   // 테스트 시작 시각 (밀리초)
	FString TestingEndTime;     // 테스트 종료 시각 (밀리초)

	// 경고/충돌 누적 지속시간 (초) — 프레임 간격 기반 추정
	float WarningDuration_sec   = 0.0f;   // Warning 이상 (Warning + Collision)
	float CollisionDuration_sec = 0.0f;   // Collision만

	// ─── 내부 빌더 함수 ──────────────────────────────────────────────────────

	// Summary CSV
	FString BuildSummaryHeader() const;
	FString BuildSummaryRow() const;

	// Frame CSV (ExportFrameDataCSV 전용)
	FString BuildFrameHeader() const;
	FString FrameRowToCSVLine(const FVTCLogRow& Row) const;

	// 공통 유틸
	FString SaveFile(const FString& Suffix, const FString& Content) const;
	static FString WarningLevelToStatus(EVTCWarningLevel Level);
	static FString GetTimestampString();
};
