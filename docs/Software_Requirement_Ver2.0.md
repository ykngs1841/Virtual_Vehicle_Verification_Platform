# VVVP V2.0 — Software Requirements

## 1. 문서 목적 및 범위

본 문서는 [System_Requirement_Ver2.0.md](System_Requirement_Ver2.0.md)의 SYS-001~SYS-021을 STM32에서 실행할 소프트웨어의 검증 가능한 동작으로 구체화한다. 범위는 UC-001 Engine Start 및 UC-002 Engine Stop에서 요구하는 입력 반영, 제어 판단, 상태 변경, LED 상태 유지 및 UART 결과 전달이다.

본 문서는 소프트웨어 동작 요구사항을 정의하며, UART 메시지 형식, GPIO 핀 매핑, 버튼 이벤트 수집 방식, 특정 HAL API, 태스크 또는 인터럽트 구조를 확정하지 않는다. V1.0 소스코드는 동결 대상이며 수정 범위에 포함되지 않는다. V2.0 구현 목표를 현재 구현 완료 상태로 간주하지 않는다.

## 2. V1.0 재사용 및 V2.0 신규 구분

- **V1.0 재사용:** `DriverInput`, `VehicleState`, `EngineController`의 현재 구현에서 확인되는 상태 저장 의미 또는 Start/Stop 제어 동작을 재사용하는 요구사항이다.
- **V2.0 신규:** UART 및 물리 GPIO 입력 처리, UART 결과 전달, 물리 LED 제어, 또는 V2.0에서 새로 확정한 제어 조건을 구현하는 요구사항이다.
- **재사용 + 신규:** V1.0의 상태/제어 의미를 유지하면서 V2.0 입출력 경로 또는 동작을 추가하는 요구사항이다.

V1.0 `DriverInput`은 Brake Position, Gear Position, Engine Button event를 저장하며 setter에서 범위 검증을 수행하지 않는다. `VehicleState`는 Battery Voltage(float), Engine State, Engine RPM 및 Dashboard State를 저장하며 setter에서 값 범위를 검증하지 않는다. `EngineController`는 Start 시 Brake > 0, Gear P/N, Battery Voltage >= 11.5V를 판단하고, Stop 시 Brake > 0만 판단한다. V1.0에는 15.0V 상한, UART, 물리 버튼 수집, 물리 LED 출력 및 Failure Reason 전달이 구현되어 있지 않다. 아래 요구사항의 해당 기능은 V2.0 신규 목표다.

## 3. 소프트웨어 요구사항

### 3.1 PC 입력 수신 및 상태 반영

| ID | 소프트웨어 요구사항 | 상위 SYS | 구분 |
|---|---|---|---|
| SW-001 | 소프트웨어는 PC에서 수신해 정상적으로 해석한 Brake Position 입력값을 `DriverInput`에 저장해야 하며, 저장된 값은 해당 입력값과 같아야 한다. | SYS-001, SYS-004 | 재사용 + 신규 UART 입력 경로 |
| SW-002 | 소프트웨어는 PC에서 수신해 정상적으로 해석한 Gear Position 입력값을 `DriverInput`에 저장해야 하며, 저장된 값은 해당 입력값과 같아야 한다. | SYS-002, SYS-005 | 재사용 + 신규 UART 입력 경로 |
| SW-003 | 소프트웨어는 PC에서 수신해 정상적으로 float 값으로 해석한 Battery Voltage를 `VehicleState`에 저장해야 하며, 저장된 값은 해당 입력값과 같아야 한다. 저장 여부를 0~15V 범위 포함 여부로 제한해서는 안 된다. | SYS-003, SYS-006 | V1.0 저장 의미 재사용 + 신규 UART 입력 경로 |

### 3.2 Engine Button 입력 및 Start 제어

| ID | 소프트웨어 요구사항 | 상위 SYS | 구분 |
|---|---|---|---|
| SW-004 | STM32의 물리 Engine Button 입력이 인식되면 소프트웨어는 이를 논리적 Engine Button 요청으로 처리하고, 해당 요청을 엔진 제어 판단에 전달해야 한다. | SYS-007 | V2.0 신규 GPIO 입력 경로; V1.0 이벤트 의미 재사용 가능 |
| SW-005 | Engine Button 요청 처리 시 Engine State가 OFF이면 소프트웨어는 해당 요청을 Start 요청으로 처리해야 한다. | SYS-008 | V1.0 재사용 |
| SW-006 | Start 요청 시 소프트웨어는 `DriverInput`에 저장된 Brake Position과 Gear Position 및 `VehicleState`에 저장된 Battery Voltage와 Engine State를 참조해 SYS-009의 조건을 평가해야 하며, Brake Position > 0, Gear Position P 또는 N, 11.5V ≤ Battery Voltage ≤ 15.0V, Engine State OFF를 모두 만족할 때만 Start를 허용해야 한다. | SYS-009 | V1.0 조건 재사용 + V2.0 신규 15.0V 상한 |
| SW-007 | Start가 허용되면 소프트웨어는 `VehicleState`의 Engine State를 ON으로, Engine RPM을 850rpm으로 변경해야 한다. | SYS-010 | V1.0 재사용 |
| SW-008 | Start가 허용되면 소프트웨어는 Engine State LED를 ON으로 표시해야 한다. | SYS-010 | V2.0 신규 |
| SW-009 | Start 성공 후 소프트웨어는 갱신된 Engine State와 Engine RPM을 UART를 통해 PC에 전달해야 한다. | SYS-011 | V2.0 신규 |
| SW-010 | Start가 거부되면 소프트웨어는 요청 처리 직전의 `VehicleState` 전체 값을 유지해야 한다. | SYS-012 | V1.0 상태 비변경 동작 재사용 및 SYS 범위 명시 |
| SW-011 | Start가 거부되면 소프트웨어는 요청 처리 직전의 Engine State LED 상태를 유지해야 한다. | SYS-013 | V2.0 신규 |
| SW-012 | Start가 거부되면 소프트웨어는 Failure Reason을 UART를 통해 PC에 전달해야 한다. | SYS-014 | V2.0 신규 |

### 3.3 Engine Stop 제어

| ID | 소프트웨어 요구사항 | 상위 SYS | 구분 |
|---|---|---|---|
| SW-013 | Engine Button 요청 처리 시 Engine State가 ON이면 소프트웨어는 해당 요청을 Stop 요청으로 처리해야 한다. | SYS-015 | V1.0 재사용 |
| SW-014 | Stop은 Engine State가 ON이고 Brake Position > 0일 때만 허용되어야 하며, Stop 허용 판단은 Gear Position 또는 Battery Voltage 값에 의존해서는 안 된다. | SYS-016 | V1.0 재사용 |
| SW-015 | Stop이 허용되면 소프트웨어는 `VehicleState`의 Engine State를 OFF로, Engine RPM을 0rpm으로 변경해야 한다. | SYS-017 | V1.0 재사용 |
| SW-016 | Stop이 허용되면 소프트웨어는 Engine State LED를 OFF로 표시해야 한다. | SYS-017 | V2.0 신규 |
| SW-017 | Stop 성공 후 소프트웨어는 갱신된 Engine State와 Engine RPM을 UART를 통해 PC에 전달해야 한다. | SYS-018 | V2.0 신규 |
| SW-018 | Stop이 거부되면 소프트웨어는 요청 처리 직전의 `VehicleState` 전체 값을 유지해야 한다. | SYS-019 | V1.0 상태 비변경 동작 재사용 및 SYS 범위 명시 |
| SW-019 | Stop이 거부되면 소프트웨어는 요청 처리 직전의 Engine State LED 상태를 유지해야 한다. | SYS-020 | V2.0 신규 |
| SW-020 | Stop이 거부되면 소프트웨어는 Failure Reason을 UART를 통해 PC에 전달해야 한다. | SYS-021 | V2.0 신규 |

## 4. 미확정 사항 (TBD)

다음 항목은 상위 SYS/Use Case 또는 현재 V1.0 소스만으로 구체적인 소프트웨어 처리 정책을 확정할 수 없다. 이를 임의의 정상 동작이나 기본값으로 간주하지 않는다.

| TBD ID | 미확정 사항 | 영향 요구사항 |
|---|---|---|
| TBD-SW-001 | UART의 메시지 형식, 인코딩, 프레이밍, 값 단위 및 입력/출력 상관관계 | SW-001~SW-003, SW-009, SW-012, SW-017, SW-020 |
| TBD-SW-002 | 잘못된 형식의 Brake/Gear/Battery 입력을 수신·저장할지 여부 및 오류 보고 동작 | SW-001~SW-003 |
| TBD-SW-003 | Battery Voltage의 NaN, Infinity, 숫자 오버플로 및 float 변환 경계 처리 | SW-003, SW-006 |
| TBD-SW-004 | 부분 수신, 입력 미수신, 통신 오류, 입력 갱신 순서 및 오래된 값으로 제어 요청이 들어온 경우의 처리 | SW-001~SW-006, SW-009, SW-012, SW-017, SW-020 |
| TBD-SW-005 | Gear Position의 유효 표현, 미지원/잘못된 값의 반영 여부 및 Start 판단 처리 | SW-002, SW-006 |
| TBD-SW-006 | GPIO 포트·핀 매핑, 입력 극성, 전기적 구성, 버튼 디바운스 및 이벤트 수집 방식 | SW-004 |
| TBD-SW-007 | Failure Reason의 상세 코드/문구, 조건별 매핑, 생성 시점 및 UART 출력 표현 | SW-012, SW-020 |
| TBD-SW-008 | Engine State LED의 GPIO 매핑 및 실제 출력 갱신 시점·오류 처리 | SW-008, SW-011, SW-016, SW-019 |
| TBD-SW-009 | 초기화 직후 Engine State, RPM, LED 및 DriverInput의 초기/동기화 정책 | SW-001~SW-020 중 초기 상태에 의존하는 동작 |
| TBD-SW-010 | V2.0의 UART/GPIO 입출력 처리와 기존 제어 로직 사이의 SW 컴포넌트 경계 및 인터페이스 | SW-001~SW-020 |

위 TBD 중 특히 입력 형식/통신 오류, 초기 상태, Failure Reason의 생성·전달 정책은 요구사항 기반의 재현 가능한 소프트웨어 검증 절차를 구체화하는 데 영향을 준다. TBD 결정 전에는 그 세부 동작을 이 문서의 요구사항으로 간주하지 않는다.

## 5. SYS 요구사항 커버리지

| 상위 SYS ID | 대응 SW 요구사항 | 커버리지 메모 |
|---|---|---|
| SYS-001 | SW-001 | Brake UART 입력 수신·반영 |
| SYS-002 | SW-002 | Gear UART 입력 수신·반영 |
| SYS-003 | SW-003 | Battery float 수신·저장, 0~15V 입력 제한 없음 |
| SYS-004 | SW-001 | Brake 상태 반영 |
| SYS-005 | SW-002 | Gear 상태 반영 |
| SYS-006 | SW-003 | Start 조건과 독립된 Battery 저장; SYS-003과 같은 동작으로 한 번만 구체화 |
| SYS-007 | SW-004 | STM32 물리 버튼 요청 입력 |
| SYS-008 | SW-005 | OFF 상태에서 Start 요청 처리 |
| SYS-009 | SW-006 | Start 허용 조건 및 15.0V 상한 |
| SYS-010 | SW-007, SW-008 | 성공 시 VehicleState 변경 및 LED ON |
| SYS-011 | SW-009 | Start 성공 상태 UART 전달 |
| SYS-012 | SW-010 | Start 거부 시 VehicleState 유지 |
| SYS-013 | SW-011 | Start 거부 시 LED 상태 유지 |
| SYS-014 | SW-012 | Start 거부 Failure Reason UART 전달 |
| SYS-015 | SW-013 | ON 상태에서 Stop 요청 처리 |
| SYS-016 | SW-014 | Brake만을 포함한 Stop 허용 조건 |
| SYS-017 | SW-015, SW-016 | 성공 시 VehicleState 변경 및 LED OFF |
| SYS-018 | SW-017 | Stop 성공 상태 UART 전달 |
| SYS-019 | SW-018 | Stop 거부 시 VehicleState 유지 |
| SYS-020 | SW-019 | Stop 거부 시 LED 상태 유지 |
| SYS-021 | SW-020 | Stop 거부 Failure Reason UART 전달 |

SYS-001~SYS-021은 모두 하나 이상의 SW 요구사항에 연결되어 있다. SYS-003 및 SYS-006은 Battery Voltage 저장 동작의 중복 요구사항이므로 SW-003 하나에 함께 추적했으며, 중복 SW 요구사항을 만들지 않았다.

## 6. 검증 시 확인할 주요 항목

- SW-001~SW-003: 정상 해석 입력이 해당 상태에 반영되는지, 특히 0~15V 밖의 정상 숫자 Battery Voltage가 입력 저장 단계에서 제한되지 않는지 확인한다.
- SW-006: Brake, Gear, Battery 및 Engine State 조건 조합과 11.5V 및 15.0V 경계값을 확인한다. 입력 parser 경계 정책은 TBD 결정 후 별도로 확인한다.
- SW-007~SW-012: Start 성공/거부 시 상태, LED, UART 결과 및 실패 상태 보존을 각각 확인한다.
- SW-014~SW-020: Stop 성공/거부 시 Gear 및 Battery에 의존하지 않는 판단, 상태, LED, UART 결과 및 실패 상태 보존을 확인한다.
- 물리 UART/GPIO를 포함하는 검증의 메시지·핀·이벤트 조건은 관련 TBD와 하드웨어 인터페이스 설계가 확정된 뒤 구체화한다.
