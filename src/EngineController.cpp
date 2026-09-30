#include "EngineController.h"

EngineController::EngineController(
    DriverInput& driverInput,
    VehicleState& vehicleState)
    : driverInput(driverInput),
    vehicleState(vehicleState)
{
}

void EngineController::processEngineButton()
{
    if (!driverInput.isEngineButtonPressed())
    {
        return;
    }

    if (vehicleState.getEngineState() == false)
    {
        if (driverInput.getBrakePosition() > 0 &&(driverInput.getGearPosition() == GearPosition::P ||driverInput.getGearPosition() == GearPosition::N) &&vehicleState.getBatteryVoltage() >= 11.5f)
        {
            vehicleState.setEngineState(true);
            vehicleState.setEngineRpm(850);
            vehicleState.setDashboardState(true);
        }
    }
    else
    {
     if (driverInput.getBrakePosition() > 0)
     {
            vehicleState.setEngineState(false);
            vehicleState.setEngineRpm(0);
            vehicleState.setDashboardState(false);
     }
    }

    driverInput.clearEngineButtonEvent();
}