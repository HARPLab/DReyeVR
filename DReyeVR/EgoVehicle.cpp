#include "EgoVehicle.h"
#include "Carla/Actor/ActorAttribute.h"             // FActorAttribute
#include "Carla/Actor/ActorRegistry.h"              // Register
#include "Carla/Game/CarlaStatics.h"                // GetCurrentEpisode
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
    LOG("Constructing Ego Vehicle: %s", *FString(this->GetName()));

    ReadConfigVariables();

    // this actor ticks AFTER the physics simulation is done
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;

    // Set up the root position to be the this mesh
    SetRootComponent(GetMesh());

    // Initialize the rigid body static mesh
    ConstructRigidBody();

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

    LOG("Finished constructing %s", *FString(this->GetName()));
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
        ReadConfigValue("Mirrors", Params.Name + "MirrorTransform", Params.MirrorTransform);
        ReadConfigValue("Mirrors", Params.Name + "ReflectionTransform", Params.ReflectionTransform);
        ReadConfigValue("Mirrors", Params.Name + "ScreenPercentage", Params.ScreenPercentage);
    };
    InitMirrorParams("Rear", RearMirrorParams);
    InitMirrorParams("Left", LeftMirrorParams);
    InitMirrorParams("Right", RightMirrorParams);
    // rear mirror chassis
    ReadConfigValue("Mirrors", "RearMirrorChassisTransform", RearMirrorChassisTransform);
    // steering wheel
    ReadConfigValue("SteeringWheel", "InitLocation", InitWheelLocation);
    ReadConfigValue("SteeringWheel", "InitRotation", InitWheelRotation);
    ReadConfigValue("SteeringWheel", "MaxSteerAngleDeg", MaxSteerAngleDeg);
    ReadConfigValue("SteeringWheel", "SteeringScale", SteeringAnimScale);
    // other/cosmetic
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
    ensure(World != nullptr);

    // initialize
    InitAIPlayer();

    // Bug-workaround for initial delay on throttle; see https://github.com/carla-simulator/carla/issues/1640
    this->GetVehicleMovementComponent()->SetTargetGear(1, true);

    // get the GameMode script
    SetGame(Cast<ADReyeVRGameMode>(UGameplayStatics::GetGameMode(World)));

    LOG("Initialized DReyeVR EgoVehicle");
}

void AEgoVehicle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/Engine/EEndPlayReason__Type/
    if (EndPlayReason == EEndPlayReason::Destroyed)
    {
        LOG("DReyeVR EgoVehicle is being destroyed! You'll need to spawn another one!");
    }

    if (GetGame())
    {
        GetGame()->SetEgoVehicle(nullptr);
        GetGame()->PossessSpectator();
    }

    if (this->Pawn)
    {
        this->Pawn->SetEgoVehicle(nullptr);
    }
}

void AEgoVehicle::BeginDestroy()
{
    Super::BeginDestroy();

    // destroy all spawned entities
    if (EgoSensor)
        EgoSensor->Destroy();

    LOG("EgoVehicle has been destroyed");
}

// Called every frame
void AEgoVehicle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Get the current data from the AEgoSensor and use it
    UpdateSensor(DeltaSeconds);

    // Update the positions based off replay data
    ReplayTick();

    // Draw debug lines on editor
    DebugLines();

    // Render EgoVehicle dashboard
    UpdateDash();

    // Update the steering wheel to be responsive to user input
    TickSteeringWheel(DeltaSeconds);

    // Ensure appropriate autopilot functionality is accessible from EgoVehicle
    TickAutopilot();

    // Update the world level
    TickGame(DeltaSeconds);

    // Play sound that requires constant ticking
    TickSounds();
}

void AEgoVehicle::ConstructRigidBody()
{
#if 0
    /// TODO: re-enable this code once spawning from DReyeVR needs to be done without a hardcoded blueprint asset
    ///       to see this effect remove the reference to EgoVehicleBPClass in DReyeVRFactory.cpp once implemented

    // https://forums.unrealengine.com/t/cannot-create-vehicle-updatedcomponent-has-not-initialized-its-rigid-body-actor/461662
    /// NOTE: this must be run in the constructors!

    // load skeletal mesh (static mesh)
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> CarMesh(TEXT(
        "SkeletalMesh'/Game/DReyeVR/EgoVehicle/model3/Mesh/SkeletalMesh_model3.SkeletalMesh_model3'"));
    // original: "SkeletalMesh'/Game/Carla/Static/Car/4Wheeled/Tesla/SM_TeslaM3_v2.SM_TeslaM3_v2'"
    USkeletalMesh *SkeletalMesh = CarMesh.Object;
    if (SkeletalMesh == nullptr)
    {
        LOG_ERROR("Unable to load skeletal mesh!");
        return;
    }

    // load skeleton (for animations)
    static ConstructorHelpers::FObjectFinder<USkeleton> CarSkeleton(
        TEXT("Skeleton'/Game/DReyeVR/EgoVehicle/model3/Mesh/Skeleton_model3.Skeleton_model3'"));
    // original:
    // "Skeleton'/Game/Carla/Static/Car/4Wheeled/Tesla/SM_TeslaM3_lights_body_Skeleton.SM_TeslaM3_lights_body_Skeleton'"
    USkeleton *Skeleton = CarSkeleton.Object;
    if (Skeleton == nullptr)
    {
        LOG_ERROR("Unable to load skeleton!");
        return;
    }

    // load animations bp
    static ConstructorHelpers::FClassFinder<UObject> AnimBPClass(
        TEXT("/Game/DReyeVR/EgoVehicle/model3/Mesh/Animation_model3.Animation_model3_C"));
    // original: "/Game/Carla/Static/Car/4Wheeled/Tesla/Tesla_Animation.Tesla_Animation_C"
    auto AnimInstance = AnimBPClass.Class;
    if (!AnimBPClass.Succeeded())
    {
        LOG_ERROR("Unable to load Animation!");
        return;
    }

    // load physics asset
    static ConstructorHelpers::FObjectFinder<UPhysicsAsset> CarPhysics(TEXT(
        "PhysicsAsset'/Game/DReyeVR/EgoVehicle/model3/Mesh/Physics_model3.Physics_model3'"));
    // original: "PhysicsAsset'/Game/Carla/Static/Car/4Wheeled/Tesla/SM_TeslaM3_PhysicsAsset.SM_TeslaM3_PhysicsAsset'"
    UPhysicsAsset *PhysicsAsset = CarPhysics.Object;
    if (PhysicsAsset == nullptr)
    {
        LOG_ERROR("Unable to load PhysicsAsset!");
        return;
    }

    // contrary to https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/Engine/USkeletalMesh/SetSkeleton/
    SkeletalMesh->Skeleton = Skeleton;
    SkeletalMesh->PhysicsAsset = PhysicsAsset;
    SkeletalMesh->Build();

    USkeletalMeshComponent *Mesh = GetMesh();
    check(Mesh != nullptr);
    Mesh->SetSkeletalMesh(SkeletalMesh, true);
    Mesh->SetAnimInstanceClass(AnimInstance);
    Mesh->SetPhysicsAsset(PhysicsAsset);
    Mesh->RecreatePhysicsState();
    this->GetVehicleMovementComponent()->RecreatePhysicsState();

    // sanity checks
    ensure(Mesh->GetPhysicsAsset() != nullptr);

    LOG("Successfully created EgoVehicle rigid body");
#endif
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
    // LOG("Setting camera pose to: %s", *(CameraPose + CameraPoseOffset).ToString());

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
    FirstPersonCam->AttachToComponent(VRCameraRoot, F);
    // Then set the actual camera to be at its origin (attached to VRCameraRoot)
    FirstPersonCam->SetRelativeLocation(FVector::ZeroVector);
    FirstPersonCam->SetRelativeRotation(FRotator::ZeroRotator);
}

const UCameraComponent *AEgoVehicle::GetCamera() const
{
    ensure(FirstPersonCam != nullptr);
    return FirstPersonCam;
}
UCameraComponent *AEgoVehicle::GetCamera()
{
    ensure(FirstPersonCam != nullptr);
    return FirstPersonCam;
}
FVector AEgoVehicle::GetCameraOffset() const
{
    return VRCameraRoot->GetComponentLocation();
}
FVector AEgoVehicle::GetCameraPosn() const
{
    return GetCamera() ? GetCamera()->GetComponentLocation() : FVector::ZeroVector;
}
FVector AEgoVehicle::GetNextCameraPosn(const float DeltaSeconds) const
{
    // usually only need this is tick before physics
    return GetCameraPosn() + DeltaSeconds * GetVelocity();
}
FRotator AEgoVehicle::GetCameraRot() const
{
    return GetCamera() ? GetCamera()->GetComponentRotation() : FRotator::ZeroRotator;
}
const class AEgoSensor *AEgoVehicle::GetSensor() const
{
    return const_cast<const class AEgoSensor *>(EgoSensor);
}

/// ========================================== ///
/// ---------------:AIPLAYER:----------------- ///
/// ========================================== ///

void AEgoVehicle::InitAIPlayer()
{
    this->SpawnDefaultController(); // spawns default (AI) controller and gets possessed by it
    auto PlayerController = this->GetController();
    ensure(PlayerController != nullptr);
    AI_Player = Cast<AWheeledVehicleAIController>(PlayerController);
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
    // update the world on refresh (ex. --reloadWorld)
    World = GetWorld();
    check(World != nullptr);
    // Spawn the EyeTracker Carla sensor and attach to Ego-Vehicle:
    {
        UCarlaEpisode *Episode = UCarlaStatics::GetCurrentEpisode(World);
        check(Episode != nullptr);
        FActorDefinition EgoSensorDef = FindDefnInRegistry(Episode, AEgoSensor::StaticClass());
        FActorDescription Description;
        { // create a Description from the Definition to spawn the actor
            Description.UId = EgoSensorDef.UId;
            Description.Id = EgoSensorDef.Id;
            Description.Class = EgoSensorDef.Class;
        }

        if (Episode == nullptr)
        {
            LOG_ERROR("Null Episode in world!");
        }
        // calls Episode::SpawnActor => SpawnActorWithInfo => ActorDispatcher->SpawnActor => SpawnFunctions[UId]
        FTransform SpawnPt = FTransform(FRotator::ZeroRotator, GetCameraPosn(), FVector::OneVector);
        EgoSensor = static_cast<AEgoSensor *>(Episode->SpawnActor(SpawnPt, Description));
    }
    check(EgoSensor != nullptr);
    // Attach the EgoSensor as a child to the EgoVehicle
    EgoSensor->SetOwner(this);
    EgoSensor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
    EgoSensor->SetEgoVehicle(this);
    if (DReyeVRGame)
        EgoSensor->SetGame(DReyeVRGame);
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

        // set the camera reenactment orientation
        {
            const FTransform CameraOrientation =
                bCameraFollowHMD // follow HMD reenacts all the head movmeents that were recorded
                    ? FTransform(Replay->GetCameraRotation(), Replay->GetCameraLocation(), FVector::OneVector)
                    : FTransform::Identity; // otherwise just point forward (neutral position)
            FirstPersonCam->SetRelativeTransform(CameraOrientation, false, nullptr, ETeleportType::TeleportPhysics);
        }

        // overwrite vehicle inputs to use the replay data
        VehicleInputs = Replay->GetUserInputs();
    }
}

void AEgoVehicle::UpdateSensor(const float DeltaSeconds)
{
    if (EgoSensor == nullptr) // Spawn and attach the EgoSensor
    {
        // unfortunately World->SpawnActor *sometimes* fails if used in BeginPlay so
        // calling it once in the tick is fine to avoid this crash.
        InitSensor();
    }

    ensure(EgoSensor != nullptr);
    if (EgoSensor == nullptr)
    {
        LOG_WARN("EgoSensor initialization failed!");
        return;
    }

    // Explicitly update the EgoSensor here, synchronized with EgoVehicle tick
    EgoSensor->ManualTick(DeltaSeconds); // Ensures we always get the latest data
}

/// ========================================== ///
/// ----------------:MIRROR:------------------ ///
/// ========================================== ///

void AEgoVehicle::MirrorParams::Initialize(class UStaticMeshComponent *MirrorSM,
                                           class UPlanarReflectionComponent *Reflection,
                                           class USkeletalMeshComponent *VehicleMesh)
{
    check(MirrorSM != nullptr);
    MirrorSM->SetupAttachment(VehicleMesh);
    MirrorSM->SetRelativeLocation(MirrorTransform.GetLocation());
    MirrorSM->SetRelativeRotation(MirrorTransform.Rotator());
    MirrorSM->SetRelativeScale3D(MirrorTransform.GetScale3D());
    MirrorSM->SetGenerateOverlapEvents(false); // don't collide with itself
    MirrorSM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MirrorSM->SetVisibility(Enabled);

    check(Reflection != nullptr);
    Reflection->SetupAttachment(MirrorSM);
    Reflection->SetRelativeLocation(ReflectionTransform.GetLocation());
    Reflection->SetRelativeRotation(ReflectionTransform.Rotator());
    Reflection->SetRelativeScale3D(ReflectionTransform.GetScale3D());
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
    Reflection->SetVisibility(Enabled);
    /// TODO: use USceneCaptureComponent::ShowFlags to define what gets rendered in the mirror
    // https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/FEngineShowFlags/
}

void AEgoVehicle::ConstructMirrors()
{

    class USkeletalMeshComponent *VehicleMesh = GetMesh();
    /// Rear mirror
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> RearSM(
            TEXT("StaticMesh'/Game/DReyeVR/EgoVehicle/model3/Mirrors/RearMirror_model3.RearMirror_model3'"));
        RearMirrorSM = CreateDefaultSubobject<UStaticMeshComponent>(FName(*(RearMirrorParams.Name + "MirrorSM")));
        RearMirrorSM->SetStaticMesh(RearSM.Object);
        RearReflection = CreateDefaultSubobject<UPlanarReflectionComponent>(FName(*(RearMirrorParams.Name + "Refl")));
        RearMirrorParams.Initialize(RearMirrorSM, RearReflection, VehicleMesh);
        // also add the chassis for this mirror
        static ConstructorHelpers::FObjectFinder<UStaticMesh> RearChassisSM(
            TEXT("StaticMesh'/Game/DReyeVR/EgoVehicle/model3/Mirrors/RearMirror_Mesh_model3.RearMirror_Mesh_model3'"));
        RearMirrorChassisSM =
            CreateDefaultSubobject<UStaticMeshComponent>(FName(*(RearMirrorParams.Name + "MirrorChassisSM")));
        RearMirrorChassisSM->SetStaticMesh(RearChassisSM.Object);
        RearMirrorChassisSM->SetupAttachment(VehicleMesh);
        RearMirrorChassisSM->SetRelativeLocation(RearMirrorChassisTransform.GetLocation());
        RearMirrorChassisSM->SetRelativeRotation(RearMirrorChassisTransform.Rotator());
        RearMirrorChassisSM->SetRelativeScale3D(RearMirrorChassisTransform.GetScale3D());
        RearMirrorChassisSM->SetGenerateOverlapEvents(false); // don't collide with itself
        RearMirrorChassisSM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        RearMirrorChassisSM->SetVisibility(RearMirrorParams.Enabled);
        RearMirrorSM->SetupAttachment(RearMirrorChassisSM);
        RearReflection->HideComponent(RearMirrorChassisSM); // don't show this in the reflection
    }
    /// Left mirror
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> LeftSM(
            TEXT("StaticMesh'/Game/DReyeVR/EgoVehicle/model3/Mirrors/LeftMirror_model3.LeftMirror_model3'"));
        LeftMirrorSM = CreateDefaultSubobject<UStaticMeshComponent>(FName(*(LeftMirrorParams.Name + "MirrorSM")));
        LeftMirrorSM->SetStaticMesh(LeftSM.Object);
        LeftReflection = CreateDefaultSubobject<UPlanarReflectionComponent>(FName(*(LeftMirrorParams.Name + "Refl")));
        LeftMirrorParams.Initialize(LeftMirrorSM, LeftReflection, VehicleMesh);
    }
    /// Right mirror
    {
        static ConstructorHelpers::FObjectFinder<UStaticMesh> RightSM(
            TEXT("StaticMesh'/Game/DReyeVR/EgoVehicle/model3/Mirrors/RightMirror_model3.RightMirror_model3'"));
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
        TEXT("SoundWave'/Game/DReyeVR/Sounds/GearShift.GearShift'"));
    GearShiftSound = CreateDefaultSubobject<UAudioComponent>(TEXT("GearShift"));
    GearShiftSound->SetupAttachment(GetRootComponent());
    GearShiftSound->bAutoActivate = false;
    GearShiftSound->SetSound(GearSound.Object);

    static ConstructorHelpers::FObjectFinder<USoundWave> TurnSignalSoundWave(
        TEXT("SoundWave'/Game/DReyeVR/Sounds/TurnSignal.TurnSignal'"));
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
        const auto &ReplayInputs = Replay->GetUserInputs();
        if (ReplayInputs.ToggledReverse)
            PressReverse();
        else
            ReleaseReverse();

        if (bEnableTurnSignalAction)
        {
            if (ReplayInputs.TurnSignalLeft)
                PressTurnSignalL();
            else
                ReleaseTurnSignalL();

            if (ReplayInputs.TurnSignalRight)
                PressTurnSignalR();
            else
                ReleaseTurnSignalR();
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
        float Now = GetWorld()->GetTimeSeconds();
        const float StartTime = std::max(RightSignalTimeToDie, LeftSignalTimeToDie) - TurnSignalDuration;
        FString TurnSignalStr = "";
        constexpr static float TurnSignalBlinkRate = 0.4f; // rate of blinking
        if (std::fmodf(Now - StartTime, TurnSignalBlinkRate * 2) < TurnSignalBlinkRate)
        {
            if (Now < RightSignalTimeToDie)
                TurnSignalStr = ">>>";
            else if (Now < LeftSignalTimeToDie)
                TurnSignalStr = "<<<";
        }
        TurnSignals->SetText(FText::FromString(TurnSignalStr));
    }

    // Draw the gear shifter
    if (bReverse) // backwards
        GearShifter->SetText(FText::FromString("R"));
    else
        GearShifter->SetText(FText::FromString("D"));
}

/// ========================================== ///
/// -----------------:WHEEL:------------------ ///
/// ========================================== ///

void AEgoVehicle::ConstructSteeringWheel()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SteeringWheelSM(TEXT(
        "StaticMesh'/Game/DReyeVR/EgoVehicle/model3/SteeringWheel/Wheel_StaticMeshl_model3.Wheel_StaticMeshl_model3'"));
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
        float WheelAngleDeg = GetWheelSteerAngle(EVehicleWheelLocation::Front_Wheel);
        // float MaxWheelAngle = GetMaximumSteerAngle();
        float DeltaAngle = 10.f * (2.0f * WheelAngleDeg - CurrentRotation.Roll);

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

void AEgoVehicle::SetGame(ADReyeVRGameMode *Game)
{
    DReyeVRGame = Game;
    check(DReyeVRGame != nullptr);
    DReyeVRGame->SetEgoVehicle(this);

    DReyeVRGame->GetPawn()->BeginEgoVehicle(this, World);
    LOG("Successfully assigned GameMode & controller pawn");
}

ADReyeVRGameMode *AEgoVehicle::GetGame()
{
    return DReyeVRGame;
}

void AEgoVehicle::TickGame(float DeltaSeconds)
{
    if (this->DReyeVRGame != nullptr)
        DReyeVRGame->Tick(DeltaSeconds);
}

/// ========================================== ///
/// ---------------:COSMETIC:----------------- ///
/// ========================================== ///

void AEgoVehicle::DebugLines() const
{
#if WITH_EDITOR

    if (bDrawDebugEditor)
    {
        // Calculate gaze data (in world space) using eye tracker data
        const DReyeVR::AggregateData *Data = EgoSensor->GetData();
        // Compute World positions and orientations
        const FRotator &WorldRot = FirstPersonCam->GetComponentRotation();
        const FVector &WorldPos = FirstPersonCam->GetComponentLocation();

        // First get the gaze origin and direction and vergence from the EyeTracker Sensor
        const float RayLength = FMath::Max(1.f, Data->GetGazeVergence() / 100.f); // vergence to m (from cm)
        const float VRMeterScale = 100.f;

        // Both eyes
        const FVector CombinedGaze = RayLength * VRMeterScale * Data->GetGazeDir();
        const FVector CombinedOrigin = WorldPos + WorldRot.RotateVector(Data->GetGazeOrigin());

        // Left eye
        const FVector LeftGaze = RayLength * VRMeterScale * Data->GetGazeDir(DReyeVR::Gaze::LEFT);
        const FVector LeftOrigin = WorldPos + WorldRot.RotateVector(Data->GetGazeOrigin(DReyeVR::Gaze::LEFT));

        // Right eye
        const FVector RightGaze = RayLength * VRMeterScale * Data->GetGazeDir(DReyeVR::Gaze::RIGHT);
        const FVector RightOrigin = WorldPos + WorldRot.RotateVector(Data->GetGazeOrigin(DReyeVR::Gaze::RIGHT));

        // Use Absolute Ray Position to draw debug information
        // Rotate and add the gaze ray to the origin
        DrawDebugSphere(World, CombinedOrigin + WorldRot.RotateVector(CombinedGaze), 4.0f, 12, FColor::Blue);

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