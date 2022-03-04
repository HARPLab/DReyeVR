#include "EgoVehicle.h"
#include "HeadMountedDisplayFunctionLibrary.h" // SetTrackingOrigin, GetWorldToMetersScale
#include "HeadMountedDisplayTypes.h"           // EOrientPositionSelector
#include "Math/NumericLimits.h"                // TNumericLimits<float>::Max
#include <string>                              // std::string, std::wstring

////////////////:INPUTS:////////////////
/// NOTE: Here we define all the Input functions for the EgoVehicle just to keep them
// from cluttering the EgoVehcile.cpp file

const DReyeVR::UserInputs &AEgoVehicle::GetVehicleInputs() const
{
    return VehicleInputs;
}

// Called to bind functionality to input
void AEgoVehicle::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent)
{
    /// NOTE: to see all DReyeVR inputs see DefaultInput.ini
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    /// NOTE: an Action is a digital input, an Axis is an analog input
    // steering and throttle analog inputs (axes)
    PlayerInputComponent->BindAxis("Steer_DReyeVR", this, &AEgoVehicle::SetSteeringKbd);
    PlayerInputComponent->BindAxis("Throttle_DReyeVR", this, &AEgoVehicle::SetThrottleKbd);
    PlayerInputComponent->BindAxis("Brake_DReyeVR", this, &AEgoVehicle::SetBrakeKbd);
    // button actions (press & release)
    PlayerInputComponent->BindAction("ToggleReverse_DReyeVR", IE_Pressed, this, &AEgoVehicle::PressReverse);
    PlayerInputComponent->BindAction("TurnSignalRight_DReyeVR", IE_Pressed, this, &AEgoVehicle::PressTurnSignalR);
    PlayerInputComponent->BindAction("TurnSignalLeft_DReyeVR", IE_Pressed, this, &AEgoVehicle::PressTurnSignalL);
    PlayerInputComponent->BindAction("ToggleReverse_DReyeVR", IE_Released, this, &AEgoVehicle::ReleaseReverse);
    PlayerInputComponent->BindAction("TurnSignalRight_DReyeVR", IE_Released, this, &AEgoVehicle::ReleaseTurnSignalR);
    PlayerInputComponent->BindAction("TurnSignalLeft_DReyeVR", IE_Released, this, &AEgoVehicle::ReleaseTurnSignalL);
    PlayerInputComponent->BindAction("HoldHandbrake_DReyeVR", IE_Pressed, this, &AEgoVehicle::HoldHandbrake);
    PlayerInputComponent->BindAction("HoldHandbrake_DReyeVR", IE_Released, this, &AEgoVehicle::ReleaseHandbrake);
    PlayerInputComponent->BindAction("ResetCamera_DReyeVR", IE_Pressed, this, &AEgoVehicle::PressResetCamera);
    PlayerInputComponent->BindAction("ResetCamera_DReyeVR", IE_Released, this, &AEgoVehicle::ReleaseResetCamera);
    /// Mouse X and Y input for looking up and turning
    PlayerInputComponent->BindAxis("MouseLookUp_DReyeVR", this, &AEgoVehicle::MouseLookUp);
    PlayerInputComponent->BindAxis("MouseTurn_DReyeVR", this, &AEgoVehicle::MouseTurn);
    // Camera position adjustments
    PlayerInputComponent->BindAction("CameraFwd_DReyeVR", IE_Pressed, this, &AEgoVehicle::CameraFwd);
    PlayerInputComponent->BindAction("CameraBack_DReyeVR", IE_Pressed, this, &AEgoVehicle::CameraBack);
    PlayerInputComponent->BindAction("CameraLeft_DReyeVR", IE_Pressed, this, &AEgoVehicle::CameraLeft);
    PlayerInputComponent->BindAction("CameraRight_DReyeVR", IE_Pressed, this, &AEgoVehicle::CameraRight);
    PlayerInputComponent->BindAction("CameraUp_DReyeVR", IE_Pressed, this, &AEgoVehicle::CameraUp);
    PlayerInputComponent->BindAction("CameraDown_DReyeVR", IE_Pressed, this, &AEgoVehicle::CameraDown);
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

void AEgoVehicle::PressResetCamera()
{
    if (!bCanResetCamera)
        return;
    bCanResetCamera = false;
    ResetCamera();
}

void AEgoVehicle::ReleaseResetCamera()
{
    bCanResetCamera = true;
}

void AEgoVehicle::ResetCamera()
{
    // First, set the root of the camera to the driver's seat head pos
    VRCameraRoot->SetRelativeLocation(CameraLocnInVehicle);
    // Then set the actual camera to be at its origin (attached to VRCameraRoot)
    FirstPersonCam->SetRelativeLocation(FVector::ZeroVector);
    FirstPersonCam->SetRelativeRotation(FRotator::ZeroRotator);
    if (bIsHMDConnected)
    {
        UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition(
            0, EOrientPositionSelector::OrientationAndPosition);
        // reload world
        UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()), false);
    }
}

void AEgoVehicle::SetSteeringKbd(const float SteeringInput)
{
    if (SteeringInput == 0.f && bIsLogiConnected)
        return;
    SetSteering(SteeringInput);
}

void AEgoVehicle::SetSteering(const float SteeringInput)
{
    float ScaledSteeringInput = this->ScaleSteeringInput * SteeringInput;
    this->GetVehicleMovementComponent()->SetSteeringInput(ScaledSteeringInput); // UE4 control
    // assign to input struct
    /// TODO: ensure this doesn't conflict with LogitechWheel when both are connected
    VehicleInputs.Steering = ScaledSteeringInput;
}

void AEgoVehicle::SetThrottleKbd(const float ThrottleInput)
{
    if (ThrottleInput == 0.f && bIsLogiConnected)
        return;
    SetThrottle(ThrottleInput);
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
    /// TODO: ensure this doesn't conflict with LogitechWheel when both are connected
    VehicleInputs.Throttle = ScaledThrottleInput;
}

void AEgoVehicle::SetBrakeKbd(const float BrakeInput)
{
    if (BrakeInput == 0.f && bIsLogiConnected)
        return;
    SetBrake(BrakeInput);
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
    /// TODO: ensure this doesn't conflict with LogitechWheel when both are connected
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

    // apply new light state
    FVehicleLightState Lights = this->GetVehicleLightState();
    Lights.RightBlinker = true;
    Lights.LeftBlinker = false;
    this->SetVehicleLightState(Lights);

    this->PlayTurnSignalSound();
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

    // apply new light state
    FVehicleLightState Lights = this->GetVehicleLightState();
    Lights.RightBlinker = false;
    Lights.LeftBlinker = true;
    this->SetVehicleLightState(Lights);

    this->PlayTurnSignalSound();
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

/// NOTE: in UE4 rotators are of the form: {Pitch, Yaw, Roll} (stored in degrees)
/// We are basing the limits off of "Cervical Spine Functional Anatomy ad the Biomechanics of Injury":
// "The cervical spine's range of motion is approximately 80 deg to 90 deg of flexion, 70 deg of extension,
// 20 deg to 45 deg of lateral flexion, and up to 90 deg of rotation to both sides."
// (www.ncbi.nlm.nih.gov/pmc/articles/PMC1250253/)
/// NOTE: flexion = looking down to chest, extension = looking up , lateral = roll
/// ALSO: These functions are only used in non-VR mode, in VR you can move freely

void AEgoVehicle::MouseLookUp(const float mY_Input)
{
    if (mY_Input != 0.f)
    {
        const float ScaleY = (this->InvertMouseY ? 1 : -1) * this->ScaleMouseY; // negative Y is "normal" controls
        FRotator UpDir = this->GetCamera()->GetRelativeRotation() + FRotator(ScaleY * mY_Input, 0.f, 0.f);
        // get the limits of a human neck (only clamping pitch)
        const float MinFlexion = -85.f;
        const float MaxExtension = 70.f;
        UpDir.Pitch = FMath::Clamp(UpDir.Pitch, MinFlexion, MaxExtension);
        this->GetCamera()->SetRelativeRotation(UpDir);
    }
}

void AEgoVehicle::MouseTurn(const float mX_Input)
{
    if (mX_Input != 0.f)
    {
        const float ScaleX = this->ScaleMouseX;
        FRotator CurrentDir = this->GetCamera()->GetRelativeRotation();
        FRotator TurnDir = CurrentDir + FRotator(0.f, ScaleX * mX_Input, 0.f);
        // get the limits of a human neck (only clamping pitch)
        const float MinLeft = -90.f;
        const float MaxRight = 90.f; // may consider increasing to allow users to look through the back window
        TurnDir.Yaw = FMath::Clamp(TurnDir.Yaw, MinLeft, MaxRight);
        this->GetCamera()->SetRelativeRotation(TurnDir);
    }
}

void AEgoVehicle::InitLogiWheel()
{
#if USE_LOGITECH_PLUGIN
    LogiSteeringInitialize(false);
    bIsLogiConnected = LogiIsConnected(WheelDeviceIdx); // get status of connected device
    if (bIsLogiConnected)
    {
        const size_t n = 1000; // name shouldn't be more than 1000 chars right?
        wchar_t *NameBuffer = (wchar_t *)malloc(n * sizeof(wchar_t));
        if (LogiGetFriendlyProductName(WheelDeviceIdx, NameBuffer, n) == false)
        {
            UE_LOG(LogTemp, Warning, TEXT("Unable to get Logi friendly name!"));
            NameBuffer = L"Unknown";
        }
        std::wstring wNameStr(NameBuffer, n);
        std::string NameStr(wNameStr.begin(), wNameStr.end());
        FString LogiName(NameStr.c_str());
        UE_LOG(LogTemp, Log, TEXT("Found a Logitech device (%s) connected on input %d"), *LogiName, WheelDeviceIdx);
        free(NameBuffer); // no longer needed
    }
    else
    {
        const FString LogiError = "Could not find Logitech device connected on input 0";
        const bool PrintToLog = false;
        const bool PrintToScreen = true;
        const float ScreenDurationSec = 20.f;
        const FLinearColor MsgColour = FLinearColor(1, 0, 0, 1); // RED
        UKismetSystemLibrary::PrintString(World, LogiError, PrintToScreen, PrintToLog, MsgColour, ScreenDurationSec);
        UE_LOG(LogTemp, Error, TEXT("%s"), *LogiError); // Error is RED
    }
#endif
}

void AEgoVehicle::DestroyLogiWheel(bool DestroyModule)
{
#if USE_LOGITECH_PLUGIN
    if (bIsLogiConnected)
    {
        // stop any forces on the wheel (we only use spring force feedback)
        LogiStopSpringForce(WheelDeviceIdx);

        if (DestroyModule) // only destroy the module at the end of the game (not ego life)
        {
            // shutdown the entire module (dangerous bc lingering pointers)
            LogiSteeringShutdown();
        }
    }
#endif
}

void AEgoVehicle::TickLogiWheel()
{
#if USE_LOGITECH_PLUGIN
    bIsLogiConnected = LogiIsConnected(WheelDeviceIdx); // get status of connected device
    if (bIsLogiConnected)
    {
        // Taking logitech inputs for steering
        LogitechWheelUpdate();

        // Add Force Feedback to the hardware steering wheel when a LogitechWheel is used
        ApplyForceFeedback();
    }
#endif
}

#if USE_LOGITECH_PLUGIN

// const std::vector<FString> VarNames = {"rgdwPOV[0]", "rgdwPOV[1]", "rgdwPOV[2]", "rgdwPOV[3]"};
const std::vector<FString> VarNames = {                                                    // 34 values
    "lX",           "lY",           "lZ",         "lRz",           "lRy",           "lRz", // variable names
    "rglSlider[0]", "rglSlider[1]", "rgdwPOV[0]", "rgbButtons[0]", "lVX",           "lVY",           "lVZ",
    "lVRx",         "lVRy",         "lVRz",       "rglVSlider[0]", "rglVSlider[1]", "lAX",           "lAY",
    "lAZ",          "lARx",         "lARy",       "lARz",          "rglASlider[0]", "rglASlider[1]", "lFX",
    "lFY",          "lFZ",          "lFRx",       "lFRy",          "lFRz",          "rglFSlider[0]", "rglFSlider[1]"};
/// NOTE: this is a debug function used to dump all the information we can regarding
// the Logitech wheel hardware we used since the exact buttons were not documented in
// the repo: https://github.com/HARPLab/LogitechWheelPlugin
void AEgoVehicle::LogLogitechPluginStruct(const struct DIJOYSTATE2 *Now)
{
    if (Old == nullptr)
    {
        Old = new struct DIJOYSTATE2;
        (*Old) = (*Now); // assign to the new (current) dijoystate struct
        return;          // initializing the Old struct ptr
    }
    const std::vector<int> NowVals = {
        Now->lX, Now->lY, Now->lZ, Now->lRx, Now->lRy, Now->lRz, Now->rglSlider[0], Now->rglSlider[1],
        // Converting unsigned int & unsigned char to int
        int(Now->rgdwPOV[0]), int(Now->rgbButtons[0]), Now->lVX, Now->lVY, Now->lVZ, Now->lVRx, Now->lVRy, Now->lVRz,
        Now->rglVSlider[0], Now->rglVSlider[1], Now->lAX, Now->lAY, Now->lAZ, Now->lARx, Now->lARy, Now->lARz,
        Now->rglASlider[0], Now->rglASlider[1], Now->lFX, Now->lFY, Now->lFZ, Now->lFRx, Now->lFRy, Now->lFRz,
        Now->rglFSlider[0], Now->rglFSlider[1]}; // 32 elements
    // Getting the (34) values from the old struct
    const std::vector<int> OldVals = {
        Old->lX, Old->lY, Old->lZ, Old->lRx, Old->lRy, Old->lRz, Old->rglSlider[0], Old->rglSlider[1],
        // Converting unsigned int & unsigned char to int
        int(Old->rgdwPOV[0]), int(Old->rgbButtons[0]), Old->lVX, Old->lVY, Old->lVZ, Old->lVRx, Old->lVRy, Old->lVRz,
        Old->rglVSlider[0], Old->rglVSlider[1], Old->lAX, Old->lAY, Old->lAZ, Old->lARx, Old->lARy, Old->lARz,
        Old->rglASlider[0], Old->rglASlider[1], Old->lFX, Old->lFY, Old->lFZ, Old->lFRx, Old->lFRy, Old->lFRz,
        Old->rglFSlider[0], Old->rglFSlider[1]};

    check(NowVals.size() == OldVals.size() && NowVals.size() == VarNames.size());

    // print any differences
    bool isDiff = false;
    for (size_t i = 0; i < NowVals.size(); i++)
    {
        if (NowVals[i] != OldVals[i])
        {
            if (!isDiff) // only gets triggered at MOST once
            {
                UE_LOG(LogTemp, Log, TEXT("Logging joystick at t=%.3f"), UGameplayStatics::GetRealTimeSeconds(World));
                isDiff = true;
            }
            UE_LOG(LogTemp, Log, TEXT("Triggered \"%s\" from %d to %d"), *(VarNames[i]), OldVals[i], NowVals[i]);
        }
    }

    // also check the 128 rgbButtons array
    for (size_t i = 0; i < 127; i++)
    {
        if (Old->rgbButtons[i] != Now->rgbButtons[i])
        {
            if (!isDiff) // only gets triggered at MOST once
            {
                UE_LOG(LogTemp, Log, TEXT("Logging joystick at t=%.3f"), UGameplayStatics::GetRealTimeSeconds(World));
                isDiff = true;
            }
            UE_LOG(LogTemp, Log, TEXT("Triggered \"rgbButtons[%d]\" from %d to %d"), int(i), int(OldVals[i]),
                   int(NowVals[i]));
        }
    }

    // assign the current joystate into the old one
    (*Old) = (*Now);
}

void AEgoVehicle::LogitechWheelUpdate()
{
    // only execute this in Windows, the Logitech plugin is incompatible with Linux
    if (LogiUpdate() == false) // update the logitech wheel
        UE_LOG(LogTemp, Warning, TEXT("Logitech wheel %d failed to update!"), WheelDeviceIdx);
    DIJOYSTATE2 *WheelState = LogiGetState(WheelDeviceIdx);
    if (bLogLogitechWheel)
        LogLogitechPluginStruct(WheelState);
    /// NOTE: obtained these from LogitechWheelInputDevice.cpp:~111
    // -32768 to 32767. -32768 = all the way to the left. 32767 = all the way to the right.
    const float WheelRotation = FMath::Clamp(float(WheelState->lX), -32767.0f, 32767.0f) / 32767.0f; // (-1, 1)
    // -32768 to 32767. 32767 = pedal not pressed. -32768 = pedal fully pressed.
    const float AccelerationPedal = fabs(((WheelState->lY - 32767.0f) / (65535.0f))); // (0, 1)
    // -32768 to 32767. Higher value = less pressure on brake pedal
    const float BrakePedal = fabs(((WheelState->lRz - 32767.0f) / (65535.0f))); // (0, 1)
    // -1 = not pressed. 0 = Top. 0.25 = Right. 0.5 = Bottom. 0.75 = Left.
    const float Dpad = fabs(((WheelState->rgdwPOV[0] - 32767.0f) / (65535.0f)));
    // apply to DReyeVR inputs
    /// (NOTE: these function calls occur in the EgoVehicle::Tick, meaning other
    // control calls could theoretically conflict/override them if called later. This is
    // the case with the keyboard inputs which is why there are wrappers (suffixed with "Kbd")
    // that always override logi inputs IF their values are nonzero
    this->SetSteering(WheelRotation);
    this->SetThrottle(AccelerationPedal);
    this->SetBrake(BrakePedal);

    //    UE_LOG(LogTemp, Log, TEXT("Dpad value %f"), Dpad);
    //    if (WheelState->rgdwPOV[0] == 0) // should work now

    // Button presses (turn signals, reverse)
    if (WheelState->rgbButtons[0] || WheelState->rgbButtons[1] || // Any of the 4 face pads
        WheelState->rgbButtons[2] || WheelState->rgbButtons[3])
        PressReverse();
    else
        ReleaseReverse();

    if (WheelState->rgbButtons[4])
        PressTurnSignalR();
    else
        ReleaseTurnSignalR();

    if (WheelState->rgbButtons[5])
        PressTurnSignalL();
    else
        ReleaseTurnSignalL();

    if (WheelState->rgbButtons[23]) // big red button on right side of g923
        PressResetCamera();
    else
        ReleaseResetCamera();

    // VRCamerRoot base position adjustment
    if (WheelState->rgdwPOV[0] == 0) // positive in X
        CameraPositionAdjust(FVector(1.f, 0.f, 0.f));
    else if (WheelState->rgdwPOV[0] == 18000) // negative in X
        CameraPositionAdjust(FVector(-1.f, 0.f, 0.f));
    else if (WheelState->rgdwPOV[0] == 9000) // positive in Y
        CameraPositionAdjust(FVector(0.f, 1.f, 0.f));
    else if (WheelState->rgdwPOV[0] == 27000) // negative in Y
        CameraPositionAdjust(FVector(0.f, -1.f, 0.f));
    // VRCamerRoot base height adjustment
    else if (WheelState->rgbButtons[19]) // positive in Z
        CameraPositionAdjust(FVector(0.f, 0.f, 1.f));
    else if (WheelState->rgbButtons[20]) // negative in Z
        CameraPositionAdjust(FVector(0.f, 0.f, -1.f));
}

void AEgoVehicle::ApplyForceFeedback() const
{
    // only execute this in Windows, the Logitech plugin is incompatible with Linux
    const float Speed = GetVelocity().Size(); // get magnitude of self (AActor's) velocity
    /// TODO: move outside this function (in tick()) to avoid redundancy
    if (bIsLogiConnected && LogiHasForceFeedback(WheelDeviceIdx))
    {
        const int OffsetPercentage = 0;      // "Specifies the center of the spring force effect"
        const int SaturationPercentage = 30; // "Level of saturation... comparable to a magnitude"
        const int CoeffPercentage = 100; // "Slope of the effect strength increase relative to deflection from Offset"
        LogiPlaySpringForce(WheelDeviceIdx, OffsetPercentage, SaturationPercentage, CoeffPercentage);
    }
    else
    {
        LogiStopSpringForce(WheelDeviceIdx);
    }
    /// NOTE: there are other kinds of forces as described in the LogitechWheelPlugin API:
    // https://github.com/HARPLab/LogitechWheelPlugin/blob/master/LogitechWheelPlugin/Source/LogitechWheelPlugin/Private/LogitechBWheelInputDevice.cpp
    // For example:
    /*
        Force Types
        0 = Spring				5 = Dirt Road
        1 = Constant			6 = Bumpy Road
        2 = Damper				7 = Slippery Road
        3 = Side Collision		8 = Surface Effect
        4 = Frontal Collision	9 = Car Airborne
    */
}
#endif