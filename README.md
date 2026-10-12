# MiniDriller - 산소가 줄어드는 지하에서 최적의 길을 찾는 2D 채굴 액션

![Unreal Engine](https://img.shields.io/badge/Unreal_Engine_5.6-0E1128?style=flat-square&logo=unrealengine&logoColor=white) ![C++](https://img.shields.io/badge/C++-00599C?style=flat-square&logo=cplusplus&logoColor=white) ![Solo](https://img.shields.io/badge/1인-개발-555555?style=flat-square) [![YouTube](https://img.shields.io/badge/YouTube-플레이_영상-FF0000?style=flat-square&logo=youtube&logoColor=white)](https://youtu.be/efCUcJviVkE)

> 본 저장소는 포트폴리오 공개용 **코드 열람 저장소**입니다. 원본 프로젝트의 외부 에셋, 사운드, 스프라이트, 빌드 파일은 저작권과 용량 문제로 제외했습니다. 플레이는 아래 실행 파일이나 영상으로 확인해 주세요.

[![MiniDriller 플레이 영상](https://img.youtube.com/vi/efCUcJviVkE/maxresdefault.jpg)](https://youtu.be/efCUcJviVkE)

▶︎ [플레이 영상](https://youtu.be/efCUcJviVkE) · 🌐 [소개 페이지](https://femur-0607.github.io/MiniDriller-Portfolio/) · 📄 [상세 보고서](https://docs.google.com/document/d/1yeZ4zxcM6ptz76q1j1rKNq6eQwSN9iCCoeyUahG_VMc/edit?usp=sharing) · 💾 [실행 파일 다운로드](https://drive.google.com/file/d/1BnZVV-5ckKqqXDWYRM3h2-618spTriuL/view?usp=drive_link) · 👤 [GitHub 프로필](https://github.com/Femur-0607)

## 프로젝트 정보

| 항목 | 내용 |
| --- | --- |
| 장르 | 2D 채굴 액션 퍼즐 |
| 개발 기간 | 2026.05 (2주, 14일) |
| 개발 형태 | 1인 프로젝트 |
| 개발 환경 | Unreal Engine 5.6.1, C++, Paper2D, PaperZD |
| 플랫폼 | PC (Windows) |
| 주요 기술 | TMap 기반 GridMap, Flood Fill, Match-4, Object Pooling, PaperZD 애니메이션 FSM |
| 상태 | 완료 (실행 파일 제공) |

MiniDriller는 Mr. Driller의 핵심 재미를 참고해 만든 UE5 C++ 2D 채굴 액션 게임입니다. 플레이어는 블록을 파괴하며 더 깊이 내려가고, 깊어질수록 산소가 줄고 장애물 확률이 늘어나는 상황에서 안전한 경로를 골라야 합니다.

![MiniDriller gameplay](GIF/Game1.gif)

## 담당 범위

1인 프로젝트로 기획, 시스템 설계, UI 구성, 사운드 적용, 테스트, 발표 자료 제작을 직접 진행했습니다.

- PaperZD 애니메이션 블루프린트를 직접 구성하고 상태 전환과 Anim Notify 흐름을 게임 로직에 연결
- `AMapManager`, `ABlock`, `ADrillerCharacter`, `UGameStatusSubsystem`의 역할을 설계하고 테스트 중 발견한 문제를 기준으로 수정
- 외부 에셋: Kenney UI/폰트, The Spriters Resource 스프라이트, 101soundboards 효과음, Gemini 생성 이미지/음악 일부

## 핵심 구현

### 1. GridMap으로 규칙상 좌표와 화면 위치를 분리

**문제.** 블록을 연속으로 파괴하면 위쪽 블록이 허공에 멈추거나 여러 블록이 같은 좌표에 겹쳤습니다. 충돌 판정 조건을 세분화해도 프레임마다 판정 순서가 달라져 해결되지 않았습니다.

**선택.** `TMap<FIntPoint, ABlock*>` GridMap을 중앙 장부로 두고, 낙하 연출을 시작하기 전에 `RemoveBlockFromGrid` → `RegisterBlock`으로 데이터 좌표를 먼저 확정했습니다. 화면 이동은 그 뒤에 따라갑니다.

**결과.** 낙하·매치 판정이 물리 충돌 순서가 아니라 논리 좌표를 기준으로 동작합니다. 격자 게임에서는 보이는 위치보다 논리 좌표를 먼저 설계해야 한다는 점을 배웠습니다.

- [MapManager.cpp](https://github.com/Femur-0607/MiniDriller-Portfolio/blob/e048b0fd9bfae287791cc47dad455e100d995081/Source/MiniDriller/Private/MapManager.cpp) · [MapManager.h](https://github.com/Femur-0607/MiniDriller-Portfolio/blob/e048b0fd9bfae287791cc47dad455e100d995081/Source/MiniDriller/Public/MapManager.h)

### 2. Flood Fill과 TSet 방문 기록으로 Match-4 판정

**문제.** 같은 색 블록이 4개 이상 이어졌는지 판정할 때 중복 탐색과 순환 재귀가 생기고, 낙하 중인 블록이 잘못 포함될 수 있습니다.

**선택.** 선택 블록에서 상하좌우 4방향으로 재귀 탐색하며 방문한 블록을 `TSet`에 기록하고, 같은 색이면서 Idle 상태인 블록만 누적합니다. 결과가 4개 이상이면 제거와 낙하 처리로 이어집니다.

**결과.** 연출 중인 블록이 판정에 끼지 않고, 같은 블록을 두 번 세지 않습니다. 직접 채굴은 4개 미만의 연결 블록도 파괴하므로 "모든 제거가 Match-4"는 아닙니다.

- [Block.cpp](https://github.com/Femur-0607/MiniDriller-Portfolio/blob/e048b0fd9bfae287791cc47dad455e100d995081/Source/MiniDriller/Private/Block.cpp) · [Block.h](https://github.com/Femur-0607/MiniDriller-Portfolio/blob/e048b0fd9bfae287791cc47dad455e100d995081/Source/MiniDriller/Public/Block.h)

![Combo gameplay](GIF/combo.gif)

### 3. 타입별 오브젝트 풀과 이중 반환 방지

**문제.** 일반·산소·장애물·배리어 블록이 레벨마다 대량 생성·파괴되고, 레벨 전환 후 남은 좌표나 두 번 반납된 블록이 다음 레벨에 끼어들었습니다.

**선택.** 블록 타입별 풀을 미리 만들고 `bIsInPool` 가드로 이중 반환을 막았습니다. 반환 시 `ResetBlockState()`가 타이머·연출·좌표를 초기화합니다. 그리드를 비울 때는 `GenerateValueArray`로 복사본을 만든 뒤 순회해 반복자 무효화 크래시를 없앴습니다.

**결과.** 레벨 전환이 안정됐고, 반복자 무효화와 MaxStepHeight 문제의 해결 과정은 [트러블슈팅 문서](Docs/Notion/troubleshooting.md)에 기록했습니다.

- [DrillerCharacter.cpp](https://github.com/Femur-0607/MiniDriller-Portfolio/blob/e048b0fd9bfae287791cc47dad455e100d995081/Source/MiniDriller/Private/DrillerCharacter.cpp) · [GameStatusSubsystem.cpp](https://github.com/Femur-0607/MiniDriller-Portfolio/blob/e048b0fd9bfae287791cc47dad455e100d995081/Source/MiniDriller/Private/GameStatusSubsystem.cpp)

## 플레이 방법

1. [실행 파일 다운로드](https://drive.google.com/file/d/1BnZVV-5ckKqqXDWYRM3h2-618spTriuL/view?usp=drive_link)에서 빌드된 게임 파일을 받습니다.
2. 압축을 풀고 폴더 안의 실행 파일을 실행합니다.

## 공개 범위와 확인 방법

- 이 저장소는 **코드 열람용**입니다. `Source/`와 `MiniDriller.uproject`는 있지만 스프라이트·사운드·블루프린트 에셋이 없어 에디터에서 열어도 플레이할 수 없습니다.
- 코드를 받아 보려면 `git clone https://github.com/Femur-0607/MiniDriller-Portfolio.git`을 사용합니다.
- 플레이는 위 실행 파일이나 [영상](https://youtu.be/efCUcJviVkE), 개발 과정은 [상세 보고서](https://docs.google.com/document/d/1yeZ4zxcM6ptz76q1j1rKNq6eQwSN9iCCoeyUahG_VMc/edit?usp=sharing)를 참고해 주세요.
- 코드 링크는 커밋 `e048b0f`에 고정돼 있어 이후 변경과 무관하게 같은 내용을 가리킵니다.

## 문서

- [기술 문서 인덱스](Docs/Notion/README.md) · [프로젝트 메인 문서](Docs/Notion/project-main.md) · [트러블슈팅 리포트](Docs/Notion/troubleshooting.md)

© 2026 Yang Ji-seop. Developed with Unreal Engine 5.
