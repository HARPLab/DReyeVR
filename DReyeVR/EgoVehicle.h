#pragma once

#include "Camera/CameraComponent.h"               // UCameraComponent
#include "Carla/Game/CarlaEpisode.h"              // CarlaEpisode
#include "Carla/Sensor/DReyeVRData.h"             // DReyeVR namespace
#include "Carla/Vehicle/CarlaWheeledVehicle.h"    // ACarlaWheeledVehicle
#include "Components/AudioComponent.h"            // UAudioComponent
#include "Components/InputComponent.h"            // InputComponent
#include "Components/PlanarReflectionComponent.h" // Planar Reflection
#include "Components/SceneComponent.h"            // USceneComponent
#include "CoreMinimal.h"                          // Unreal functions
#include "DReyeVRUtils.h"                         // ReadConfigValue
#include "EgoSensor.h"                            // AEgoSensor
#include "FlatHUD.h"                              // ADReyeVRHUD
#include "ImageUtils.h"                           // CreateTexture2D
#include "LevelScript.h"                          // ADReyeVRLevel
#include "WheeledVehicle.h"                       // VehicleMovementComponent
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
    FVector2D ProjectGazeToScreen(const FVector &Origin, const FVector &Dir, bool bPlayerViewportRelative = true) const;

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
    void Register(); // function to register the AEgoVehicle with Carla's ActorRegistry

    ////////////////:CAMERA:////////////////
    void ConstructCamera(); // needs to be called in the constructor
    void InitSteamVR();     // Initialize the Head Mounted Display
    UPROPERTY(Category = Camera, EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class USceneComponent *VRCameraRoot;
    UPROPERTY(Category = Camera, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
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

    ////////////////:MIRRORS:////////////////
    void ConstructMirrors();
    struct MirrorParams
    {
        bool Enabled;
        FVector MirrorPos, MirrorScale, ReflectionPos, ReflectionScale;
        FRotator MirrorRot, ReflectionRot;
        float ScreenPercentage;
        FString Name;
        void Initialize(class UStaticMeshComponent *SM, class UPlanarReflectionComponent *Reflection,
                        class USkeletalMeshComponent *VehicleMesh);
    };
    struct MirrorParams RearMirrorParams, LeftMirrorParams, RightMirrorParams;
    UPROPERTY(Category = Mirrors, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UStaticMeshComponent *RightMirrorSM;
    UPROPERTY(Category = Mirrors, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UPlanarReflectionComponent *RightReflection;
    UPROPERTY(Category = Mirrors, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UStaticMeshComponent *LeftMirrorSM;
    UPROPERTY(Category = Mirrors, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UPlanarReflectionComponent *LeftReflection;
    UPROPERTY(Category = Mirrors, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UStaticMeshComponent *RearMirrorSM;
    UPROPERTY(Category = Mirrors, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UPlanarReflectionComponent *RearReflection;
    // rear mirror chassis (dynamic)
    UPROPERTY(Category = Mirrors, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UStaticMeshComponent *RearMirrorChassisSM;
    FVector RearMirrorChassisPos, RearMirrorChassisScale;
    FRotator RearMirrorChassisRot;

    ////////////////:INPUTS:////////////////
    /// NOTE: since there are so many functions here, they are defined in EgoInputs.cpp
    struct DReyeVR::UserInputs VehicleInputs; // struct for user inputs
    // Vehicle control functions
    void SetSteering(const float SteeringInput);
    void SetThrottle(const float ThrottleInput);
    void SetBrake(const float BrakeInput);
    // keyboard mechanisms to access Axis vehicle control (steering, throttle, brake)
    void SetSteeringKbd(const float SteeringInput);
    void SetThrottleKbd(const float ThrottleInput);
    void SetBrakeKbd(const float BrakeInput);
    bool bReverse;
    // "button presses" should have both a "Press" and "Release" function
    // And, if using the logitech plugin, should also have an "is rising edge" bool so they can only
    // be pressed after being released (cant double press w/ no release)
    // Reverse toggle
    void PressReverse();
    void ReleaseReverse();
    bool bCanPressReverse = true;
    // left turn signal
    void PressTurnSignalL();
    void ReleaseTurnSignalL();
    float LeftSignalTimeToDie; // how long until the blinkers go out
    bool bCanPressTurnSignalL = true;
    // right turn signal
    void PressTurnSignalR();
    void ReleaseTurnSignalR();
    float RightSignalTimeToDie; // how long until the blinkers go out
    bool bCanPressTurnSignalR = true;
    // handbrake
    void PressHandbrake();
    void ReleaseHandbrake();
    bool bCanPressHandbrake = true;
    // mouse controls
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

    void PressResetCamera();
    void ReleaseResetCamera();
    void ResetCamera();
    bool bCanResetCamera = true;

    // Vehicle parameters
    float ScaleSteeringInput;
    float ScaleThrottleInput;
    float ScaleBrakeInput;
    bool InvertMouseY;
    float ScaleMouseY;
    float ScaleMouseX;

    void InitLogiWheel();
    void TickLogiWheel();
    void DestroyLogiWheel(bool DestroyModule);
    bool bLogLogitechWheel = false;
    int WheelDeviceIdx = 0; // usually leaving as 0 is fine, only use 1 if 0 is taken
#if USE_LOGITECH_PLUGIN
    struct DIJOYSTATE2 *Old = nullptr; // global "old" struct for the last state
    void LogLogitechPluginStruct(const struct DIJOYSTATE2 *Now);
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
    float HUDScaleVR;           // How much to scale the HUD in VR

    ////////////////:FLATHUD:////////////////
    // (Flat) HUD (NOTE: ONLY FOR NON VR)
    void InitFlatHUD();
    UPROPERTY(Category = HUD, EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class ADReyeVRHUD *FlatHUD;
    void DrawFlatHUD(float DeltaSeconds);
    FVector2D ReticlePos;                // 2D reticle position from eye gaze
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
    float SteeringAnimScale;

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
