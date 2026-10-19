# System Architecture

## Design Principles

본 프로젝트는 차량 상태와 제어 로직을 분리하여 설계하였다.

차량의 상태는 VehicleState에서 관리하고, 모든 제어 판단은 ECU에서 수행한다.

각 컴포넌트는 하나의 역할만 수행하도록 구성하여 유지보수와 확장이 가능하도록 설계하였다.

## System Components

### User Interface

사용자로부터 차량 상태를 입력받고 시뮬레이션 결과를 출력한다.

### ECU

차량 상태를 확인하여 시동 및 종료 가능 여부를 판단한다.

판단 결과에 따라 VehicleState를 변경한다.

### VehicleState

차량의 현재 상태를 저장한다.

관리하는 정보는 다음과 같다.

- Brake Position
- Gear Position
- Battery Voltage
- Engine State
- Engine RPM
- Dashboard State

### Verification Engine

최종 차량 상태를 검증하고 PASS 또는 FAIL 결과를 생성한다.

### Logger

사용자 입력과 시뮬레이션 결과를 기록한다.

## Data Flow

프로그램은 아래 순서로 동작한다.

1. 차량 상태 입력
2. ECU 제어 로직 수행
3. VehicleState 갱신
4. 결과 검증
5. 로그 기록
6. 결과 출력

## Version

- V1.0 — PC 기반 Vehicle Simulation
- V2.0 — STM32 기반 Embedded Vehicle Control (구현 및 실물 검증 완료)

V1.0은 PC에서 Engine Start/Stop 로직을 시뮬레이션하며, V2.0은 해당 C++ 로직을 재사용·확장하여 실제 STM32 보드의 UART 및 GPIO와 연동한다.

### V2.0 Main Features

- STM32 NUCLEO-G0B1RE 기반 엔진 제어
- UART Interactive CLI를 통한 Brake/Gear/Battery 설정 및 차량 상태 조회
- YuRobot Easy Module Shield V1의 SW1(PA10, Active Low) 버튼으로 Engine Start/Stop 요청
- LED1(PA5, Active High)으로 실제 Engine State 표시
- Start 조건: Brake > 0, Gear P/N, Battery 11.5~15.0V(양 끝값 포함). Stop 조건: Brake > 0
- Battery 입력 저장은 0~15V로 제한하지 않으며, 제어 실패 시 기존 차량 상태와 엔진 상태 LED 유지
- Start/Stop 실패 원인을 UART로 출력: `START_BRAKE`, `START_GEAR`, `START_BATTERY`, `STOP_BRAKE`

### V2.0 Verification

| 검증 구분 | 결과 |
|---|---|
| 호스트 자동 검사 | 최적화 설정 `/Od`, `/O2` 각각 331개 통과, 실패 0개 |
| STM32 빌드 | Debug/Release 컴파일 및 링크 성공 |
| 실제 STM32 보드 | 정상 Start/Stop, Brake/Gear/Battery 경계값 및 실패 조건 검증 성공 |
| Failure Reason | 4종 실패 원인의 UART 출력 실물 검증 성공 |

실제 보드 검증은 **사용자 수동 검증 결과**이며, 호스트 자동 검사와 구분한다.

## Documents

- [Version History](Version_History.md)
- [Architecture](docs/Architecture.md) / [Class Diagram](UML/ClassDiagram.puml)
- [V2.0 Use Case](docs/Use_Case_Ver2.0.md) / [IO List](docs/IO_List_Ver2.0.md)
- [V2.0 System Requirement](docs/System_Requirement_Ver2.0.md) / [Software Requirement](docs/Software_Requirement_Ver2.0.md)
- [V2.0 System Architecture](UML/SystemArchitecture_Ver2.0.puml) / [Software Architecture](UML/SoftwareArchitecture_Ver2.0.puml)
