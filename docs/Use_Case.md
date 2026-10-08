# Use Case

## UC-001 Engine Start

### 목적
운전자의 엔진 시동 요청에 따라 Engine Start 가능 여부를 판단하고
조건을 만족할 경우 엔진을 시동

### Actor
Driver

### 입력
- Brake Position
- Gear Position
- Battery Voltage
- Engine Button

### 사전 조건
- Engine State = OFF

### 정상 흐름
1. PC에서 Brake Position을 입력한다.
2. PC에서 Gear Position을 입력한다.
3. PC에서 Battery Voltage를 입력한다.
4. Driver가 STM32의 Engine Button을 누른다.
5. STM32가 입력 조건을 확인한다.
6. EngineController가 시동 가능 여부를 판단한다.
7. 모든 조건이 만족되면 Engine State를 ON으로 변경한다.
8. Engine RPM을 850 rpm으로 설정한다.
9. LED를 ON한다.
10. UART를 통해 엔진 상태를 PC에 출력한다.

### 실패 흐름
- Brake Position이 0인 경우 시동하지 않는다.
- Gear Position이 P/N이 아닌 경우 시동하지 않는다.
- Battery Voltage가 11.5 V 미만이거나 15.0 V를 초과하는 경우 시동하지 않는다.

### 결과
#### 성공
- Engine State = ON
- Engine RPM = 850 rpm
- LED = ON

#### 실패
- Engine State 유지
- LED = OFF
- Failure Reason을 UART로 출력

# Use Case

## UC-002 Engine Stop

### 목적
운전자의 엔진 시동 중단요청에 따라 엔진 정지 가능 여부를 판단하고
조건을 만족할 경우 엔진을 중단한다.

### Actor
Driver

### 입력
- Brake Position


### 사전 조건
- Engine State = ON

### 정상 흐름
1. PC에서 Brake Position을 입력한다.
2. Driver가 STM32의 Engine Button을 누른다.
3. STM32가 입력 조건을 확인한다.
4. EngineController가 시동 중단가능 여부를 판단한다.
5. 모든 조건이 만족되면 Engine State를 OFF로 변경한다.
6. Engine RPM을 0 rpm으로 설정한다.
7. LED를 OFF한다.
8. UART를 통해 엔진 상태를 PC에 출력한다.

### 실패 흐름
- Brake Position이 0인 경우 엔진이 중단되지 않는다.


### 결과
#### 성공
- Engine State = OFF
- Engine RPM = 0 rpm
- LED = OFF

#### 실패
- Engine State 유지
- LED = ON
- Failure Reason을 UART로 출력