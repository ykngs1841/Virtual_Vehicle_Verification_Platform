# IO List — V2.0

## 문서 범위

- 본 문서는 STM32 기반 Embedded Vehicle Control을 목표로 하는 V2.0의 논리 Input / Output을 정의한다.
- 본 문서는 설계 기준이며, UART, 물리 버튼, LED 기능이 현재 구현 완료되었음을 의미하지 않는다.
- V1.0 기준 IO 정의는 [IO_List.md](IO_List.md)를 참조한다. V1.0 소스코드와 동작 의미는 변경하지 않는다.
- 본 문서는 Use Case의 기능 동작을 정리하며, UART 메시지 구조와 GPIO 핀 배치 등 상세 인터페이스는 확정하지 않는다.

## 입력 처리 원칙

- 입력 형식의 수신·해석과 EngineController의 차량 제어 조건 판단은 서로 다른 처리 단계이다.
- 정상적으로 숫자로 해석된 값은 제어 조건을 만족하는지와 관계없이 해당 입력 상태에 반영한다.
- 입력의 잘못된 형식에 대한 오류 응답 및 복구 방식은 TBD이다.

## DriverInput

### Brake Position

브레이크 페달 입력값

- Type: Integer
- Nominal Domain: 0 ~ 100
- Unit: %
- V2.0 Source: PC input over UART
- Storage: 정상 수신·해석된 값을 DriverInput에 반영
- Description:
  - 0: 브레이크 미입력
  - 1 ~ 99: 브레이크 강도
  - 100: 최대 제동
- 입력 형식 검증 및 Nominal Domain 밖 값의 처리: TBD

### Gear Position

변속기 위치

- Type: Enum
- Values: P, R, N, D
- Unit: N/A
- V2.0 Source: PC input over UART
- Storage: 정상 수신·해석된 값을 DriverInput에 반영
- 유효하지 않은 Gear 표현의 수신 처리: TBD

## VehicleState

### Battery Voltage

차량 배터리 전압

- Type: Float
- Input Storage Domain: 0 ~ 15 V로 제한하지 않음
- Unit: V
- V2.0 Source: PC input over UART
- Storage: 정상적으로 숫자로 해석된 값은 시동 허용 범위와 관계없이 VehicleState에 반영
- 입력 형식 오류 처리: TBD

### Engine State

엔진 상태

- Type: Boolean
- Values: ON / OFF
- Owner: VehicleState

### Engine RPM

엔진 회전수

- Type: Integer
- Nominal Domain: 0 ~ 8000
- Unit: rpm
- Owner: VehicleState

## Event Input

### Engine Button

운전자의 엔진 제어 요청을 발생시키는 물리 버튼

- Logical Type: Event
- V2.0 Source: STM32 physical input (GPIO)
- Press event: EngineController에 제어 요청 전달
- GPIO pin number and electrical configuration: TBD
- Button debounce and event capture method: TBD

## ECU Control Conditions

### Engine Start

Engine State가 OFF일 때 Engine Button 요청을 처리한다. 다음 조건을 모두 만족하면 Start를 허용한다.

- Brake Position > 0
- Gear Position = P 또는 N
- 11.5 V ≤ Battery Voltage ≤ 15.0 V
- Engine State = OFF

Start 성공 시:

- Engine State = ON
- Engine RPM = 850 rpm
- LED = ON
- UART로 Engine State와 Engine RPM 전달

Start 실패 시:

- EngineController의 요청 처리 직전 VehicleState 전체 유지
- LED 상태 유지
- Failure Reason을 UART로 전달

### Engine Stop

Engine State가 ON일 때 Engine Button 요청을 처리한다. 다음 조건을 모두 만족하면 Stop을 허용한다.

- Brake Position > 0
- Engine State = ON

Stop 조건에는 Gear Position 및 Battery Voltage를 포함하지 않는다.

Stop 성공 시:

- Engine State = OFF
- Engine RPM = 0 rpm
- LED = OFF
- UART로 Engine State와 Engine RPM 전달

Stop 실패 시:

- EngineController의 요청 처리 직전 VehicleState 전체 유지
- LED 상태 유지
- Failure Reason을 UART로 전달

## ECU Outputs

### Engine State Indicator

- Logical Signal: Engine State에 따른 LED 표시
- Engine State ON: LED ON
- Engine State OFF: LED OFF
- LED pin number, polarity, and electrical configuration: TBD
- LED는 V2.0 구현 목표이며 현재 V1.0 구현 기능이 아니다.

### Vehicle Status over UART

- Engine State
- Engine RPM
- Transmission encoding, message structure, framing, update timing, and error handling: TBD
- UART 상태 출력은 V2.0 구현 목표이며 현재 V1.0 구현 기능이 아니다.

### Control Result and Failure Reason

- Control Result: PASS / FAIL (logical result)
- Failure Reason: String or encoded value; exact values and representation: TBD
- UC-001/UC-002 실패 시 Failure Reason을 UART로 전달한다.
- 조건별 Failure Reason 매핑 및 UART 메시지 포함 방식: TBD

## V1.0과의 버전 경계

- V1.0의 Battery Voltage Start 조건은 `Battery Voltage ≥ 11.5 V`이며, V1.0 코드에 15.0 V 상한 조건은 없다.
- V2.0의 Start 허용 전압은 `11.5 V ≤ Battery Voltage ≤ 15.0 V`이다.
- V2.0의 UART 입력·출력, 물리 Engine Button, LED 출력은 구현 목표이며 V1.0 완료 기능으로 간주하지 않는다.
