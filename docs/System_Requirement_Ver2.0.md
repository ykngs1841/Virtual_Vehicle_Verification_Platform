# VVVP V2.0 — System Requirements

## 1. 문서 목적 및 적용 범위

본 문서는 VVVP V2.0 STM32 기반 Embedded Vehicle Control의 시스템 수준 기능 요구사항을 정의한다. 요구사항은 [Use_Case_Ver2.0.md](Use_Case_Ver2.0.md)의 UC-001 Engine Start와 UC-002 Engine Stop, 그리고 [IO_List_Ver2.0.md](IO_List_Ver2.0.md)의 논리 I/O 정의를 바탕으로 한다.

V1.0의 Engine Start/Stop 조건 및 VehicleState 변화 의미를 가능한 한 재사용한다. V2.0의 UART, STM32 물리 버튼 및 LED는 목표 기능이며, 현재 구현 완료로 간주하지 않는다. V1.0 소스코드는 본 문서의 범위에서 변경하지 않는다.

본 문서는 시스템 외부에서 확인 가능한 기능과 결과를 정의한다. UART 상세 프로토콜, GPIO 핀 구성, 소프트웨어 클래스·함수 구조 및 하드웨어 인터페이스 설계는 정의하지 않는다.

## 2. 관련 문서

- [Use_Case_Ver2.0.md](Use_Case_Ver2.0.md) — V2.0 시스템 Use Case
- [IO_List_Ver2.0.md](IO_List_Ver2.0.md) — V2.0 논리 Input / Output
- [IO_List.md](IO_List.md) — V1.0 IO 정의 참고
- `include/`, `src/` — V1.0 기존 동작 및 상태 의미 참고

## 3. 시스템 기능 요구사항

구분:

- **V1.0 재사용:** 기존 V1.0의 제어 조건 또는 상태 변화 의미를 V2.0에서도 유지
- **V2.0 신규:** V2.0의 외부 인터페이스 또는 추가 제어 조건
- **재사용 + 신규 인터페이스:** 기존 기능 의미를 유지하면서 V2.0 입출력 경로를 추가

| ID | 요구사항 | 관련 Use Case | 구분 |
|---|---|---|---|
| SYS-001 | 시스템은 PC가 UART로 전달한 Brake Position 입력을 수신해야 한다. | UC-001, UC-002 | V2.0 신규 |
| SYS-002 | 시스템은 PC가 UART로 전달한 Gear Position 입력을 수신해야 한다. | UC-001 | V2.0 신규 |
| SYS-003 | 시스템은 PC가 UART로 전달한 Battery Voltage 입력을 수신해야 한다. Battery Voltage는 float 값으로 취급하며, 정상적으로 숫자로 해석된 값은 0~15V 범위 밖이더라도 입력 저장 단계에서 범위 제한 없이 VehicleState에 반영해야 한다. | UC-001 | V2.0 신규 인터페이스; V1.0 VehicleState 의미 재사용 |
| SYS-004 | 시스템은 정상적으로 수신·해석한 Brake Position을 해당 운전자 입력 상태에 반영해야 한다. | UC-001, UC-002 | 재사용 + 신규 인터페이스 |
| SYS-005 | 시스템은 정상적으로 수신·해석한 Gear Position을 해당 운전자 입력 상태에 반영해야 한다. | UC-001 | 재사용 + 신규 인터페이스 |
| SYS-006 | 시스템은 정상적으로 수신·해석한 Battery Voltage를 시동 허용 조건과 별개로 VehicleState에 반영해야 한다. | UC-001 | 재사용 + 신규 인터페이스 |
| SYS-007 | 시스템은 STM32 물리 Engine Button 입력을 엔진 제어 요청으로 받아들여야 한다. | UC-001, UC-002 | 재사용 이벤트 의미 + V2.0 신규 인터페이스 |
| SYS-008 | Engine State가 OFF인 상태에서 Engine Button 요청을 처리할 때 시스템은 해당 요청을 Engine Start 요청으로 판단해야 한다. | UC-001 | V1.0 재사용 |
| SYS-009 | Engine Start 요청에 대해 시스템은 Brake Position > 0, Gear Position이 P 또는 N, Battery Voltage가 11.5V 이상 15.0V 이하, Engine State가 OFF인 경우에만 Start를 허용해야 한다. | UC-001 | V1.0 조건 재사용 + V2.0 15.0V 상한 신규 |
| SYS-010 | Start가 허용되면 시스템은 Engine State를 ON, Engine RPM을 850rpm으로 설정하고 LED를 ON으로 표시해야 한다. | UC-001 | VehicleState 의미 재사용 + V2.0 LED 신규 |
| SYS-011 | Start가 허용되면 시스템은 Engine State와 Engine RPM을 UART를 통해 PC에 전달해야 한다. | UC-001 | V2.0 신규 |
| SYS-012 | Engine Start 요청이 허용되지 않으면 시스템은 EngineController 요청 처리 직전의 VehicleState 전체를 유지해야 한다. | UC-001 | V1.0 상태 유지 의미 재사용 및 범위 명확화 |
| SYS-013 | Engine Start 요청이 허용되지 않으면 시스템은 LED 상태를 요청 처리 직전과 동일하게 유지해야 한다. | UC-001 | V2.0 신규 |
| SYS-014 | Engine Start 요청이 허용되지 않으면 시스템은 Failure Reason을 UART를 통해 PC에 전달해야 한다. | UC-001 | V2.0 신규 출력; 상세 값 TBD |
| SYS-015 | Engine State가 ON인 상태에서 Engine Button 요청을 처리할 때 시스템은 해당 요청을 Engine Stop 요청으로 판단해야 한다. | UC-002 | V1.0 재사용 |
| SYS-016 | Engine Stop 요청에 대해 시스템은 Brake Position > 0이고 Engine State가 ON인 경우에만 Stop을 허용해야 한다. Stop 허용 판단에는 Gear Position과 Battery Voltage를 사용하지 않아야 한다. | UC-002 | V1.0 재사용 |
| SYS-017 | Stop이 허용되면 시스템은 Engine State를 OFF, Engine RPM을 0rpm으로 설정하고 LED를 OFF로 표시해야 한다. | UC-002 | VehicleState 의미 재사용 + V2.0 LED 신규 |
| SYS-018 | Stop이 허용되면 시스템은 Engine State와 Engine RPM을 UART를 통해 PC에 전달해야 한다. | UC-002 | V2.0 신규 |
| SYS-019 | Engine Stop 요청이 허용되지 않으면 시스템은 EngineController 요청 처리 직전의 VehicleState 전체를 유지해야 한다. | UC-002 | V1.0 상태 유지 의미 재사용 및 범위 명확화 |
| SYS-020 | Engine Stop 요청이 허용되지 않으면 시스템은 LED 상태를 요청 처리 직전과 동일하게 유지해야 한다. | UC-002 | V2.0 신규 |
| SYS-021 | Engine Stop 요청이 허용되지 않으면 시스템은 Failure Reason을 UART를 통해 PC에 전달해야 한다. | UC-002 | V2.0 신규 출력; 상세 값 TBD |

### 3.1 V1.0 재사용 범위에 대한 주의

V1.0 소스코드에는 DriverInput, VehicleState, EngineController를 통한 기본 Start/Stop 조건과 상태 변화가 구현되어 있다. V1.0 코드는 배터리 하한 11.5V만 판단하며, 15.0V 상한, Failure Reason 출력, UART 통신, STM32 물리 버튼 및 LED 출력은 구현하지 않는다. 따라서 해당 V2.0 기능은 V1.0 완료 기능이 아니라 신규 요구사항이다.

V1.0의 `Dashboard State`와 V2.0의 물리 LED는 동일한 출력으로 간주하지 않는다. V2.0 LED 동작은 V2.0 신규 외부 출력이다.

## 4. 미확정 사항 (TBD)

다음 항목은 시스템 요구사항의 기능적 의도를 넘어서는 정책·인터페이스 세부사항으로, 후속 설계에서 확정한다.

| TBD ID | 미확정 사항 | 관련 요구사항 |
|---|---|---|
| TBD-001 | UART 메시지 형식, 인코딩, 프레이밍 및 신호 단위 표현 | SYS-001~SYS-006, SYS-011, SYS-014, SYS-018, SYS-021 |
| TBD-002 | GPIO 핀 번호, 전기적 구성 및 입력 극성 | SYS-007 |
| TBD-003 | 버튼 디바운스와 물리 입력을 논리 이벤트로 수집하는 방법 | SYS-007 |
| TBD-004 | 입력 미수신 또는 부분 수신 시 상태 갱신 및 제어 요청 처리 | SYS-001~SYS-006, SYS-008~SYS-009, SYS-015~SYS-016 |
| TBD-005 | NaN, Infinity 및 숫자 오버플로 표현의 수신·저장 처리 | SYS-003, SYS-006 |
| TBD-006 | 잘못된 형식 또는 범위 밖 Brake/Gear 입력의 수신·저장 처리 | SYS-001, SYS-002, SYS-004~SYS-005 |
| TBD-007 | Failure Reason의 상세 코드·문구, 조건별 매핑 및 UART 전달 형식 | SYS-014, SYS-021 |
| TBD-008 | 통신 오류 감지, 복구 및 오류 상태 보고 정책 | SYS-001~SYS-006, SYS-011, SYS-014, SYS-018, SYS-021 |
| TBD-009 | V2.0 시스템 초기화 후 Engine State, RPM, LED 등 초기 상태 중 Use Case에서 정하지 않은 값 | SYS-010~SYS-021 |

## 5. Use Case ↔ System Requirement 추적성

| Use Case | 연결된 System Requirement |
|---|---|
| UC-001 Engine Start | SYS-001, SYS-002, SYS-003, SYS-004, SYS-005, SYS-006, SYS-007, SYS-008, SYS-009, SYS-010, SYS-011, SYS-012, SYS-013, SYS-014 |
| UC-002 Engine Stop | SYS-001, SYS-004, SYS-007, SYS-015, SYS-016, SYS-017, SYS-018, SYS-019, SYS-020, SYS-021 |

## 6. 향후 검증 시 확인할 주요 항목

- 정상 수신된 Brake, Gear, Battery 입력이 각각 해당 상태에 반영되는지 확인한다.
- 0~15V 범위 밖의 정상 숫자 Battery Voltage도 입력 단계에서 제한되지 않고 저장되는지 확인한다.
- Start 허용 조건의 경계값인 11.5V와 15.0V가 허용되고, 그보다 낮거나 높은 값은 거부되는지 확인한다.
- Start 조건 중 Brake, Gear, Engine State 각 조건의 만족·불만족에 따른 결과를 확인한다.
- Start 성공 시 Engine State, RPM, LED 및 UART 상태 전달 결과를 확인한다.
- Stop 판단이 Brake > 0과 Engine State ON에 의존하며 Gear 및 Battery 값에 의존하지 않는지 확인한다.
- Stop 성공 시 Engine State, RPM, LED 및 UART 상태 전달 결과를 확인한다.
- Start/Stop 실패 시 요청 처리 직전 VehicleState 전체와 LED 상태가 유지되는지 확인한다.
- 실패 시 Failure Reason이 UART로 전달되는지 확인한다. 구체 코드·매핑은 TBD 확정 후 검증 기준에 반영한다.
- 입력 형식, 통신 오류, 버튼 입력 세부 동작 및 초기 상태의 검증 기준은 해당 TBD 정책이 확정된 뒤 정의한다.
