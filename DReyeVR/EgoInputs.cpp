#include "EgoVehicle.h"

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
    PlayerInputComponent->BindAxis("Steer_DReyeVR", this, &AEgoVehicle::SetSteering);
    PlayerInputComponent->BindAxis("Throttle_DReyeVR", this, &AEgoVehicle::SetThrottle);
    PlayerInputComponent->BindAxis("Brake_DReyeVR", this, &AEgoVehicle::SetBrake);
    // reverse & handbrake actions
    PlayerInputComponent->BindAction("ToggleReverse_DReyeVR", IE_Pressed, this, &AEgoVehicle::ToggleReverse);
    PlayerInputComponent->BindAction("HoldHandbrake_DReyeVR", IE_Pressed, this, &AEgoVehicle::HoldHandbrake);
    PlayerInputComponent->BindAction("HoldHandbrake_DReyeVR", IE_Released, this, &AEgoVehicle::ReleaseHandbrake);
    PlayerInputComponent->BindAction("TurnSignalRight_DReyeVR", IE_Pressed, this, &AEgoVehicle::TurnSignalRight);
    PlayerInputComponent->BindAction("TurnSignalLeft_DReyeVR", IE_Pressed, this, &AEgoVehicle::TurnSignalLeft);
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

/// NOTE: the CarlaVehicle does not actually move the vehicle, only its state/animations
// to actually move the vehicle we'll use GetVehicleMovementComponent() which is part of AWheeledVehicle
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

void AEgoVehicle::ToggleReverse()
{
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

void AEgoVehicle::TurnSignalRight()
{
    // store in local input container
    VehicleInputs.TurnSignalRight = true;

    // apply new light state
    FVehicleLightState Lights = this->GetVehicleLightState();
    Lights.RightBlinker = true;
    Lights.LeftBlinker = false;
    this->SetVehicleLightState(Lights);

    this->PlayTurnSignalSound();
    RightSignalTimeToDie = FPlatformTime::Seconds() + this->TurnSignalDuration; // reset counter
    LeftSignalTimeToDie = 0.f;                                                  // immediately stop left signal
}

void AEgoVehicle::TurnSignalLeft()
{
    // store in local input container
    VehicleInputs.TurnSignalLeft = true;

    // apply new light state
    FVehicleLightState Lights = this->GetVehicleLightState();
    Lights.RightBlinker = false;
    Lights.LeftBlinker = true;
    this->SetVehicleLightState(Lights);

    this->PlayTurnSignalSound();
    RightSignalTimeToDie = 0.f;                                                // immediately stop right signal
    LeftSignalTimeToDie = FPlatformTime::Seconds() + this->TurnSignalDuration; // reset counter
}

void AEgoVehicle::HoldHandbrake()
{
    this->GetVehicleMovementComponent()->SetHandbrakeInput(true); // UE4 control
    // assign to input struct
    VehicleInputs.HoldHandbrake = true;
}

void AEgoVehicle::ReleaseHandbrake()
{
    this->GetVehicleMovementComponent()->SetHandbrakeInput(false); // UE4 control
    // assign to input struct
    VehicleInputs.HoldHandbrake = false;
}

/// NOTE: in UE4 rotators are of the form: {Pitch, Yaw, Roll} (stored in degrees)
/// We are basing the limits off of "Cervical Spine Functional Anatomy ad the Biomechanics of Injury":
// "The cervical spine's range of motion is approximately 80° to 90° of flexion, 70° of extension,
// 20° to 45° of lateral flexion, and up to 90° of rotation to both sides."
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
    bIsLogiConnected = LogiIsConnected(0); // get status of connected device
    if (!bIsLogiConnected)
    {
        UE_LOG(LogTemp, Log, TEXT("WARNING: Could not find Logitech device connected on input 0"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Found a Logitech device connected on input 0"));
    }
#endif
}

void AEgoVehicle::TickLogiWheel()
{
#if USE_LOGITECH_PLUGIN
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

const std::vector<FString> VarNames = {"rgdwPOV[0]", "rgdwPOV[1]", "rgdwPOV[2]", "rgdwPOV[3]"};
/// NOTE: this is a debug function used to dump all the information we can regarding
// the Logitech wheel hardware we used since the exact buttons were not documented in
// the repo: https://github.com/drb1992/LogitechWheelPlugin
void AEgoVehicle::LogLogitechPluginStruct(const DIJOYSTATE2 *Now)
{
    if (Old == nullptr)
    {
        Old = new struct DIJOYSTATE2;
        (*Old) = (*Now); // assign to the new (current) dijoystate struct
        return;          // initializing the Old struct ptr
    }
    // Getting all (4) values from the current struct
    const std::vector<int> NowVals = {int(Now->rgdwPOV[0]), int(Now->rgdwPOV[1]), int(Now->rgdwPOV[2]),
                                      int(Now->rgdwPOV[3])};
    // Getting the (4) values from the old struct
    const std::vector<int> OldVals = {int(Old->rgdwPOV[0]), int(Old->rgdwPOV[1]), int(Old->rgdwPOV[2]),
                                      int(Old->rgdwPOV[3])};

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
    LogiUpdate(); // update the logitech wheel
    DIJOYSTATE2 *WheelState = LogiGetState(0);
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
    SetSteering(WheelRotation);
    SetThrottle(AccelerationPedal);
    SetBrake(BrakePedal);

    //    UE_LOG(LogTemp, Log, TEXT("Dpad value %f"), Dpad);
    //    if (WheelState->rgdwPOV[0] == 0) // should work now
    if (WheelState->rgbButtons[0] || WheelState->rgbButtons[1] || WheelState->rgbButtons[2] ||
        WheelState->rgbButtons[3]) // replace reverse with face buttons
    {
        if (isPressRisingEdgeRev == true) // only toggle reverse on rising edge of button press
        {
            isPressRisingEdgeRev = false; // not rising edge while the button is pressed
            UE_LOG(LogTemp, Log, TEXT("Reversing: Dpad value %f"), Dpad);
            ToggleReverse();
        }
    }
    else
    {
        isPressRisingEdgeRev = true;
    }
    if (WheelState->rgbButtons[4])
    {
        TurnSignalRight();
    }
    if (WheelState->rgbButtons[5])
    {
        TurnSignalLeft();
    }

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
                                              //    UE_LOG(LogTemp, Log, TEXT("Speed value %f"), Speed);
    const int WheelIndex = 0;                 // first (only) wheel attached
    /// TODO: move outside this function (in tick()) to avoid redundancy
    if (LogiHasForceFeedback(WheelIndex))
    {
        const int OffsetPercentage = 0;      // "Specifies the center of the spring force effect"
        const int SaturationPercentage = 30; // "Level of saturation... comparable to a magnitude"
        const int CoeffPercentage = 100; // "Slope of the effect strength increase relative to deflection from Offset"
        LogiPlaySpringForce(WheelIndex, OffsetPercentage, SaturationPercentage, CoeffPercentage);
    }
    /// NOTE: there are other kinds of forces as described in the LogitechWheelPlugin API:
    // https://github.com/drb1992/LogitechWheelPlugin/blob/master/LogitechWheelPlugin/Source/LogitechWheelPlugin/Private/LogitechBWheelInputDevice.cpp
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