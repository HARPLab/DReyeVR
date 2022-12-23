#pragma once

#include "Camera/CameraComponent.h"                   // UCameraComponent
#include "Carla/Actor/DReyeVRCustomActor.h"           // ADReyeVRCustomActor
#include "Carla/Game/CarlaEpisode.h"                  // CarlaEpisode
#include "Carla/Sensor/DReyeVRData.h"                 // DReyeVR namespace
#include "Carla/Vehicle/CarlaWheeledVehicle.h"        // ACarlaWheeledVehicle
#include "Carla/Vehicle/WheeledVehicleAIController.h" // AWheeledVehicleAIController
#include "Components/AudioComponent.h"                // UAudioComponent
#include "Components/InputComponent.h"                // InputComponent
#include "Components/PlanarReflectionComponent.h"     // Planar Reflection
#include "Components/SceneComponent.h"                // USceneComponent
#include "CoreMinimal.h"                              // Unreal functions
#include "DReyeVRUtils.h"                             // ReadConfigValue
#include "EgoSensor.h"                                // AEgoSensor
#include "FlatHUD.h"                                  // ADReyeVRHUD
#include "ImageUtils.h"                               // CreateTexture2D
#include "LevelScript.h"                              // ADReyeVRLevel
#include "WheeledVehicle.h"                           // VehicleMovementComponent
#include <stdio.h>
#include <vector>

#include "EgoVehicle.generated.h"

class ADReyeVRLevel;
class ADReyeVRPawn;

UCLASS()
class CARLAUE4_API AEgoVehicle : public ACarlaWheeledVehicle
{
    GENERATED_BODY()

    friend class ADReyeVRPawn;

  public:
    // Sets default values for this pawn's properties
    AEgoVehicle(const FObjectInitializer &ObjectInitializer);

    void ReadConfigVariables();

    virtual void Tick(float DeltaTime) override; // called automatically

    // Setters from external classes
    void SetLevel(ADReyeVRLevel *Level);
    void SetPawn(ADReyeVRPawn *Pawn);
    void SetVolume(const float VolumeIn);

    // Getters
    FVector GetCameraOffset() const;
    FVector GetCameraPosn() const;
    FVector GetNextCameraPosn(const float DeltaSeconds) const;
    FRotator GetCameraRot() const;
    const UCameraComponent *GetCamera() const;
    UCameraComponent *GetCamera();
    const DReyeVR::UserInputs &GetVehicleInputs() const;
    const class AEgoSensor *GetSensor() const;

    // autopilot API
    void SetAutopilot(const bool AutopilotOn);
    bool GetAutopilotStatus() const;
    /// TODO: add custom routes for autopilot

    // Play sounds
    void PlayGearShiftSound(const float DelayBeforePlay = 0.f) const;
    void PlayTurnSignalSound(const float DelayBeforePlay = 0.f) const;

    // Camera view
    size_t GetNumCameraPoses() const;                // how many diff poses?
    void SetCameraRootPose(const FTransform &Pose);  // give arbitrary FTransform
    void SetCameraRootPose(const FString &PoseName); // index into named FTransform
    void SetCameraRootPose(size_t PoseIdx);          // index into ordered FTransform
    const FTransform &GetCameraRootPose() const;
    void NextCameraView();
    void PrevCameraView();

  protected:
    // Called when the game starts (spawned) or ends (destroyed)
    virtual void BeginPlay() override;
    virtual void BeginDestroy() override;

    // World variables
    class UWorld *World;

  private:
    void Register(); // function to register the AEgoVehicle with Carla's ActorRegistry

    ////////////////:CAMERA:////////////////
    void ConstructCameraRoot(); // needs to be called in the constructor
    UPROPERTY(Category = Camera, EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class USceneComponent *VRCameraRoot;
    UPROPERTY(Category = Camera, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UCameraComponent *FirstPersonCam;
    FTransform CameraPose, CameraPoseOffset;                      // camera pose (location & rotation) and manual offset
    std::vector<std::pair<FString, FTransform>> CameraTransforms; // collection of named transforms from params
    size_t CurrentCameraTransformIdx = 0;
    bool bCameraFollowHMD = true; // disable this (in params) to replay without following the player's HMD (replay-only)

    ////////////////:SENSOR:////////////////
    void ReplayTick();
    void InitSensor();
    class AEgoSensor *EgoSensor; // custom sensor helper that holds logic for extracting useful data
    void UpdateSensor(const float DeltaTime);
    FVector CombinedGaze, CombinedOrigin;
    FVector LeftGaze, LeftOrigin;
    FVector RightGaze, RightOrigin;

    ///////////////:DREYEVRPAWN://///////////
    class ADReyeVRPawn *Pawn = nullptr;

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

    ////////////////:AICONTROLLER:////////////////
    class AWheeledVehicleAIController *AI_Player = nullptr;
    void InitAIPlayer();
    bool bAutopilotEnabled = false;
    void TickAutopilot();

    ////////////////:INPUTS:////////////////
    /// NOTE: since there are so many functions here, they are defined in EgoInputs.cpp
    struct DReyeVR::UserInputs VehicleInputs; // struct for user inputs
    // Vehicle control functions
    void SetSteering(const float SteeringInput);
    void SetThrottle(const float ThrottleInput);
    void SetBrake(const float BrakeInput);
    bool bReverse;

    // "button presses" should have both a "Press" and "Release" function
    // And, if using the logitech plugin, should also have an "is rising edge" bool so they can only
    // be pressed after being released (cant double press w/ no release)
    // Reverse toggle
    void PressReverse();
    void ReleaseReverse();
    bool bCanPressReverse = true;
    // turn signals
    bool bEnableTurnSignalAction = true; // tune with "EnableTurnSignalAction" in config
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

    // Camera control functions (offset by some amount)
    void CameraPositionAdjust(const FVector &Disp);
    void CameraFwd();
    void CameraBack();
    void CameraLeft();
    void CameraRight();
    void CameraUp();
    void CameraDown();

    // changing camera views
    void PressNextCameraView();
    void ReleaseNextCameraView();
    bool bCanPressNextCameraView = true;
    void PressPrevCameraView();
    void ReleasePrevCameraView();
    bool bCanPressPrevCameraView = true;

    // Vehicle parameters
    float ScaleSteeringInput;
    float ScaleThrottleInput;
    float ScaleBrakeInput;

    ////////////////:SOUNDS:////////////////
    void ConstructEgoSounds(); // needs to be called in the constructor
    UPROPERTY(Category = "Audio", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UAudioComponent *GearShiftSound; // nice for toggling reverse
    UPROPERTY(Category = "Audio", EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    class UAudioComponent *TurnSignalSound; // good for turn signals

    ////////////////:LEVEL:////////////////
    void TickLevel(float DeltaSeconds);
    class ADReyeVRLevel *DReyeVRLevel;

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
    bool bDrawDebugEditor = false;
};
