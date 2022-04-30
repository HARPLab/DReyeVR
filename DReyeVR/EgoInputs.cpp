#include "EgoVehicle.h"
#include "Math/NumericLimits.h" // TNumericLimits<float>::Max
#include <string>               // std::string, std::wstring

////////////////:INPUTS:////////////////
/// NOTE: Here we define all the Input functions for the EgoVehicle just to keep them
// from cluttering the EgoVehcile.cpp file

const DReyeVR::UserInputs &AEgoVehicle::GetVehicleInputs() const
{
    return VehicleInputs;
}

void AEgoVehicle::CameraFwd()
{
    CameraPositionAdjust(FVector(1.f, 0.f, 0.f));
}

void AEgoVehicle::CameraBack()
{
    CameraPositionAdjust(FVector(-1.f, 0.f, 0.f));
}

void AEgoVehicle::CameraLeft()
{
    CameraPositionAdjust(FVector(0.f, -1.f, 0.f));
}

void AEgoVehicle::CameraRight()
{
    CameraPositionAdjust(FVector(0.f, 1.f, 0.f));
}

void AEgoVehicle::CameraUp()
{
    CameraPositionAdjust(FVector(0.f, 0.f, 1.f));
}

void AEgoVehicle::CameraDown()
{
    CameraPositionAdjust(FVector(0.f, 0.f, -1.f));
}

void AEgoVehicle::CameraPositionAdjust(const FVector &displacement)
{
    const FVector &CurrentRelLocation = VRCameraRoot->GetRelativeLocation();
    VRCameraRoot->SetRelativeLocation(CurrentRelLocation + displacement);
}

void AEgoVehicle::SetSteering(const float SteeringInput)
{
    float ScaledSteeringInput = this->ScaleSteeringInput * SteeringInput;
    this->GetVehicleMovementComponent()->SetSteeringInput(ScaledSteeringInput); // UE4 control
    // assign to input struct
    VehicleInputs.Steering = ScaledSteeringInput;
}

void AEgoVehicle::SetThrottle(const float ThrottleInput)
{
    float ScaledThrottleInput = this->ScaleThrottleInput * ThrottleInput;
    this->GetVehicleMovementComponent()->SetThrottleInput(ScaledThrottleInput); // UE4 control

    // apply new light state
    FVehicleLightState Lights = this->GetVehicleLightState();
    Lights.Reverse = false;
    Lights.Brake = false;
    this->SetVehicleLightState(Lights);

    // assign to input struct
    VehicleInputs.Throttle = ScaledThrottleInput;
}

void AEgoVehicle::SetBrake(const float BrakeInput)
{
    float ScaledBrakeInput = this->ScaleBrakeInput * BrakeInput;
    this->GetVehicleMovementComponent()->SetBrakeInput(ScaledBrakeInput); // UE4 control

    // apply new light state
    FVehicleLightState Lights = this->GetVehicleLightState();
    Lights.Reverse = false;
    Lights.Brake = true;
    this->SetVehicleLightState(Lights);

    // assign to input struct
    VehicleInputs.Brake = ScaledBrakeInput;
}

void AEgoVehicle::PressReverse()
{
    if (!bCanPressReverse)
        return;
    bCanPressReverse = false; // don't press again until release

    // negate to toggle bw + (forwards) and - (backwards)
    const int CurrentGear = this->GetVehicleMovementComponent()->GetTargetGear();
    const int NewGear = CurrentGear != 0 ? -1 * CurrentGear : -1; // set to -1 if parked, else -gear
    this->bReverse = !this->bReverse;
    this->GetVehicleMovementComponent()->SetTargetGear(NewGear, true); // UE4 control

    // apply new light state
    FVehicleLightState Lights = this->GetVehicleLightState();
    Lights.Reverse = this->bReverse;
    this->SetVehicleLightState(Lights);

    UE_LOG(LogTemp, Log, TEXT("Toggle Reverse"));
    // assign to input struct
    VehicleInputs.ToggledReverse = true;

    this->PlayGearShiftSound();
}

void AEgoVehicle::ReleaseReverse()
{
    VehicleInputs.ToggledReverse = false;
    bCanPressReverse = true;
}

void AEgoVehicle::PressTurnSignalR()
{
    if (!bCanPressTurnSignalR)
        return;
    bCanPressTurnSignalR = false; // don't press again until release
    // store in local input container
    VehicleInputs.TurnSignalRight = true;

    if (bEnableTurnSignalAction)
    {
        // apply new light state
        FVehicleLightState Lights = this->GetVehicleLightState();
        Lights.RightBlinker = true;
        Lights.LeftBlinker = false;
        this->SetVehicleLightState(Lights);
        // play sound
        this->PlayTurnSignalSound();
    }
    RightSignalTimeToDie = TNumericLimits<float>::Max(); // wait until button released (+inf until then)
    LeftSignalTimeToDie = 0.f;                           // immediately stop left signal
}

void AEgoVehicle::ReleaseTurnSignalR()
{
    if (bCanPressTurnSignalR)
        return;
    VehicleInputs.TurnSignalRight = false;
    RightSignalTimeToDie = FPlatformTime::Seconds() + this->TurnSignalDuration; // reset counter
    bCanPressTurnSignalR = true;
}

void AEgoVehicle::PressTurnSignalL()
{
    if (!bCanPressTurnSignalL)
        return;
    bCanPressTurnSignalL = false; // don't press again until release
    // store in local input container
    VehicleInputs.TurnSignalLeft = true;

    if (bEnableTurnSignalAction)
    {
        // apply new light state
        FVehicleLightState Lights = this->GetVehicleLightState();
        Lights.RightBlinker = false;
        Lights.LeftBlinker = true;
        this->SetVehicleLightState(Lights);
        // play sound
        this->PlayTurnSignalSound();
    }
    RightSignalTimeToDie = 0.f;                         // immediately stop right signal
    LeftSignalTimeToDie = TNumericLimits<float>::Max(); // wait until button released (+inf until then)
}

void AEgoVehicle::ReleaseTurnSignalL()
{
    if (bCanPressTurnSignalL)
        return;
    VehicleInputs.TurnSignalLeft = false;
    LeftSignalTimeToDie = FPlatformTime::Seconds() + this->TurnSignalDuration; // reset counter
    bCanPressTurnSignalL = true;
}

void AEgoVehicle::PressHandbrake()
{
    if (!bCanPressHandbrake)
        return;
    bCanPressHandbrake = false;                             // don't press again until release
    GetVehicleMovementComponent()->SetHandbrakeInput(true); // UE4 control
    // assign to input struct
    VehicleInputs.HoldHandbrake = true;
}

void AEgoVehicle::ReleaseHandbrake()
{
    GetVehicleMovementComponent()->SetHandbrakeInput(false); // UE4 control
    // assign to input struct
    VehicleInputs.HoldHandbrake = false;
    bCanPressHandbrake = true;
}