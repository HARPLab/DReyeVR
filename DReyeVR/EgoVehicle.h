#pragma once

#include "Camera/CameraComponent.h"            // UCameraComponent
#include "Carla/Game/CarlaEpisode.h"           // CarlaEpisode
#include "Carla/Sensor/DReyeVRData.h"          // DReyeVR namespace
#include "Carla/Vehicle/CarlaWheeledVehicle.h" // ACarlaWheeledVehicle
#include "Components/AudioComponent.h"         // UAudioComponent
#include "Components/InputComponent.h"         // InputComponent
#include "Components/SceneComponent.h"         // USceneComponent
#include "CoreMinimal.h"                       // Unreal functions
#include "DReyeVRUtils.h"                      // ReadConfigValue
#include "EgoSensor.h"                         // AEgoSensor
#include "FlatHUD.h"                           // ADReyeVRHUD
#include "ImageUtils.h"                        // CreateTexture2D
#include "LevelScript.h"                       // ADReyeVRLevel
#include "WheeledVehicle.h"                    // VehicleMovementComponent
#include <stdio.h>
#include <vector>

// #define USE_LOGITECH_PLUGIN true // handled in .Build.cs file

#ifndef _WIN32
// can only use LogitechWheel plugin on Windows! :(
#undef USE_LOGITECH_PLUGIN
#define USE_LOGITECH_PLUGIN false
#endif

#if USE_LOGITECH_PLUGIN
#include "LogitechSteeringWheelLib.h" // LogitechWheel plugin for hardware integration & force feedback
#endif

#include "EgoVehicle.generated.h"

class ADReyeVRLevel;

UCLASS()
class CARLAUE4_API AEgoVehicle : public ACarlaWheeledVehicle
{
    GENERATED_BODY()

  public:
    // Sets default values for this pawn's properties
    AEgoVehicle(const FObjectInitializer &ObjectInitializer);

    void ReadConfigVariables();

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent *PlayerInputComponent) override;

    // Setters from external classes
    void SetLevel(ADReyeVRLevel *Level);
    void SetVolume(const float VolumeIn);

    // Getters
    FVector GetCameraOffset() const;
    FVector GetCameraPosn() const;
    FVector GetNextCameraPosn(const float DeltaSeconds) const;
    FRotator GetCameraRot() const;
    const UCameraComponent *GetCamera() const;
    UCameraComponent *GetCamera();
    const DReyeVR::UserInputs &GetVehicleInputs() const;

    // Play sounds
    void PlayGearShiftSound(const float DelayBeforePlay = 0.f) const;
    void PlayTurnSignalSound(const float DelayBeforePlay = 0.f) const;

  protected:
    // Called when the game starts (spawned) or ends (destroyed)
    virtual void BeginPlay() override;
    virtual void BeginDestroy() override;

    // World variables
    class UWorld *World;
    class APlayerController *Player;

  private:
    void Register();   // function to register the AEgoVehicle with Carla's ActorRegistry
    void FinishTick(); // do all the things necessary at the end of a tick

    ////////////////:CAMERA:////////////////
    void ConstructCamera(); // needs to be called in the constructor
    void InitSteamVR();     // Initialize the Head Mounted Display
    UPROPERTY(Category = Camera, EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class USceneComponent *VRCameraRoot;
    UPROPERTY(Category = Camera, EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UCameraComponent *FirstPersonCam;
    FVector CameraLocnInVehicle{21.0f, -40.0f, 120.0f}; // depends on vehicle mesh (units in cm)
    float FieldOfView = 90.f;                           // in degrees

    ////////////////:SENSOR:////////////////
    void ReplayTick();
    void InitSensor();
    class AEgoSensor *EgoSensor; // custom sensor helper that holds logic for extracting useful data
    void UpdateSensor(const float DeltaTime);
    FVector CombinedGaze, CombinedOrigin;
    FVector LeftGaze, LeftOrigin;
    FVector RightGaze, RightOrigin;

    ////////////////:INPUTS:////////////////
    /// NOTE: since there are so many functions here, they are defined in EgoInputs.cpp
    struct DReyeVR::UserInputs VehicleInputs; // struct for user inputs
    // Vehicle control functions
    void SetSteering(const float SteeringInput);
    void SetThrottle(const float ThrottleInput);
    void SetBrake(const float BrakeInput);
    bool bReverse;
    bool isPressRisingEdgeRev = true; // first press is a rising edge
    void ToggleReverse();
    float RightSignalTimeToDie, LeftSignalTimeToDie; // how long the blinkers last
    void TurnSignalLeft();
    void TurnSignalRight();
    void HoldHandbrake();
    void ReleaseHandbrake();
    void MouseLookUp(const float mY_Input);
    void MouseTurn(const float mX_Input);

    // Camera control functions (offset by some amount)
    void CameraPositionAdjust(const FVector &displacement);
    void CameraFwd();
    void CameraBack();
    void CameraLeft();
    void CameraRight();
    void CameraUp();
    void CameraDown();

    // Vehicle parameters
    float ScaleSteeringInput;
    float ScaleThrottleInput;
    float ScaleBrakeInput;
    bool InvertMouseY;
    float ScaleMouseY;
    float ScaleMouseX;

    void InitLogiWheel();
    void TickLogiWheel();
#if USE_LOGITECH_PLUGIN
    DIJOYSTATE2 *Old = nullptr; // global "old" struct for the last state
    void LogLogitechPluginStruct(const DIJOYSTATE2 *Now);
    void LogitechWheelUpdate();      // for logitech wheel integration
    void ApplyForceFeedback() const; // for logitech wheel integration
#endif

    ////////////////:SOUNDS:////////////////
    void ConstructEgoSounds(); // needs to be called in the constructor
    UPROPERTY(Category = "Audio", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UAudioComponent *GearShiftSound; // nice for toggling reverse
    UPROPERTY(Category = "Audio", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UAudioComponent *TurnSignalSound; // good for turn signals

    ////////////////:LEVEL:////////////////
    void TickLevel(float DeltaSeconds);
    class ADReyeVRLevel *DReyeVRLevel;

    ////////////////:SPECTATOR:////////////////
    void InitSpectator();
    void InitReticleTexture();  // initializes the spectator-reticle texture
    void DrawSpectatorScreen(); // called on every tick
    UTexture2D *ReticleTexture; // UE4 texture for eye reticle

    ////////////////:FLATHUD:////////////////
    // (Flat) HUD (NOTE: ONLY FOR NON VR)
    void InitFlatHUD();
    UPROPERTY(Category = HUD, EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class ADReyeVRHUD *FlatHUD;
    void DrawFlatHUD(float DeltaSeconds);
    int ReticleSize = 100;               // diameter of reticle (line thickness is 10% of this)
    bool bDrawFlatHud = true;            // whether to draw the flat hud at all (default true, but false in VR)
    bool bDrawFPSCounter = true;         // draw FPS counter in top left corner
    bool bDrawGaze = false;              // whether or not to draw a line for gaze-ray on HUD
    bool bDrawSpectatorReticle = true;   // Reticle used in the VR-spectator mode
    bool bDrawFlatReticle = true;        // Reticle used in the flat mode (uses HUD) (ONLY in non-vr mode)
    bool bEnableSpectatorScreen = false; // don't spent time rendering the spectator screen
    bool bRectangularReticle = false;    // draw the reticle texture on the HUD & Spectator (NOT RECOMMENDED)

    ////////////////:DASH:////////////////
    // Text Render components (Like the HUD but part of the mesh and works in VR)
    void ConstructDashText(); // needs to be called in the constructor
    UPROPERTY(Category = "Dash", EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UTextRenderComponent *Speedometer;
    UPROPERTY(Category = "Dash", EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UTextRenderComponent *TurnSignals;
    float TurnSignalDuration; // time in seconds
    UPROPERTY(Category = "Dash", EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UTextRenderComponent *GearShifter;
    void UpdateDash();
    FVector DashboardLocnInVehicle{110, 0, 105}; // can change via params
    bool bUseMPH;
    float SpeedometerScale; // scale from CM/s to MPH or KPH depending on bUseMPH

    ////////////////:STEERINGWHEEL:////////////////
    void ConstructSteeringWheel(); // needs to be called in the constructor
    UPROPERTY(Category = Steering, EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UStaticMeshComponent *SteeringWheel;
    void TickSteeringWheel(const float DeltaTime);
    FVector InitWheelLocation;
    FRotator InitWheelRotation;
    float MaxSteerAngleDeg;
    float MaxSteerVelocity;
    float SteeringScale;

    ////////////////:OTHER:////////////////

    // Actor registry
    int EgoVehicleID;
    UCarlaEpisode *Episode = nullptr;

    // Other
    void DebugLines() const;
    bool bIsHMDConnected = false;  // checks for HMD connection on BeginPlay
    bool bIsLogiConnected = false; // check if Logi device is connected (on BeginPlay)
    bool bDrawDebugEditor = false;
};
