#include "EgoVehicle.h"
#include "Carla/Actor/ActorAttribute.h"             // FActorAttribute
#include "Carla/Actor/ActorRegistry.h"              // Register
#include "Carla/Game/CarlaStatics.h"                // GetEpisode
#include "Carla/Vehicle/CarlaWheeledVehicleState.h" // ECarlaWheeledVehicleState
#include "DrawDebugHelpers.h"                       // Debug Line/Sphere
#include "Engine/EngineTypes.h"                     // EBlendMode
#include "Engine/World.h"                           // GetWorld
#include "GameFramework/Actor.h"                    // Destroy
#include "HeadMountedDisplayFunctionLibrary.h"      // SetTrackingOrigin, GetWorldToMetersScale
#include "HeadMountedDisplayTypes.h"                // ESpectatorScreenMode
#include "Kismet/GameplayStatics.h"                 // GetPlayerController
#include "Kismet/KismetSystemLibrary.h"             // PrintString, QuitGame
#include "Math/Rotator.h"                           // RotateVector, Clamp
#include "Math/UnrealMathUtility.h"                 // Clamp

#include <algorithm>

// Sets default values
AEgoVehicle::AEgoVehicle(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
    ReadConfigVariables();

    // Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;

    // Set this pawn to be controlled by first (only) player
    AutoPossessPlayer = EAutoReceiveInput::Player0;

    // Set up the root position to be the this mesh
    SetRootComponent(GetMesh());

    // Initialize the camera components
    ConstructCamera();

    // Initialize audio components
    ConstructEgoSounds();

    // Initialize collision event functions
    ConstructCollisionHandler();

    // Initialize text render components
    ConstructDashText();
}

void AEgoVehicle::ReadConfigVariables()
{
    ReadConfigValue("EgoVehicle", "CameraInit", CameraLocnInVehicle);
    ReadConfigValue("EgoVehicle", "DashLocation", DashboardLocnInVehicle);
    ReadConfigValue("EgoVehicle", "SpeedometerInMPH", bUseMPH);
    ReadConfigValue("EgoVehicle", "TurnSignalDuration", TurnSignalDuration);
    // camera
    ReadConfigValue("EgoVehicle", "FieldOfView", FieldOfView);
    // other/cosmetic
    ReadConfigValue("EgoVehicle", "ActorRegistryID", EgoVehicleID);
    ReadConfigValue("EgoVehicle", "DrawDebugEditor", bDrawDebugEditor);
    // HUD (Head's Up Display)
    ReadConfigValue("EgoVehicleHUD", "DrawFPSCounter", bDrawFPSCounter);
    ReadConfigValue("EgoVehicleHUD", "DrawFlatReticle", bDrawFlatReticle);
    ReadConfigValue("EgoVehicleHUD", "ReticleSize", ReticleSize);
    ReadConfigValue("EgoVehicleHUD", "DrawGaze", bDrawGaze);
    ReadConfigValue("EgoVehicleHUD", "DrawSpectatorReticle", bDrawSpectatorReticle);
    ReadConfigValue("EgoVehicleHUD", "EnableSpectatorScreen", bEnableSpectatorScreen);
    // inputs
    ReadConfigValue("VehicleInputs", "ScaleSteeringDamping", ScaleSteeringInput);
    ReadConfigValue("VehicleInputs", "ScaleThrottleInput", ScaleThrottleInput);
    ReadConfigValue("VehicleInputs", "ScaleBrakeInput", ScaleBrakeInput);
    ReadConfigValue("VehicleInputs", "InvertMouseY", InvertMouseY);
    ReadConfigValue("VehicleInputs", "ScaleMouseY", ScaleMouseY);
    ReadConfigValue("VehicleInputs", "ScaleMouseX", ScaleMouseX);
}

void AEgoVehicle::BeginPlay()
{
    // Called when the game starts or when spawned
    Super::BeginPlay();

    // Get information about the world
    World = GetWorld();
    Player = UGameplayStatics::GetPlayerController(World, 0); // main player (0) controller
    Episode = UCarlaStatics::GetCurrentEpisode(World);

    // Setup the HUD
    InitFlatHUD();

    // Get information about the VR headset & initialize SteamVR
    InitSteamVR();

    // Spawn and attach the EgoSensor
    InitSensor();

    // Enable VR spectator screen & eye reticle
    InitSpectator();

    // Initialize logitech steering wheel
    InitLogiWheel();

    // Bug-workaround for initial delay on throttle; see https://github.com/carla-simulator/carla/issues/1640
    this->GetVehicleMovementComponent()->SetTargetGear(1, true);

    // Register Ego Vehicle with ActorRegistry
    Register();

    UE_LOG(LogTemp, Log, TEXT("Initialized DReyeVR EgoVehicle"));
}

void AEgoVehicle::BeginDestroy()
{
    // destroy all spawned entities
    if (EgoSensor)
        EgoSensor->Destroy();

    Super::BeginDestroy();
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

    // Draw the flat-screen HUD items like eye-reticle and FPS counter
    DrawFlatHUD(DeltaSeconds);

    // Draw the spectator vr screen and overlay elements
    DrawSpectatorScreen();

    // Update the world level
    TickLevel(DeltaSeconds);

    // Tick the logitech wheel
    TickLogiWheel();

    // Finish the EgoVehicle tick
    FinishTick();
}

/// ========================================== ///
/// ----------------:CAMERA:------------------ ///
/// ========================================== ///

void AEgoVehicle::InitSteamVR()
{
    bIsHMDConnected = UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled();
    if (bIsHMDConnected)
    {
        UE_LOG(LogTemp, Log, TEXT("Head mounted device detected!"));
        // Now we'll begin with setting up the VR Origin logic
        UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Eye); // Also have Floor & Stage Level
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No head mounted device detected!"));
    }
}

void AEgoVehicle::ConstructCamera()
{
    // Spawn the RootComponent and Camera for the VR camera
    VRCameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRCameraRoot"));
    VRCameraRoot->SetupAttachment(GetRootComponent());      // The vehicle blueprint itself
    VRCameraRoot->SetRelativeLocation(CameraLocnInVehicle); // Offset from center of camera

    // Create a camera and attach to root component
    FirstPersonCam = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCam"));
    FirstPersonCam->SetupAttachment(VRCameraRoot);
    FirstPersonCam->bUsePawnControlRotation = false; // free for VR movement
    FirstPersonCam->bLockToHmd = true;               // lock orientation and position to HMD
    FirstPersonCam->FieldOfView = FieldOfView;       // editable
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
    return CameraLocnInVehicle;
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
}

void AEgoVehicle::ReplayTick()
{
    // perform all sensor updates that occur when replaying
    if (EgoSensor->IsReplaying())
    {
        // this gets reached when the simulator is replaying data from a carla log
        const DReyeVR::AggregateData *Replay = EgoSensor->GetData();

        // include positional update here, else there is lag/jitter between the camera and the vehicle
        // since the Carla Replayer tick differs from the EgoVehicle tick
        const FTransform ReplayTransform(Replay->GetVehicleRotation(), // FRotator (Rotation)
                                         Replay->GetVehicleLocation(), // FVector (Location)
                                         FVector::OneVector);          // FVector (Scale3D)
        SetActorTransform(ReplayTransform, false, nullptr, ETeleportType::None);

        // assign first person camera orientation and location
        FirstPersonCam->SetRelativeRotation(Replay->GetCameraRotation(), false, nullptr, ETeleportType::None);
        FirstPersonCam->SetRelativeLocation(Replay->GetCameraLocation(), false, nullptr, ETeleportType::None);
    }
}

void AEgoVehicle::UpdateSensor(const float DeltaSeconds)
{
    // Calculate gaze data (in world space) using eye tracker data
    const DReyeVR::AggregateData *Data = EgoSensor->GetData();
    // Compute World positions and orientations
    const FRotator WorldRot = FirstPersonCam->GetComponentRotation();
    const FVector WorldPos = FirstPersonCam->GetComponentLocation();

    // First get the gaze origin and direction and vergence from the EyeTracker Sensor
    const float RayLength = FMath::Max(1.f, Data->GetGazeVergence() / 100.f); // vergence to m (from cm)
    const float VRMeterScale = UHeadMountedDisplayFunctionLibrary::GetWorldToMetersScale(World);

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
/// ----------------:SOUNDS:------------------ ///
/// ========================================== ///

void AEgoVehicle::ConstructEgoSounds()
{
    // retarget the engine cue
    static ConstructorHelpers::FObjectFinder<USoundCue> EgoEngineCue(
        TEXT("SoundCue'/Game/Carla/Blueprints/Vehicles/DReyeVR/Sounds/EgoEngineRev.EgoEngineRev'"));
    EngineRevSound->bAutoActivate = true;          // start playing on begin
    EngineRevSound->SetSound(EgoEngineCue.Object); // using this sound

    // Initialize ego-centric audio components
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

    static ConstructorHelpers::FObjectFinder<USoundWave> CarCrashSound(
        TEXT("SoundWave'/Game/Carla/Blueprints/Vehicles/DReyeVR/Sounds/Crash.Crash'"));
    CrashSound = CreateDefaultSubobject<UAudioComponent>(TEXT("CarCrash"));
    CrashSound->SetupAttachment(GetRootComponent());
    CrashSound->bAutoActivate = false;
    CrashSound->SetSound(CarCrashSound.Object);
}

void AEgoVehicle::TickSounds()
{
    // parent (ACarlaWheeledVehicle) contains the EngineRev logic
    Super::TickSounds();
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

void AEgoVehicle::PlayCrashSound(const float DelayBeforePlay) const
{
    if (this->CrashSound)
        this->CrashSound->Play(DelayBeforePlay);
}

void AEgoVehicle::SetVolume(const float VolumeIn)
{
    if (GearShiftSound)
        GearShiftSound->SetVolumeMultiplier(VolumeIn);
    if (TurnSignalSound)
        TurnSignalSound->SetVolumeMultiplier(VolumeIn);
    if (CrashSound)
        CrashSound->SetVolumeMultiplier(VolumeIn);
    Super::SetVolume(VolumeIn);
}

/// ========================================== ///
/// ---------------:COLLISIONS:--------------- ///
/// ========================================== ///

void AEgoVehicle::ConstructCollisionHandler()
{
    // using Carla's GetVehicleBoundingBox function
    UBoxComponent *Bounds = this->GetVehicleBoundingBox();
    Bounds->SetGenerateOverlapEvents(true);
    Bounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Bounds->SetCollisionProfileName(TEXT("Trigger"));
    Bounds->OnComponentBeginOverlap.AddDynamic(this, &AEgoVehicle::OnOverlapBegin);
}

void AEgoVehicle::OnOverlapBegin(UPrimitiveComponent *OverlappedComp, AActor *OtherActor,
                                 UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                 const FHitResult &SweepResult)
{
    if (OtherActor != nullptr && OtherActor != this)
    {
        FString actor_name = OtherActor->GetName();
        UE_LOG(LogTemp, Log, TEXT("Collision with \"%s\""), *actor_name);
        // can be more flexible, such as having collisions with static props or people too
        if (OtherActor->IsA(ACarlaWheeledVehicle::StaticClass()))
        {
            // emit the car collision sound at the midpoint between the vehicles' collision
            const FVector Location = (OtherActor->GetActorLocation() - GetActorLocation()) / 2.f;
            // const FVector Location = OtherActor->GetActorLocation(); // more pronounced spacial audio
            const FRotator Rotation(0.f, 0.f, 0.f);
            const float VolMult = 1.f; //((OtherActor->GetVelocity() - GetVelocity()) / 40.f).Size();
            const float PitchMult = 1.f;
            const float SoundStartTime = 0.f; // how far in to the sound to begin playback
            // "fire and forget" sound function
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), CrashSound->Sound, Location, Rotation, VolMult, PitchMult,
                                                  SoundStartTime, CrashSound->AttenuationSettings, nullptr, this);
        }
    }
}

/// ========================================== ///
/// ---------------:SPECTATOR:---------------- ///
/// ========================================== ///

void AEgoVehicle::InitSpectator()
{
    if (bIsHMDConnected)
    {
        if (bEnableSpectatorScreen)
        {
            InitReticleTexture(); // generate array of pixel values
            check(ReticleTexture);
            UHeadMountedDisplayFunctionLibrary::SetSpectatorScreenMode(ESpectatorScreenMode::TexturePlusEye);
            UHeadMountedDisplayFunctionLibrary::SetSpectatorScreenTexture(ReticleTexture);
        }
        else
        {
            UHeadMountedDisplayFunctionLibrary::SetSpectatorScreenMode(ESpectatorScreenMode::Disabled);
        }
    }
}

void AEgoVehicle::InitReticleTexture()
{

    /// NOTE: need to create transient like this bc of a UE4 bug in release mode
    // https://forums.unrealengine.com/development-discussion/rendering/1767838-fimageutils-createtexture2d-crashes-in-packaged-build
    TArray<FColor> ReticleSrc; // pixel values array for eye reticle texture
    if (bRectangularReticle)
    {
        GenerateSquareImage(ReticleSrc, ReticleSize, FColor(255, 0, 0, 128));
    }
    else
    {
        GenerateCrosshairImage(ReticleSrc, ReticleSize, FColor(255, 0, 0, 128));
    }
    ReticleTexture = UTexture2D::CreateTransient(ReticleSize, ReticleSize, PF_B8G8R8A8);
    void *TextureData = ReticleTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(TextureData, ReticleSrc.GetData(), 4 * ReticleSize * ReticleSize);
    ReticleTexture->PlatformData->Mips[0].BulkData.Unlock();
    ReticleTexture->UpdateResource();
    // ReticleTexture = FImageUtils::CreateTexture2D(ReticleSize, ReticleSize, ReticleSrc, GetWorld(),
    //                                               "EyeReticleTexture", EObjectFlags::RF_Transient, params);

    check(ReticleTexture);
    check(ReticleTexture->Resource);
}

void AEgoVehicle::DrawSpectatorScreen()
{
    if (!bEnableSpectatorScreen || Player == nullptr || !bIsHMDConnected)
        return;
    // calculate View size (of open window). Note this is not the same as resolution
    FIntPoint ViewSize;
    Player->GetViewportSize(ViewSize.X, ViewSize.Y);
    // Get eye tracker variables
    const FRotator WorldRot = GetCamera()->GetComponentRotation();
    const FVector CombinedGazePosn = CombinedOrigin + WorldRot.RotateVector(this->CombinedGaze);

    /// TODO: draw other things on the spectator screen?
    if (bDrawSpectatorReticle)
    {
        /// NOTE: this is the better way to get the ViewportSize
        FVector2D ReticlePos;
        UGameplayStatics::ProjectWorldToScreen(Player, CombinedGazePosn, ReticlePos, true);
        /// NOTE: the SetSpectatorScreenModeTexturePlusEyeLayout expects normalized positions on the screen
        /// NOTE: to get the best drawing, the texture is offset slightly by this vector
        const FVector2D ScreenOffset(ReticleSize * 0.5f, -ReticleSize);
        ReticlePos += ScreenOffset; // move X right by Dim.X/2, move Y up by Dim.Y
        // define min and max bounds
        FVector2D TextureRectMin(FMath::Clamp(ReticlePos.X / ViewSize.X, 0.f, 1.f),
                                 FMath::Clamp(ReticlePos.Y / ViewSize.Y, 0.f, 1.f));
        // max needs to define the bottom right corner, so needs to be +Dim.X right, and +Dim.Y down
        FVector2D TextureRectMax(FMath::Clamp((ReticlePos.X + ReticleSize) / ViewSize.X, 0.f, 1.f),
                                 FMath::Clamp((ReticlePos.Y + ReticleSize) / ViewSize.Y, 0.f, 1.f));
        UHeadMountedDisplayFunctionLibrary::SetSpectatorScreenModeTexturePlusEyeLayout(
            FVector2D{0.f, 0.f}, // whole window (top left)
            FVector2D{1.f, 1.f}, // whole window (top ->*bottom? right)
            TextureRectMin,      // top left of texture
            TextureRectMax,      // bottom right of texture
            true,                // draw eye data as background
            false,               // clear w/ black
            true                 // use alpha
        );
    }
}

/// ========================================== ///
/// ----------------:FLATHUD:----------------- ///
/// ========================================== ///

void AEgoVehicle::InitFlatHUD()
{
    check(Player);
    AHUD *Raw_HUD = Player->GetHUD();
    FlatHUD = Cast<ADReyeVRHUD>(Raw_HUD);
    if (FlatHUD)
    {
        FlatHUD->SetPlayer(Player);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Unable to initialize DReyeVR HUD!"));
    }
}

void AEgoVehicle::DrawFlatHUD(float DeltaSeconds)
{
    if (FlatHUD == nullptr || Player == nullptr)
        return;
    // calculate View size (of open window). Note this is not the same as resolution
    FIntPoint ViewSize;
    Player->GetViewportSize(ViewSize.X, ViewSize.Y);
    // Get eye tracker variables
    const FRotator WorldRot = GetCamera()->GetComponentRotation();
    const FVector CombinedGazePosn = CombinedOrigin + WorldRot.RotateVector(this->CombinedGaze);
    // Draw elements of the HUD
    if (bDrawFlatReticle) // Draw reticle on flat-screen HUD
    {
        const float Diameter = ReticleSize;
        const float Thickness = (ReticleSize / 2.f) / 10.f; // 10 % of radius
        if (bRectangularReticle)
        {
            FlatHUD->DrawDynamicSquare(CombinedGazePosn, Diameter, FColor(255, 0, 0, 255), Thickness);
        }
        else
        {
            FlatHUD->DrawDynamicCrosshair(CombinedGazePosn, Diameter, FColor(255, 0, 0, 255), true, Thickness);
#if 0
            // many problems here, for some reason the UE4 hud's DrawSimpleTexture function
            // crashes the thread its on by invalidating the ReticleTexture->Resource which is
            // non-const (but should be!!) This has to be a bug in UE4 code that we unfortunately have
            // to work around
            if (!ensure(ReticleTexture) || !ensure(ReticleTexture->Resource))
            {
                InitReticleTexture();
            }
            if (ReticleTexture != nullptr && ReticleTexture->Resource != nullptr)
            {
                FlatHUD->DrawReticle(ReticleTexture, ReticlePos + FVector2D(-ReticleSize * 0.5f, -ReticleSize * 0.5f));
            }
#endif
        }
    }
    if (bDrawFPSCounter)
    {
        FlatHUD->DrawDynamicText(FString::FromInt(int(1.f / DeltaSeconds)), FVector2D(ViewSize.X - 100, 50),
                                 FColor(0, 255, 0, 213), 2);
    }
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
    TurnSignals->SetRelativeLocation(DashboardLocnInVehicle + FVector(0, -40.f, 0));
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
    if (Player == nullptr)
        return;
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
        if (Replay->GetUserInputs().TurnSignalLeft)
        {
            LeftSignalTimeToDie = FPlatformTime::Seconds() + TurnSignalDuration;
            PlayTurnSignalSound();
        }
        if (Replay->GetUserInputs().TurnSignalRight)
        {
            RightSignalTimeToDie = FPlatformTime::Seconds() + TurnSignalDuration;
            PlayTurnSignalSound();
        }
    }
    else
    {
        XPH = GetVehicleForwardSpeed() * SpeedometerScale; // FwdSpeed is in cm/s
    }

    const FString Data = FString::FromInt(int(FMath::RoundHalfFromZero(XPH)));
    Speedometer->SetText(FText::FromString(Data));

    // Draw the signals
    float Now = FPlatformTime::Seconds();
    if (Now < RightSignalTimeToDie)
        TurnSignals->SetText(FText::FromString(">>>"));
    else if (Now < LeftSignalTimeToDie)
        TurnSignals->SetText(FText::FromString("<<<"));
    else
        TurnSignals->SetText(FText::FromString("")); // nothing

    // Draw the gear shifter
    if (bReverse)
        GearShifter->SetText(FText::FromString("R"));
    else
        GearShifter->SetText(FText::FromString("D"));
}

/// ========================================== ///
/// -----------------:LEVEL:------------------ ///
/// ========================================== ///

void AEgoVehicle::SetLevel(ADReyeVRLevel *Level)
{
    this->DReyeVRLevel = Level;
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
    FActorView::IdType ID = EgoVehicleID;
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

void AEgoVehicle::FinishTick()
{
    VehicleInputs = {}; // clear inputs to be updated on the next tick
}

/// ========================================== ///
/// ---------------:COSMETIC:----------------- ///
/// ========================================== ///

void AEgoVehicle::DebugLines() const
{
    // Compute World positions and orientations
    const FRotator WorldRot = FirstPersonCam->GetComponentRotation();

#if WITH_EDITOR
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
    if (bDrawGaze && FlatHUD != nullptr)
    {
        // Draw line components in FlatHUD
        FlatHUD->DrawDynamicLine(CombinedOrigin, CombinedOrigin + 10.f * WorldRot.RotateVector(CombinedGaze),
                                 FColor::Red, 3.0f);
    }
}