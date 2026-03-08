// Copyright GMTCK PQDQ Team. All Rights Reserved.
// VTC_VRGameMode.h — VR 전용 GameMode
//
// [역할]
//   VRTestLevel에서 사용하는 GameMode.
//   OperatorController를 기본 컨트롤러로 사용해
//   세션 관리(프로파일 로드/적용, P키 재적용)만 활성화한다.
//
// [Blueprint]
//   BP_VTC_VRGameMode 생성 후 VR 테스트 레벨의 World Settings > GameMode Override
//   에 설정한다. OperatorController와 TrackerPawn은 Blueprint에서도 override 가능.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VTC_VRGameMode.generated.h"

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="VTC VR Game Mode"))
class VRTRACKERCOLLISION_API AVTC_VRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AVTC_VRGameMode();
};
