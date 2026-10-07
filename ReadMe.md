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
- V2.0 — STM32 기반 Embedded Vehicle Control (In Progress)