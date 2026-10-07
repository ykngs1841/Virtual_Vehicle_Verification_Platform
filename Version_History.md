# Version History

## V1.0 — Vehicle Simulation

### Goal

PC 환경에서 차량의 Engine Start/Stop 제어 로직을 구현

### Main Features

- Brake Position 입력
- Gear Position 입력
- Battery Voltage 입력
- Engine Button Event 처리
- Engine Start/Stop 판단
- VehicleState 관리
- 기본적인 차량 상태 출력

### Architecture

User Interface -> EngineController -> VehicleState


## V2.0 — Embedded Vehicle Control

### Goal

V1.0의 Engine Start/Stop 제어 로직을 STM32 기반 Embedded System으로 확장

### Planned Changes

- PC ↔ STM32 Serial 통신
- PC에서 Brake / Gear / Battery 입력
- STM32 GPIO 기반 Engine Button 입력
- STM32에서 Engine Start/Stop Logic 수행
- LED를 통한 Engine State 출력
- UART를 통한 차량 상태 모니터링
- Application Logic과 Hardware Dependency 분리