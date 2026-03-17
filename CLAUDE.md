# VR Tracker Collision (VTC) — Claude Code 프로젝트 규칙

## 프로젝트 개요

- **플러그인명**: VRTrackerCollision (클래스 접두사 `VTC_`)
- **엔진**: Unreal Engine 5.5
- **구조**: **단일 VR 레벨** (VR 전용, HMD 필수)
- **설정**: 에디터 Details 패널 + INI 파일 (`Config/VTCSettings.ini`)
- **VR 장비**: HTC Vive Pro 2 + Vive Tracker 3.0 × 5 (Waist, L/R Knee, L/R Foot)

## 아키텍처 핵심

- Setup 레벨 없음, 시뮬레이션 모드 없음
- GameInstance::Init()에서 INI 자동 로드 → OperatorController가 각 Actor에 적용
- 키 바인딩: 1=캘리브레이션, 2=테스트시작, 3=CSV내보내기, 4=Hip Snap
- VehicleHipPosition: bMonitorOnly=true 마커 (Waist 거리 측정만, Warning/Collision 없음)
- PostProcessVolume: WarningFeedback BeginPlay에서 자동 탐색

## 코딩 규칙

- 클래스 접두사: `VTC_` (예: VTC_TrackerPawn, VTC_BodyActor)
- 주석: **한국어** 사용
- UPROPERTY 카테고리: `"VTC|카테고리명"`
- 로그: `UE_LOG(LogTemp, Log/Warning/Error, TEXT("[VTC] ..."))`
- Delegate: `DECLARE_DYNAMIC_MULTICAST_DELEGATE_*` 패턴

## 문서 관리 규칙

### 관리 대상 문서

| 파일 | 용도 |
|------|------|
| `handoff.md` | 개발자 인수인계: 아키텍처, 데이터 흐름, 클래스 목록 |
| `blueprintsetup.md` | BP/에셋 셋업 가이드: 설정 방법, 머티리얼, FAQ |

### 코드 변경 시 문서 업데이트 체크리스트

**새 클래스/파일 추가 시:**
- [ ] handoff.md → 소스 파일 목록에 추가
- [ ] handoff.md → 해당 시스템 섹션에 클래스 설명 추가
- [ ] blueprintsetup.md → 필요한 BP 에셋 목록에 추가

**클래스/파일 삭제 시:**
- [ ] handoff.md → 소스 파일 목록에서 제거
- [ ] handoff.md → 관련 설명 제거
- [ ] blueprintsetup.md → 관련 BP/설정 제거

**키 바인딩/UI 변경 시:**
- [ ] handoff.md → 키 바인딩 섹션 갱신
- [ ] blueprintsetup.md → 조작법/키 안내 갱신

**프로퍼티/설정 추가·변경 시:**
- [ ] blueprintsetup.md → 설정 테이블 갱신
- [ ] blueprintsetup.md → INI 키 목록 갱신 (해당 시)

**Delegate/이벤트 흐름 변경 시:**
- [ ] handoff.md → 데이터 흐름 다이어그램 갱신

**ReferencePoint/CollisionDetector 로직 변경 시:**
- [ ] handoff.md → 거리 계산/경고 단계 설명 갱신
- [ ] blueprintsetup.md → FAQ 갱신

### handoff.md 필수 섹션

1. Project Overview (장비, 엔진, 단일 레벨 구조)
2. System Architecture (다이어그램)
3. Core Systems (TrackerPawn, BodyActor, CollisionDetector, WarningFeedback, SessionManager, DataLogger, OperatorController)
4. Data Flow (INI → GameInstance → OperatorController → Actors)
5. Key Bindings
6. C++ Source File List (현재 존재하는 파일만)

### blueprintsetup.md 필수 섹션

1. 필수 BP 에셋 목록 + 생성/설정 방법
2. PostProcessVolume 배치
3. 머티리얼 가이드
4. INI 설정 파일 설명
5. VR 테스트 절차 (SteamVR → VR Preview → Hip Snap → 테스트)
6. FAQ

## 절대 하지 말 것

- 존재하지 않는 파일/클래스를 문서에 기록하지 말 것
- "Level 1", "Level 2", "Setup Level", "시뮬레이션 모드" 등 삭제된 개념을 사용하지 말 것
- 코드를 수정한 뒤 문서 업데이트를 빠뜨리지 말 것
