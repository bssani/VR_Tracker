# VR Tracker Collision (VTC) — Blueprint / 에셋 셋업 가이드

## 1. 필수 BP 에셋 목록

아래 에셋을 Content Browser에서 생성하고 설정한다.

### BP_VTC_TrackerPawn
- 부모 클래스: `VTC_TrackerPawn`
- MotionSource 이름을 SteamVR Tracker Role과 맞춘다
  - 기본값: Waist, LeftKnee, RightKnee, LeftAnkle, RightAnkle
  - SteamVR → Settings → Controllers → Manage Trackers에서 동일한 Role 할당 필요
  - 주의: SteamVR의 하드웨어 역할명은 여전히 "LeftFoot"/"RightFoot"이므로 MotionSource 문자열 값은 그대로 유지됨

### BP_VTC_GameMode
- 부모 클래스: `VTC_GameMode`
- Default Pawn Class = `BP_VTC_TrackerPawn`
- Player Controller Class = `VTC_OperatorController` (또는 BP 서브클래스)

### BP_VTC_GameInstance
- 부모 클래스: `VTC_GameInstance`
- Project Settings → Maps & Modes → Game Instance Class에 할당
- Init()에서 INI 자동 로드됨

### BP_VTC_SessionManager
- 부모 클래스: `VTC_SessionManager`
- 레벨에 배치 (OperatorController가 자동 탐색)
- CollisionDetector, DataLogger, WarningFeedback 컴포넌트 자동 생성

### BP_VTC_BodyActor
- 부모 클래스: `VTC_BodyActor`
- 레벨에 배치 (SessionManager에서 자동 탐색)

### BP_VTC_StatusActor
- 부모 클래스: `VTC_StatusActor`
- 3D 위젯 표시용 — 레벨에 배치

### WBP_VTC_StatusWidget
- 부모 클래스: `VTC_StatusWidget`
- Widget Blueprint로 생성
- StatusActor의 Widget Component에 할당

### WBP_VTC_OperatorMonitor (선택)
- 부모 클래스: `VTC_OperatorMonitorWidget`
- 운영자 데스크탑에 거리 데이터 + 상태 표시
- OperatorController의 OperatorMonitorWidgetClass에 할당

### VTC_ReferencePoint
- 레벨에 직접 배치하여 차량 내부 구조물 기준점 설정
- PartName, RelevantBodyParts, SafeDistance 설정
- VehicleHipPosition은 OperatorController가 자동 스폰 (수동 배치 불필요)

## 2. PostProcessVolume 배치

WarningFeedback은 BeginPlay에서 레벨의 PostProcessVolume을 자동 탐색한다.

1. 레벨에 **PostProcessVolume** 추가
2. **Infinite Extent (Unbound)** 체크
3. Post Process Material 슬롯에 경고용 머티리얼 인스턴스 할당 (아래 참조)

> 자동 탐색이므로 WarningFeedback에 수동 할당은 불필요.

## 3. 머티리얼 가이드

### 경고 시각 효과 머티리얼
- **M_VTC_WarningPostProcess**: Post Process Material
- Scalar Parameter: `WarningIntensity` (0.0 ~ 1.0)
- Vector Parameter: `WarningColor`
  - Warning = 노란색 (1.0, 1.0, 0.0)
  - Collision = 빨간색 (1.0, 0.0, 0.0)

### 충돌 구체 머티리얼
- BodySegmentComponent에서 사용하는 반투명 구체 머티리얼
- `bShowCollisionSpheres` INI 설정으로 표시/숨김 제어

## 4. INI 설정 파일

경로: `{ProjectDir}/Config/VTCSettings.ini`

GameInstance::Init()에서 자동 로드. 에디터에서 `SaveConfigToINI()` 호출 시 저장.

### INI 키 목록

| 섹션 | 키 | 타입 | 설명 |
|------|-----|------|------|
| VTC/Settings | MountOffset_Waist_X/Y/Z | float | Waist 트래커 장착 오프셋 (cm) |
| VTC/Settings | MountOffset_LeftKnee_X/Y/Z | float | Left Knee 장착 오프셋 |
| VTC/Settings | MountOffset_RightKnee_X/Y/Z | float | Right Knee 장착 오프셋 |
| VTC/Settings | MountOffset_LeftAnkle_X/Y/Z | float | Left Ankle 장착 오프셋 |
| VTC/Settings | MountOffset_RightAnkle_X/Y/Z | float | Right Ankle 장착 오프셋 |
| VTC/Settings | VehicleHipPosition_X/Y/Z | float | 차량 기준 Hip 위치 (cm) |
| VTC/Settings | ShowCollisionSpheres | bool | 충돌 구체 표시 여부 |
| VTC/Settings | ShowTrackerMesh | bool | Vive Tracker 하드웨어 메시 표시 여부 |

### 에디터에서만 설정하는 값 (INI 미포함)

| 프로퍼티 | 클래스 | 설명 |
|----------|--------|------|
| SubjectID | SessionConfig | 피실험자 ID (매 세션 입력) |
| Height_cm | SessionConfig | 피실험자 키 (매 세션 입력) |
| WarningThreshold_cm | SessionConfig | 경고 거리 임계값 |
| CollisionThreshold_cm | SessionConfig | 충돌 거리 임계값 |
| bUseVehiclePreset | SessionConfig | 차종 프리셋 사용 여부 |

## 5. VR 테스트 절차

### 사전 준비
1. SteamVR 실행 → Room Setup 완료 (바닥 기준점 설정)
2. SteamVR → Settings → Controllers → Manage Trackers
   - 5개 Tracker에 Role 할당: Waist, LeftKnee, RightKnee, LeftFoot, RightFoot (SteamVR 하드웨어 역할명 기준 — 코드 내부는 LeftAnkle/RightAnkle로 표시)
3. 에디터 → Project Settings → Maps & Modes
   - Game Instance = `BP_VTC_GameInstance`
   - Default GameMode = `BP_VTC_GameMode`

### 테스트 실행
1. **VR Preview** 모드로 실행 (에디터 Play → VR Preview)
2. 키 **4** → **Hip Snap**: Waist(+MountOffset)를 VehicleHipPosition으로 정렬
3. 키 **1** → 캘리브레이션 시작 (T-Pose 유지)
4. 키 **2** → 테스트 시작 (또는 캘리브레이션 완료 후 자동 전환)
5. 테스트 진행 중 OperatorMonitorWidget에서 실시간 거리 확인
6. 키 **3** → 세션 종료 + CSV 내보내기

### 데이터 확인
- CSV 파일: `{ProjectDir}/Saved/VTC_Logs/` 디렉토리
- 파일명: `VTC_{SubjectID}_{Timestamp}.csv`

## 6. FAQ

### Q: PostProcess 경고가 작동하지 않아요
- 레벨에 PostProcessVolume이 배치되어 있는지 확인
- PostProcessVolume의 **Infinite Extent (Unbound)** 체크 확인
- 출력 로그에서 `[VTC] WarningFeedback: PostProcessVolume 자동 탐색 완료` 확인

### Q: 트래커 위치가 실제와 다릅니다
- SteamVR Manage Trackers에서 Role이 올바르게 할당되었는지 확인
- MountOffset 값을 INI에서 조정 (트래커 하드웨어 위치 → 실제 신체 접촉점 보정)

### Q: Hip Snap 후에도 위치가 맞지 않습니다
- VehicleHipPosition이 올바른 월드 좌표로 설정되었는지 확인
- MountOffset_Waist 값이 정확한지 확인 (해부학적 오프셋 보정)

### Q: 거리 측정이 모니터에 표시되지 않습니다
- OperatorMonitorWidgetClass가 OperatorController에 할당되었는지 확인
- ReferencePoint의 RelevantBodyParts가 비어있지 않은지 확인

### Q: VehicleHipPosition 거리가 경고를 발생시킵니다
- VehicleHipPosition ReferencePoint는 `bMonitorOnly=true`로 설정됨 (자동)
- 거리 표시만 하고 Warning/Collision은 트리거하지 않음
