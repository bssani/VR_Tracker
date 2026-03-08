// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_ProfileLibrary.h — 피실험자 + 차량 조합 프로파일 저장/불러오기
//
// [개념]
//   FVTCSessionConfig 전체를 "프로파일"로 저장한다.
//   파일 위치: Saved/VTCProfiles/<ProfileName>.json
//
//   한 사람이 여러 차량을 타는 경우:
//     "Subject01_VehicleA.json", "Subject01_VehicleB.json" 처럼 구분.
//
// [사용법]
//   - VTC_ProfileManagerWidget (Utility Editor)에서 프로파일 생성/편집/저장
//   - VRTestLevel의 OperatorMonitorWidget에서 드롭다운으로 선택 → 적용

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VTC_SessionConfig.h"
#include "VTC_ProfileLibrary.generated.h"

UCLASS(BlueprintType)
class VRTRACKERCOLLISION_API UVTC_ProfileLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 프로파일 저장 디렉토리 반환 (Saved/VTCProfiles/)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Profile")
	static FString GetProfileDirectory();

	// SessionConfig를 ProfileName 이름의 JSON 파일로 저장.
	// Config.ProfileName이 비어있으면 ProfileName 파라미터를 사용.
	UFUNCTION(BlueprintCallable, Category = "VTC|Profile")
	static bool SaveProfile(const FString& ProfileName, const FVTCSessionConfig& Config);

	// JSON 파일에서 SessionConfig 불러오기
	UFUNCTION(BlueprintCallable, Category = "VTC|Profile")
	static bool LoadProfile(const FString& ProfileName, FVTCSessionConfig& OutConfig);

	// 저장된 모든 프로파일 이름 목록 반환
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "VTC|Profile")
	static TArray<FString> GetAvailableProfileNames();

	// 프로파일 삭제
	UFUNCTION(BlueprintCallable, Category = "VTC|Profile")
	static bool DeleteProfile(const FString& ProfileName);
};
