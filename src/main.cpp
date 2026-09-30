#include <iostream>
#include "DriverInput.h"
#include "VehicleState.h"
#include "EngineController.h"

int main()
{
    std::cout << "Virtual Vehicle Verification Platform Start" << std::endl;
    DriverInput driverInput;
    VehicleState vehicleState;

    // EngineController 생성
    EngineController engineController(
        driverInput,
        vehicleState
    );
    float batteryVoltage;
    std::cout << "Battery Voltage: ";
    std::cin >> batteryVoltage;
    vehicleState.setBatteryVoltage(batteryVoltage);

	int Brakeposition;
	std::cout << "Brake Position(0-100): ";
    std::cin >> Brakeposition;
	driverInput.setBrakePosition(Brakeposition);

    // 운전자 입력
    char gearInput;

    std::cout << "Gear Position(P/R/N/D): ";
    std::cin >> gearInput;

    if (gearInput == 'P')
    {
        driverInput.setGearPosition(GearPosition::P);
    }
    else if (gearInput == 'R')
    {
        driverInput.setGearPosition(GearPosition::R);
    }
    else if (gearInput == 'N')
    {
        driverInput.setGearPosition(GearPosition::N);
    }
    else if (gearInput == 'D')
    {
        driverInput.setGearPosition(GearPosition::D);
    }
    driverInput.pressEngineButton();
    // ECU에 버튼 이벤트 전달
    engineController.processEngineButton();


    std::cout << "Engine State: "
        << vehicleState.getEngineState() << std::endl;

    std::cout << "Engine RPM: "
        << vehicleState.getEngineRpm() << std::endl;
    return 0;
}