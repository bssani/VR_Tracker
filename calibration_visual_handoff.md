# Calibration Visual Explainer — 세션 핸드오프

> 작성일: 2026-03-23
> 목적: Leadership 발표용 캘리브레이션 설명 다이어그램 6장 제작
> 상태: **계획 완료, 제작 대기**

---

## 1. 배경 및 목적

VR Tracker Collision (VTC) 플러그인의 **Leadership 발표**를 준비 중이다.
발표에서 캘리브레이션이 왜 필요하고 어떻게 작동하는지를 시각적으로 설명하는 다이어그램 6장을 제작해야 한다.

기존에 생성된 발표 자료:
- `Docs/generate_ppt.py` — 16슬라이드 PPT (Python-pptx)
- `Docs/calibration_explainer.md` — 캘리브레이션 기술 설명 문서

이번 작업은 **calibration_explainer.md의 핵심 내용을 시각화**하는 것이다.

---

## 2. 핵심 문제 (대화에서 도출)

### 2.1 사용자가 올린 그림 설명

사용자가 업로드한 이미지: 사람이 의자에 앉아있는 side view 다이어그램.
- **빨간 점** = Waist Tracker (허리/배 앞쪽에 착용)
- **파란 점** = Vehicle Hip Position (차량 설계 기준 H-point, 시트 위)

### 2.2 문제 상황

Waist Tracker를 Hip Position에 단순 snap 하면:
- 빨간 점(Tracker)이 파란 점(Hip Position)으로 강제 이동
- Tracker는 실제 Hip Joint보다 **앞쪽(복부)**에 있으므로
- 이 오프셋만큼 전체 body가 뒤로/아래로 밀려서 **시트 안으로 파묻히는 현상** 발생

### 2.3 해결 방법 (코드에서 확인)

`VTC_OperatorController.cpp`의 `SnapToVehicleHip()` 함수:

```
// 핵심 로직
1. BodyActor->GetBodyPartLocation(Waist) 로 MountOffset 보정된 유효 위치 획득
2. Delta = VehicleHipPosition - EffectiveWaistPosition (보정된 위치 기준)
3. Pawn 전체를 Delta만큼 이동
```

수식: `EffectiveLocation = TrackerWorldLocation + Quat(TrackerWorldRotation) × MountOffset`

→ Raw Tracker 위치가 아닌, MountOffset 보정된 위치 기준으로 Delta를 계산하므로
→ Tracker↔Hip 사이 물리적 오프셋이 유지되고 body가 파묻히지 않음

---

## 3. 키 바인딩 (최신)

| 키 | 기능 |
|----|------|
| **1** | 캘리브레이션 시작 (T-Pose) |
| **2** | 테스트 시작 |
| **3** | CSV 저장 + 세션 종료 |
| **P** | Hip Snap (← 기존 4에서 변경됨) |

> ⚠️ 코드에는 아직 키 4로 되어있을 수 있음. 발표 자료에서는 P로 표기해야 함.

---

## 4. 다이어그램 6장 계획

### 디자인 스펙

- **형식**: SVG (show_widget) 또는 React artifact
- **배경**: 다크 네이비 (#1A1A2E) — 기존 PPT 컬러 팔레트와 통일
- **색상 팔레트**:
  - 시안 (#00B4D8) — 강조, Hip Position
  - 빨간색 (#FF6B6B) — 문제/Tracker
  - 초록색 (#57CC99) — 해결/Effective Position
  - 노란색 (#FFD166) — 경고
  - 흰색/연한 파란 — 텍스트
- **스타일**: 기존 PPT와 동일한 톤 (Leadership Briefing 수준)

---

### Image 1 — "Source: NX CAD Data"

**내용**: NX CAD에서 데이터를 가져오는 첫 단계

- Side view: 차량 시트 단면 윤곽선 (심플한 라인 드로잉)
- 파란 점 = Hip Position (H-point), 라벨 표시
- 시트 메시 영역 표시
- 라벨: "Vehicle Seating Mesh from NX" / "Hip Position (H-point)"

**핵심 메시지**: 차량 설계 기준점은 NX CAD에서 정확히 정의되어 있다

---

### Image 2 — "The Goal: Position Matching"

**내용**: NX의 Hip Position과 실제 사람의 Hip을 일치시켜야 하는 이유

- 왼쪽 패널: NX CAD 시트 + 파란 점 (Hip Position)
- 오른쪽 패널: 실제 사람이 앉아있는 모습 + 빨간 점 (Waist Tracker)
- 가운데: "Match" 또는 "=" 화살표
- 두 점을 일치시켜야 "동일 기준에서 비교 가능"하다는 설명

**핵심 메시지**: 가상 인체를 차량 설계 기준 Hip에 정렬해야 피험자 간 비교가 유효하다

---

### Image 3 — "The Physical Constraint"

**내용**: Tracker를 실제 Hip Joint에 부착할 수 없는 물리적 한계

- **Top-down view** (위에서 본 골반 단면)
  - 골반 뼈 윤곽선 (simplified)
  - 실제 Hip Joint 위치 = 골반 중심/안쪽 (파란 점)
  - Tracker 부착 위치 = 배 앞쪽, 벨트 라인 (빨간 점)
  - 두 점 사이 오프셋 화살표 + "MountOffset" 라벨
- **Side view** (보조)
  - 앉은 자세에서 Tracker가 복부 앞으로 돌출된 모습

**핵심 메시지**: Vive Tracker는 스트랩으로 허리에 착용하므로 Hip Joint보다 항상 앞쪽(복부 방향)에 위치한다. 직접 부착 불가능.

---

### Image 4 — "The Problem: Naive Snap"

**내용**: 단순 snap 시 body가 시트에 파묻히는 문제

- Side view: 사용자가 올린 그림과 유사한 구도
- 빨간 점(Waist Tracker) → 파란 점(Hip Position)으로 화살표 ("Snap!")
- 결과: body 실루엣이 시트 메시 안으로 들어간 모습
  - 반투명 빨간색으로 겹침 영역 표시
- 빨간 X 마크 + "Body sinks into seat cushion"
- 설명: "Tracker ≠ Hip Joint → offset 무시하면 body가 뒤로 밀림"

**핵심 메시지**: Tracker 위치를 Hip Position에 직접 맞추면 MountOffset만큼 body가 시트 안으로 파묻힌다

---

### Image 5 — "The Solution: MountOffset Correction"

**내용**: MountOffset 보정 후 올바른 snap

- Side view: 같은 구도
- 빨간 점 = Raw Waist Tracker (원래 위치)
- 초록 점 = Effective Position (Tracker + MountOffset 보정)
  - 빨간→초록 사이 점선 화살표 + "MountOffset" 라벨
- 파란 점 = Vehicle Hip Position (목표)
- Delta 화살표: 초록 점 → 파란 점 (이 Delta만큼 Pawn 전체 이동)
- 결과: body가 시트 위에 정상적으로 앉아있는 모습
- 초록 체크마크 ✓
- 수식 박스: `Effective = TrackerPos + Quat(Rotation) × MountOffset`

**핵심 메시지**: MountOffset 보정으로 Tracker의 물리적 편차를 수학적으로 제거한 후 snap 수행

---

### Image 6 — "Full Calibration Flow"

**내용**: 전체 캘리브레이션 워크플로우 (5단계 가로 플로우)

```
Step 1          Step 2           Step 3          Step 4          Step 5
T-Pose    →   Body         →   Validation  →   Hip Snap    →   Test
(Key: 1)      Measurement       (Auto)         (Key: P)        Ready
                                                               (Key: 2)
```

**각 단계 상세:**

| Step | 제목 | 설명 | 아이콘/비주얼 |
|------|------|------|-------------|
| 1 | T-Pose | 피험자가 팔 벌리고 서있는 자세. 3초 카운트다운 (음성: "3, 2, 1, 완료!"). 키 1로 시작 | 사람 stick figure, T-Pose |
| 2 | Body Measurement | 4개 세그먼트 자동 측정: Hip→LKnee, Hip→RKnee, LKnee→LAnkle, RKnee→RAnkle. HMD 높이로 키 추정 | 세그먼트 라인 4개 표시된 하체 |
| 3 | Validation | 3중 자동 검증: ①최소 10cm ②좌우 비대칭 25% 미만 ③대퇴:경골 비율 0.7~1.5. 실패 시 재시도 요청 | 체크리스트 아이콘 |
| 4 | Hip Snap | 키 P로 MountOffset 보정 snap 실행. Pawn 전체가 Delta만큼 이동하여 Hip Position에 정렬 | 화살표 + 시트 |
| 5 | Test Ready | 키 2로 테스트 시작. 실시간 거리 측정 + Safe/Warning/Collision 피드백 활성화 | 녹색 "GO" |

**하단**: 전체 소요 시간 타임라인 바 ("Total: ~30 seconds")

---

## 5. 참고 코드 위치

다이어그램 내용의 기술적 근거가 되는 코드:

| 개념 | 파일 | 함수/라인 |
|------|------|-----------|
| Hip Snap (MountOffset 보정) | `VTC_OperatorController.cpp` | `SnapToVehicleHip()` |
| MountOffset 적용 수식 | `VTC_BodyActor.cpp` | `SyncSpherePositions()` 내 lambda |
| GetBodyPartLocation (보정된 위치) | `VTC_BodyActor.cpp` | `GetBodyPartLocation()` |
| T-Pose 캘리브레이션 | `VTC_CalibrationComponent.cpp` | `StartCalibration()`, `SnapCalibrate()` |
| 3중 유효성 검증 | `VTC_CalibrationComponent.cpp` | `ValidateMeasurements()` |
| 세그먼트 측정 | `VTC_CalibrationComponent.cpp` | `CalculateMeasurements()` |
| TrackerPawn snap | `VTC_TrackerPawn.cpp` | `SnapWaistTo()` |

---

## 6. 이전 대화 참조

### 캘리브레이션 설계 논의
- **채팅 URL**: https://claude.ai/chat/41c061a1-4026-4f99-911f-6d297edb6fad
- **주요 내용**: 눈 높이 vs H-point 기준, 골반 Tracker 착용 편차, 헤드레스트 방식, 차량별 Config 관리

### VR Tracker 셋업
- **채팅 URL**: https://claude.ai/chat/6f670c6d-ec0d-4f3e-b1d7-c5dcb57919ff
- **주요 내용**: OpenXR Vive Tracker 설정, MotionSource 매핑, Skeletal Mesh pelvis 위치 조정

### PPT 발표 자료
- **채팅 URL**: https://claude.ai/chat/6d07c905-b932-4558-bd0c-fc740dd104eb
- **주요 내용**: 16슬라이드 Leadership Briefing PPT 생성

---

## 7. 다음 작업 (이 세션 이어갈 때)

1. **Image 1~6 SVG 제작** — show_widget 또는 React artifact로 생성
2. **PPT 통합** — 생성된 다이어그램을 기존 PPT에 삽입하거나 별도 슬라이드로 추가
3. **키 바인딩 업데이트** — 코드에서 Hip Snap 키를 4→P로 변경 확인 (아직 안 됐으면 코드 수정)
4. **calibration_explainer.md 갱신** — Image 참조 추가

---

## 8. 제약 사항 / 주의 사항

- PPT 컬러 팔레트 반드시 통일 (다크 네이비 배경)
- Leadership 대상이므로 기술 용어는 최소화, 시각적 직관성 우선
- 수식은 Image 5에만 포함 (나머지는 개념 설명 위주)
- "NX CAD"는 GM에서 사용하는 Siemens NX를 의미
- 다이어그램은 영어로 작성 (발표가 영어)
