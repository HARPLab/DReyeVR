#include "EgoVehicle.h"
#include "Carla/Actor/ActorAttribute.h"             // FActorAttribute
#include "Carla/Actor/ActorRegistry.h"              // Register
#include "Carla/Game/CarlaStatics.h"                // GetEpisode
#include "Carla/Vehicle/CarlaWheeledVehicleState.h" // ECarlaWheeledVehicleState
#include "DReyeVRPawn.h"                            // ADReyeVRPawn
#include "DrawDebugHelpers.h"                       // Debug Line/Sphere
#include "Engine/EngineTypes.h"                     // EBlendMode
#include "Engine/World.h"                           // GetWorld
#include "GameFramework/Actor.h"                    // Destroy
#include "Kismet/KismetSystemLibrary.h"             // PrintString, QuitGame
#include "Math/Rotator.h"                           // RotateVector, Clamp
#include "Math/UnrealMathUtility.h"                 // Clamp

#include <algorithm>

// Sets default values
AEgoVehicle::AEgoVehicle(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
    ReadConfigVariables();

    // this actor ticks AFTER the physics simulation is done
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;

    // Set up the root position to be the this mesh
    SetRootComponent(GetMesh());

    // Initialize the camera components
    ConstructCameraRoot();

    // Initialize audio components
    ConstructEgoSounds();

    // Initialize mirrors
    ConstructMirrors();

    // Initialize text render components
    ConstructDashText();

    // Initialize the steering wheel
    ConstructSteeringWheel();
}

void AEgoVehicle::ReadConfigVariables()
{
    ReadConfigValue("EgoVehicle", "DashLocation", DashboardLocnInVehicle);
    ReadConfigValue("EgoVehicle", "SpeedometerInMPH", bUseMPH);
    ReadConfigValue("EgoVehicle", "EnableTurnSignalAction", bEnableTurnSignalAction);
    ReadConfigValue("EgoVehicle", "TurnSignalDuration", TurnSignalDuration);
    // mirrors
    auto InitMirrorParams = [](const FString &Name, struct MirrorParams &Params) {
        Params.Name = Name;
        ReadConfigValue("Mirrors", Params.Name + "MirrorEnabled", Params.Enabled);
        ReadConfigValue("Mirrors", Params.Name + "MirrorPos", Params.MirrorPos);
        ReadConfigValue("Mirrors", Params.Name + "MirrorRot", Params.MirrorRot);
        ReadConfigValue("Mirrors", Params.Name + "MirrorScale", Params.MirrorScale);
        ReadConfigValue("Mirrors", Params.Name + "ReflectionPos", Params.ReflectionPos);
        ReadConfigValue("Mirrors", Params.Name + "ReflectionRot", Params.ReflectionRot);
        ReadConfigValue("Mirrors", Params.Name + "ReflectionScale", Params.ReflectionScale);
        ReadConfigValue("Mirrors", Params.Name + "ScreenPercentage", Params.ScreenPercentage);
    };
    InitMirrorParams("Rear", RearMirrorParams);
    InitMirrorParams("Left", LeftMirrorParams);
    InitMirrorParams("Right", RightMirrorParams);
    // rear mirror chassis
    ReadConfigValue("Mirrors", "RearMirrorChassisPos", RearMirrorChassisPos);
    ReadConfigValue("Mirrors", "RearMirrorChassisRot", RearMirrorChassisRot);
    ReadConfigValue("Mirrors", "RearMirrorChassisScale", RearMirrorChassisScale);
    // steering wheel
    ReadConfigValue("SteeringWheel", "InitLocation", InitWheelLocation);
    ReadConfigValue("SteeringWheel", "InitRotation", InitWheelRotation);
    ReadConfigValue("SteeringWheel", "MaxSteerAngleDeg", MaxSteerAngleDeg);
    ReadConfigValue("SteeringWheel", "MaxSteerVelocity", MaxSteerVelocity);
    ReadConfigValue("SteeringWheel", "SteeringScale", SteeringAnimScale);
    // other/cosmetic
    ReadConfigValue("EgoVehicle", "ActorRegistryID", EgoVehicleID);
    ReadConfigValue("EgoVehicle", "DrawDebugEditor", bDrawDebugEditor);
    // inputs
    ReadConfigValue("VehicleInputs", "ScaleSteeringDamping", ScaleSteeringInput);
    ReadConfigValue("VehicleInputs", "ScaleThrottleInput", ScaleThrottleInput);
    ReadConfigValue("VehicleInputs", "ScaleBrakeInput", ScaleBrakeInput);
    // replay
    ReadConfigValue("Replayer", "CameraFollowHMD", bCameraFollowHMD);
}

void AEgoVehicle::BeginPlay()
{
    // Called when the game starts or when spawned
    Super::BeginPlay();

    // Get information about the world
    World = GetWorld();
    Episode = UCarlaStatics::GetCurrentEpisode(World);

    // Spawn and attach the EgoSensor
    InitSensor();

    // initialize
    InitAIPlayer();

    // Bug-workaround for initial delay on throttle; see https://github.com/carla-simulator/carla/issues/1640
    this->GetVehicleMovementComponent()->SetTargetGear(1, true);

    // Register Ego Vehicle with ActorRegistry
    Register();

    UE_LOG(LogTemp, Log, TEXT("Initialized DReyeVR EgoVehicle"));
}

void AEgoVehicle::BeginDestroy()
{
    Super::BeginDestroy();

    // destroy all spawned entities
    if (EgoSensor)
        EgoSensor->Destroy();
}

// Called every frame
void AEgoVehicle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Update the positions based off replay data
    ReplayTick();

    // Get the current data from the AEgoSensor and use it
    UpdateSensor(DeltaSeconds);

    // Draw debug lines on editor
    DebugLines();

    // Render EgoVehicle dashboard
    UpdateDash();

    // Update the steering wheel to be responsive to user input
    TickSteeringWheel(DeltaSeconds);

    // Ensure appropriate autopilot functionality is accessible from EgoVehicle
    TickAutopilot();

    if (Pawn)
    {
        // Draw the spectator vr screen and overlay elements
        Pawn->DrawSpectatorScreen(EgoSensor->GetData()->GetGazeOrigin(DReyeVR::Gaze::LEFT),
                                  EgoSensor->GetData()->GetGazeDir(DReyeVR::Gaze::LEFT));

        // draws combined reticle
        Pawn->DrawFlatHUD(DeltaSeconds, EgoSensor->GetData()->GetGazeOrigin(), EgoSensor->GetData()->GetGazeDir());
    }

    // Update the world level
    TickLevel(DeltaSeconds);

    // Play sound that requires constant ticking
    TickSounds();
}

/// ========================================== ///
/// ----------------:CAMERA:------------------ ///
/// ========================================== ///

void AEgoVehicle::ConstructCameraRoot()
{
    // Spawn the RootComponent and Camera for the VR camera
    VRCameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRCameraRoot"));
    VRCameraRoot->SetupAttachment(GetRootComponent()); // The vehicle blueprint itself
    VRCameraRoot->SetRelativeLocation(FVector::ZeroVector);
    VRCameraRoot->SetRelativeRotation(FRotator::ZeroRotator);

    // taking this names directly from the [CameraPose] params in DReyeVRConfig.ini
    std::vector<FString> CameraPoses = {
        "DriversSeat",  // 1st
        "ThirdPerson",  // 2nd
        "BirdsEyeView", // 3rd
        "Front",        // 4th
    };
    for (FString &Key : CameraPoses)
    {
        FVector Location;
        FRotator Rotation;
        ReadConfigValue("CameraPose", Key + "Loc", Location);
        ReadConfigValue("CameraPose", Key + "Rot", Rotation);
        // converting FString to std::string for hashing
        FTransform Transform = FTransform(Rotation, Location, FVector::OneVector);
        CameraTransforms.push_back(std::make_pair(Key, Transform));
    }

    // assign the starting camera root pose to the given starting pose
    FString StartingPose;
    ReadConfigValue("CameraPose", "StartingPose", StartingPose);
    SetCameraRootPose(StartingPose);
}

void AEgoVehicle::SetCameraRootPose(const FString &CameraPoseName)
{
    // given a string (key), index into the map to obtain the FTransform
    FTransform CameraPoseTransform;
    bool bFoundMatchingPair = false;
    for (std::pair<FString, FTransform> &CamPose : CameraTransforms)
    {
        if (CamPose.first == CameraPoseName)
        {
            CameraPoseTransform = CamPose.second;
            bFoundMatchingPair = true;
            break;
        }
    }
    ensure(bFoundMatchingPair == true); // yells at you if CameraPoseName was not found
    SetCameraRootPose(CameraPoseTransform);
}

size_t AEgoVehicle::GetNumCameraPoses() const
{
    return CameraTransforms.size();
}

void AEgoVehicle::SetCameraRootPose(size_t CameraPoseIdx)
{
    // allow setting the camera root by indexing into CameraTransforms array
    CameraPoseIdx = std::min(CameraPoseIdx, CameraTransforms.size() - 1);
    SetCameraRootPose(CameraTransforms[CameraPoseIdx].second);
}

void AEgoVehicle::SetCameraRootPose(const FTransform &CameraPoseTransform)
{
    // sets the base posision of the Camera root (where the camera is at "rest")
    this->CameraPose = CameraPoseTransform;
    UE_LOG(LogTemp, Log, TEXT("Setting camera pose to: %s"), *(CameraPose + CameraPoseOffset).ToString());

    // First, set the root of the camera to the driver's seat head pos
    VRCameraRoot->SetRelativeLocation(CameraPose.GetLocation() + CameraPoseOffset.GetLocation());
    VRCameraRoot->SetRelativeRotation(CameraPose.Rotator() + CameraPoseOffset.Rotator());
}

void AEgoVehicle::SetPawn(ADReyeVRPawn *PawnIn)
{
    ensure(VRCameraRoot != nullptr);
    this->Pawn = PawnIn;
    ensure(Pawn != nullptr);
    this->FirstPersonCam = Pawn->GetCamera();
    ensure(FirstPersonCam != nullptr);
    FAttachmentTransformRules F(EAttachmentRule::KeepRelative, false);
    Pawn->AttachToComponent(VRCameraRoot, F);
    Pawn->GetCamera()->AttachToComponent(VRCameraRoot, F);
    // Then set the actual camera to be at its origin (attached to VRCameraRoot)
    FirstPersonCam->SetRelativeLocation(FVector::ZeroVector);
    FirstPersonCam->SetRelativeRotation(FRotator::ZeroRotator);
}

const UCameraComponent *AEgoVehicle::GetCamera() const
{
    return FirstPersonCam;
}
UCameraComponent *AEgoVehicle::GetCamera()
{
    return FirstPersonCam;
}
FVector AEgoVehicle::GetCameraOffset() const
{
    return VRCameraRoot->GetComponentLocation();
}
FVector AEgoVehicle::GetCameraPosn() const
{
    return GetCamera()->GetComponentLocation();
}
FVector AEgoVehicle::GetNextCameraPosn(const float DeltaSeconds) const
{
    // usually only need this is tick before physics
    return GetCameraPosn() + DeltaSeconds * GetVelocity();
}
FRotator AEgoVehicle::GetCameraRot() const
{
    return GetCamera()->GetComponentRotation();
}

/// ========================================== ///
/// ---------------:AIPLAYER:----------------- ///
/// ========================================== ///

void AEgoVehicle::InitAIPlayer()
{
    AI_Player = Cast<AWheeledVehicleAIController>(this->GetController());
    ensure(AI_Player != nullptr);
}

void AEgoVehicle::SetAutopilot(const bool AutopilotOn)
{
    bAutopilotEnabled = AutopilotOn;
    AI_Player->SetAutopilot(bAutopilotEnabled);
    AI_Player->SetStickyControl(bAutopilotEnabled);
}

bool AEgoVehicle::GetAutopilotStatus() const
{
    return bAutopilotEnabled;
}

void AEgoVehicle::TickAutopilot()
{
    ensure(AI_Player != nullptr);
    if (AI_Player != nullptr)
    {
        bAutopilotEnabled = AI_Player->IsAutopilotEnabled();
    }
}

/// ========================================== ///
/// ----------------:SENSOR:------------------ ///
/// ========================================== ///

void AEgoVehicle::InitSensor()
{
    // Spawn the EyeTracker Carla sensor and attach to Ego-Vehicle:
    FActorSpawnParameters EyeTrackerSpawnInfo;
    EyeTrackerSpawnInfo.Owner = this;
    EyeTrackerSpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    EgoSensor = World->SpawnActor<AEgoSensor>(GetCameraPosn(), FRotator::ZeroRotator, EyeTrackerSpawnInfo);
    check(EgoSensor != nullptr);
    // Attach the EgoSensor as a child to the EgoVehicle
    EgoSensor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
    EgoSensor->SetEgoVehicle(this);
    if (DReyeVRLevel)
        EgoSensor->SetLevel(DReyeVRLevel);
}

void AEgoVehicle::ReplayTick()
{
    const bool bIsReplaying = EgoSensor->IsReplaying();
    // need to enable/disable VehicleMesh simulation
    class USkeletalMeshComponent *VehicleMesh = GetMesh();
    if (VehicleMesh)
        VehicleMesh->SetSimulatePhysics(!bIsReplaying); // disable physics when replaying (teleporting)
    if (FirstPersonCam)
        FirstPersonCam->bLockToHmd = !bIsReplaying; // only lock orientation and position to HMD when not replaying

    // perform all sensor updates that occur when replaying
    if (bIsReplaying)
    {
        // this gets reached when the simulator is replaying data from a carla log
        const DReyeVR::AggregateData *Replay = EgoSensor->GetData();

        // include positional update here, else there is lag/jitter between the camera and the vehicle
        // since the Carla Replayer tick differs from the EgoVehicle tick
        const FTransform ReplayTransform(Replay->GetVehicleRotation(), // FRotator (Rotation)
                                         Replay->GetVehicleLocation(), // FVector (Location)
                                         FVector::OneVector);          // FVector (Scale3D)
        // see https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/Engine/ETeleportType/
        SetActorTransform(ReplayTransform, false, nullptr, ETeleportType::TeleportPhysics);

        if (bCameraFollowHMD)
        {
            // assign first person camera orientation and location (absolute)
            const FTransform ReplayCameraTransAbs(Replay->GetCameraRotationAbs(), // FRotator (Rotation)
                                                  Replay->GetCameraLocationAbs(), // FVector (Location)
                                                  FVector::OneVector);            // FVector (Scale3D)
            FirstPersonCam->SetWorldTransform(ReplayCameraTransAbs, false, nullptr, ETeleportType::TeleportPhysics);
        }
        else
        {
            // reset to forward view
            FirstPersonCam->SetRelativeLocation(FVector::ZeroVector);
            FirstPersonCam->SetRelativeRotation(FRotator::ZeroRotator);
        }

        // overwrite vehicle inputs to use the replay data
        VehicleInputs = Replay->GetUserInputs();
    }
}

void AEgoVehicle::UpdateSensor(const float DeltaSeconds)
{
    // Explicitly update the EgoSensor here, synchronized with EgoVehicle tick
    EgoSensor->ManualTick(DeltaSeconds); // Ensures we always get the latest data

    // Calculate gaze data (in world space) using eye tracker data
    const DReyeVR::AggregateData *Data = EgoSensor->GetData();
    // Compute World positions and orientations
    const FRotator WorldRot = FirstPersonCam->GetComponentRotation();
    const FVector WorldPos = FirstPersonCam->GetComponentLocation();

    // First get the gaze origin and direction and vergence from the EyeTracker Sensor
    const float RayLength = FMath::Max(1.f, Data->GetGazeVergence() / 100.f); // vergence to m (from cm)
    const float VRMeterScale = 100.f;

    // Both eyes
    CombinedGaze = RayLength * VRMeterScale * Data->GetGazeDir();
    CombinedOrigin = WorldPos + WorldRot.RotateVector(Data->GetGazeOrigin());

    // Left eye
    LeftGaze = RayLength * VRMeterScale * Data->GetGazeDir(DReyeVR::Gaze::LEFT);
    LeftOrigin = WorldPos + WorldRot.RotateVector(Data->GetGazeOrigin(DReyeVR::Gaze::LEFT));

    // Right eye
    RightGaze = RayLength * VRMeterScale * Data->GetGazeDir(DReyeVR::Gaze::RIGHT);
    RightOrigin = WorldPos + WorldRot.RotateVector(Data->GetGazeOrigin(DReyeVR::Gaze::RIGHT));
}

/// ========================================== ///
/// ----------------:MIRROR:------------------ ///
/// ========================================== ///

void AEgoVehicle::MirrorParams::Initialize(class UStaticMeshComponent *MirrorSM,
                                           class UPlanarReflectionComponent *Reflection,
                                           class USkeletalMeshComponent *VehicleMesh)
{
    UE_LOG(LogTemp, Log, TEXT("Initializing %s mirror"), *Name)

    check(MirrorSM != nullptr);
    MirrorSM->SetupAttachment(VehicleMesh);
    MirrorSM->SetRelativeLocation(MirrorPos);
    MirrorSM->SetRelativeRotation(MirrorRot);
    MirrorSM->SetRelativeScale3D(MirrorScale);
    MirrorSM->SetGenerateOverlapEvents(false); // don't collide with itself
    MirrorSM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MirrorSM->SetVisibility(true);

    check(Reflection != nullptr);
    Reflection->SetupAttachment(MirrorSM);
    Reflection->SetRelativeLocation(ReflectionPos);
    Reflection->SetRelativeRotation(ReflectionRot);
    Reflection->SetRelativeScale3D(ReflectionScale);
    Reflection->NormalDistortionStrength = 0.0f;
    Reflection->PrefilterRoughness = 0.0f;
    Reflection->DistanceFromPlaneFadeoutStart = 1500.f;
    Reflection->DistanceFromPlaneFadeoutEnd = 0.f;
    Reflection->AngleFromPlaneFadeStart = 0.f;
    Reflection->AngleFromPlaneFadeEnd = 90.f;
    Reflection->PrefilterRoughnessDistance = 10000.f;
    Reflection->ScreenPercentage = ScreenPercentage; // change this to reduce quality & improve performance
    Reflection->bShowPreviewPlane = false;
    Reflection->HideComponent(VehicleMesh);
    Reflection->SetVisibility(true);
    /// TODO: use USceneCaptureComponent::ShowFlags to define what gets rendered in the mirror
    // https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/FEngineShowFlags/
}

void AEgoVehicle::ConstructMirrors()
{

    class USkeletalMeshComponent *VehicleMesh = GetMesh();
    /// Rear mirror
    if (RearMirrorParams.Enabled)
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> RearSM(
            TEXT("StaticMesh'/Game/Carla/Blueprints/Vehicles/DReyeVR/Mirrors/"
                 "RearMirror_DReyeVR_Glass_SM.RearMirror_DReyeVR_Glass_SM'"));
        RearMirrorSM = CreateDefaultSubobject<UStaticMeshComponent>(FName(*(RearMirrorParams.Name + "MirrorSM")));
        RearMirrorSM->SetStaticMesh(RearSM.Object);
        RearReflection = CreateDefaultSubobject<UPlanarReflectionComponent>(FName(*(RearMirrorParams.Name + "Refl")));
        RearMirrorParams.Initialize(RearMirrorSM, RearReflection, VehicleMesh);
        // also add the chassis for this mirror
        static ConstructorHelpers::FObjectFinder<UStaticMesh> RearChassisSM(TEXT(
            "StaticMesh'/Game/Carla/Blueprints/Vehicles/DReyeVR/Mirrors/RearMirror_DReyeVR_SM.RearMirror_DReyeVR_SM'"));
        RearMirrorChassisSM =
            CreateDefaultSubobject<UStaticMeshComponent>(FName(*(RearMirrorParams.Name + "MirrorChassisSM")));
        RearMirrorChassisSM->SetStaticMesh(RearChassisSM.Object);
        RearMirrorChassisSM->SetupAttachment(VehicleMesh);
        RearMirrorChassisSM->SetRelativeLocation(RearMirrorChassisPos);
        RearMirrorChassisSM->SetRelativeRotation(RearMirrorChassisRot);
        RearMirrorChassisSM->SetRelativeScale3D(RearMirrorChassisScale);
        RearMirrorChassisSM->SetGenerateOverlapEvents(false); // don't collide with itself
        RearMirrorChassisSM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        RearMirrorChassisSM->SetVisibility(true);
        RearMirrorSM->SetupAttachment(RearMirrorChassisSM);
        RearReflection->HideComponent(RearMirrorChassisSM); // don't show this in the reflection
    }
    /// Left mirror
    if (LeftMirrorParams.Enabled)
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> LeftSM(TEXT(
            "StaticMesh'/Game/Carla/Blueprints/Vehicles/DReyeVR/Mirrors/LeftMirror_DReyeVR_SM.LeftMirror_DReyeVR_SM'"));
        LeftMirrorSM = CreateDefaultSubobject<UStaticMeshComponent>(FName(*(LeftMirrorParams.Name + "MirrorSM")));
        LeftMirrorSM->SetStaticMesh(LeftSM.Object);
        LeftReflection = CreateDefaultSubobject<UPlanarReflectionComponent>(FName(*(LeftMirrorParams.Name + "Refl")));
        LeftMirrorParams.Initialize(LeftMirrorSM, LeftReflection, VehicleMesh);
    }
    /// Right mirror
    if (RightMirrorParams.Enabled)
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> RightSM(
            TEXT("StaticMesh'/Game/Carla/Blueprints/Vehicles/DReyeVR/Mirrors/"
                 "RightMirror_DReyeVR_SM.RightMirror_DReyeVR_SM'"));
        RightMirrorSM = CreateDefaultSubobject<UStaticMeshComponent>(FName(*(RightMirrorParams.Name + "MirrorSM")));
        RightMirrorSM->SetStaticMesh(RightSM.Object);
        RightReflection = CreateDefaultSubobject<UPlanarReflectionComponent>(FName(*(RightMirrorParams.Name + "Refl")));
        RightMirrorParams.Initialize(RightMirrorSM, RightReflection, VehicleMesh);
    }
}

/// ========================================== ///
/// ----------------:SOUNDS:------------------ ///
/// ========================================== ///

void AEgoVehicle::ConstructEgoSounds()
{
    // Initialize ego-centric audio components
    // See ACarlaWheeledVehicle::ConstructSounds for all Vehicle sounds
    ensureMsgf(EngineRevSound != nullptr, TEXT("Vehicle engine rev should be initialized!"));
    ensureMsgf(CrashSound != nullptr, TEXT("Vehicle crash sound should be initialized!"));

    static ConstructorHelpers::FObjectFinder<USoundWave> GearSound(
        TEXT("SoundWave'/Game/Carla/Blueprints/Vehicles/DReyeVR/Sounds/GearShift.GearShift'"));
    GearShiftSound = CreateDefaultSubobject<UAudioComponent>(TEXT("GearShift"));
    GearShiftSound->SetupAttachment(GetRootComponent());
    GearShiftSound->bAutoActivate = false;
    GearShiftSound->SetSound(GearSound.Object);

    static ConstructorHelpers::FObjectFinder<USoundWave> TurnSignalSoundWave(
        TEXT("SoundWave'/Game/Carla/Blueprints/Vehicles/DReyeVR/Sounds/TurnSignal.TurnSignal'"));
    TurnSignalSound = CreateDefaultSubobject<UAudioComponent>(TEXT("TurnSignal"));
    TurnSignalSound->SetupAttachment(GetRootComponent());
    TurnSignalSound->bAutoActivate = false;
    TurnSignalSound->SetSound(TurnSignalSoundWave.Object);
}

void AEgoVehicle::PlayGearShiftSound(const float DelayBeforePlay) const
{
    if (this->GearShiftSound)
        GearShiftSound->Play(DelayBeforePlay);
}

void AEgoVehicle::PlayTurnSignalSound(const float DelayBeforePlay) const
{
    if (this->TurnSignalSound)
        this->TurnSignalSound->Play(DelayBeforePlay);
}

void AEgoVehicle::SetVolume(const float VolumeIn)
{
    if (GearShiftSound)
        GearShiftSound->SetVolumeMultiplier(VolumeIn);
    if (TurnSignalSound)
        TurnSignalSound->SetVolumeMultiplier(VolumeIn);
    Super::SetVolume(VolumeIn);
}

/// ========================================== ///
/// -----------------:DASH:------------------- ///
/// ========================================== ///

void AEgoVehicle::ConstructDashText() // dashboard text (speedometer, turn signals, gear shifter)
{
    // Create speedometer
    Speedometer = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Speedometer"));
    Speedometer->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    Speedometer->SetRelativeLocation(DashboardLocnInVehicle);
    Speedometer->SetRelativeRotation(FRotator(0.f, 180.f, 0.f)); // need to flip it to get the text in driver POV
    Speedometer->SetTextRenderColor(FColor::Red);
    Speedometer->SetText(FText::FromString("0"));
    Speedometer->SetXScale(1.f);
    Speedometer->SetYScale(1.f);
    Speedometer->SetWorldSize(10); // scale the font with this
    Speedometer->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
    Speedometer->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    SpeedometerScale = CmPerSecondToXPerHour(bUseMPH);

    // Create turn signals
    TurnSignals = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TurnSignals"));
    TurnSignals->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    TurnSignals->SetRelativeLocation(DashboardLocnInVehicle + FVector(0, 11.f, -5.f));
    TurnSignals->SetRelativeRotation(FRotator(0.f, 180.f, 0.f)); // need to flip it to get the text in driver POV
    TurnSignals->SetTextRenderColor(FColor::Red);
    TurnSignals->SetText(FText::FromString(""));
    TurnSignals->SetXScale(1.f);
    TurnSignals->SetYScale(1.f);
    TurnSignals->SetWorldSize(10); // scale the font with this
    TurnSignals->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
    TurnSignals->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);

    // Create gear shifter
    GearShifter = CreateDefaultSubobject<UTextRenderComponent>(TEXT("GearShifter"));
    GearShifter->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    GearShifter->SetRelativeLocation(DashboardLocnInVehicle + FVector(0, 15.f, 0));
    GearShifter->SetRelativeRotation(FRotator(0.f, 180.f, 0.f)); // need to flip it to get the text in driver POV
    GearShifter->SetTextRenderColor(FColor::Red);
    GearShifter->SetText(FText::FromString("D"));
    GearShifter->SetXScale(1.f);
    GearShifter->SetYScale(1.f);
    GearShifter->SetWorldSize(10); // scale the font with this
    GearShifter->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
    GearShifter->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
}

void AEgoVehicle::UpdateDash()
{
    // Draw text components
    float XPH; // miles-per-hour or km-per-hour
    if (EgoSensor->IsReplaying())
    {
        const DReyeVR::AggregateData *Replay = EgoSensor->GetData();
        XPH = Replay->GetVehicleVelocity() * SpeedometerScale; // FwdSpeed is in cm/s
        if (Replay->GetUserInputs().ToggledReverse)
        {
            bReverse = !bReverse;
            PlayGearShiftSound();
        }
        if (bEnableTurnSignalAction)
        {
            if (Replay->GetUserInputs().TurnSignalLeft)
            {
                LeftSignalTimeToDie = FPlatformTime::Seconds() + TurnSignalDuration;
                RightSignalTimeToDie = 0.f;
                PlayTurnSignalSound();
            }
            if (Replay->GetUserInputs().TurnSignalRight)
            {
                RightSignalTimeToDie = FPlatformTime::Seconds() + TurnSignalDuration;
                LeftSignalTimeToDie = 0.f;
                PlayTurnSignalSound();
            }
        }
    }
    else
    {
        XPH = GetVehicleForwardSpeed() * SpeedometerScale; // FwdSpeed is in cm/s
    }

    const FString Data = FString::FromInt(int(FMath::RoundHalfFromZero(XPH)));
    Speedometer->SetText(FText::FromString(Data));

    if (bEnableTurnSignalAction)
    {
        // Draw the signals
        float Now = FPlatformTime::Seconds();
        if (Now < RightSignalTimeToDie)
            TurnSignals->SetText(FText::FromString(">>>"));
        else if (Now < LeftSignalTimeToDie)
            TurnSignals->SetText(FText::FromString("<<<"));
        else
            TurnSignals->SetText(FText::FromString("")); // nothing
    }

    // Draw the gear shifter
    if (bReverse)
        GearShifter->SetText(FText::FromString("R"));
    else
        GearShifter->SetText(FText::FromString("D"));
}

/// ========================================== ///
/// -----------------:WHEEL:------------------ ///
/// ========================================== ///

void AEgoVehicle::ConstructSteeringWheel()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SteeringWheelSM(
        TEXT("StaticMesh'/Game/Carla/Blueprints/Vehicles/DReyeVR/SteeringWheel/"
             "SM_SteeringWheel_DReyeVR.SM_SteeringWheel_DReyeVR'"));
    SteeringWheel = CreateDefaultSubobject<UStaticMeshComponent>(FName("SteeringWheel"));
    SteeringWheel->SetStaticMesh(SteeringWheelSM.Object);
    SteeringWheel->SetupAttachment(GetRootComponent()); // The vehicle blueprint itself
    SteeringWheel->SetRelativeLocation(InitWheelLocation);
    SteeringWheel->SetRelativeRotation(InitWheelRotation);
    SteeringWheel->SetGenerateOverlapEvents(false); // don't collide with itself
    SteeringWheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SteeringWheel->SetVisibility(true);
}

void AEgoVehicle::TickSteeringWheel(const float DeltaTime)
{
    const FRotator CurrentRotation = SteeringWheel->GetRelativeRotation();
    const float RawSteering = GetVehicleInputs().Steering; // this is scaled in SetSteering
    const float TargetAngle = (RawSteering / ScaleSteeringInput) * SteeringAnimScale;
    FRotator NewRotation = CurrentRotation;
    if (Pawn && Pawn->GetIsLogiConnected())
    {
        NewRotation.Roll = TargetAngle;
    }
    else
    {
        float DeltaAngle = (TargetAngle - CurrentRotation.Roll);

        // place a speed-limit on the steering wheel
        DeltaAngle = FMath::Clamp(DeltaAngle, -MaxSteerVelocity, MaxSteerVelocity);

        // create the new rotation using the deltas
        NewRotation += DeltaTime * FRotator(0.f, 0.f, DeltaAngle);

        // Clamp the roll amount so the wheel can't spin infinitely
        NewRotation.Roll = FMath::Clamp(NewRotation.Roll, -MaxSteerAngleDeg, MaxSteerAngleDeg);
    }
    SteeringWheel->SetRelativeRotation(NewRotation);
}

/// ========================================== ///
/// -----------------:LEVEL:------------------ ///
/// ========================================== ///

void AEgoVehicle::SetLevel(ADReyeVRLevel *Level)
{
    this->DReyeVRLevel = Level;
    check(DReyeVRLevel != nullptr);
}

void AEgoVehicle::TickLevel(float DeltaSeconds)
{
    if (this->DReyeVRLevel != nullptr)
        DReyeVRLevel->Tick(DeltaSeconds);
}

/// ========================================== ///
/// -----------------:OTHER:------------------ ///
/// ========================================== ///

void AEgoVehicle::Register()
{
    FCarlaActor::IdType ID = EgoVehicleID;
    FActorDescription Description;
    Description.Class = ACarlaWheeledVehicle::StaticClass();
    Description.Id = "vehicle.dreyevr.egovehicle";
    Description.UId = ID;
    // ensure this vehicle is denoted by the 'hero' attribute
    FActorAttribute HeroRole;
    HeroRole.Id = "role_name";
    HeroRole.Type = EActorAttributeType::String;
    HeroRole.Value = "hero";
    Description.Variations.Add(HeroRole.Id, std::move(HeroRole));
    // ensure the vehicle has attributes denoting number of wheels
    FActorAttribute NumWheels;
    NumWheels.Id = "number_of_wheels";
    NumWheels.Type = EActorAttributeType::Int;
    NumWheels.Value = "4";
    Description.Variations.Add(NumWheels.Id, std::move(NumWheels));
    FString RegistryTags = "EgoVehicle,DReyeVR";
    Episode->RegisterActor(*this, Description, RegistryTags, ID);
}

/// ========================================== ///
/// ---------------:COSMETIC:----------------- ///
/// ========================================== ///

void AEgoVehicle::DebugLines() const
{
#if WITH_EDITOR
    // Compute World positions and orientations
    const FRotator WorldRot = FirstPersonCam->GetComponentRotation();
    // Rotate and add the gaze ray to the origin
    FVector CombinedGazePosn = CombinedOrigin + WorldRot.RotateVector(CombinedGaze);

    // Use Absolute Ray Position to draw debug information
    if (bDrawDebugEditor)
    {
        DrawDebugSphere(World, CombinedGazePosn, 4.0f, 12, FColor::Blue);

        // Draw individual rays for left and right eye
        DrawDebugLine(World,
                      LeftOrigin,                                        // start line
                      LeftOrigin + 10 * WorldRot.RotateVector(LeftGaze), // end line
                      FColor::Green, false, -1, 0, 1);

        DrawDebugLine(World,
                      RightOrigin,                                         // start line
                      RightOrigin + 10 * WorldRot.RotateVector(RightGaze), // end line
                      FColor::Yellow, false, -1, 0, 1);
    }
#endif
}