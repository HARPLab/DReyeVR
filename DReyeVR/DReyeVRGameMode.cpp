#include "DReyeVRGameMode.h"
#include "Carla/AI/AIControllerFactory.h"      // AAIControllerFactory
#include "Carla/Actor/StaticMeshFactory.h"     // AStaticMeshFactory
#include "Carla/Game/CarlaStatics.h"           // GetReplayer, GetEpisode
#include "Carla/Recorder/CarlaReplayer.h"      // ACarlaReplayer
#include "Carla/Sensor/DReyeVRSensor.h"        // ADReyeVRSensor
#include "Carla/Sensor/SensorFactory.h"        // ASensorFactory
#include "Carla/Trigger/TriggerFactory.h"      // TriggerFactory
#include "Carla/Vehicle/CarlaWheeledVehicle.h" // ACarlaWheeledVehicle
#include "Carla/Weather/Weather.h"             // AWeather
#include "Components/AudioComponent.h"         // UAudioComponent
#include "DReyeVRFactory.h"                    // ADReyeVRFactory
#include "DReyeVRPawn.h"                       // ADReyeVRPawn
#include "DReyeVRUtils.h"                      // FindDefnInRegistry
#include "EgoVehicle.h"                        // AEgoVehicle
#include "FlatHUD.h"                           // ADReyeVRHUD
#include "HeadMountedDisplayFunctionLibrary.h" // IsHeadMountedDisplayAvailable
#include "Kismet/GameplayStatics.h"            // GetPlayerController
#include "UObject/UObjectIterator.h"           // TObjectInterator

ADReyeVRGameMode::ADReyeVRGameMode(FObjectInitializer const &FO) : Super(FO)
{
    // initialize stuff here
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bStartWithTickEnabled = false;

    // initialize default classes
    this->HUDClass = ADReyeVRHUD::StaticClass();
    // find object UClass rather than UBlueprint
    // https://forums.unrealengine.com/t/cdo-constructor-failed-to-find-thirdperson-c-template-mannequin-animbp/99003
    static ConstructorHelpers::FObjectFinder<UClass> WeatherBP(
        TEXT("/Game/Carla/Blueprints/Weather/BP_Weather.BP_Weather_C"));
    this->WeatherClass = WeatherBP.Object;

    // initialize actor factories
    // https://forums.unrealengine.com/t/what-is-the-right-syntax-of-fclassfinder-and-how-could-i-generaly-use-it-to-find-a-specific-blueprint/363884
    static ConstructorHelpers::FClassFinder<ACarlaActorFactory> VehicleFactoryBP(
        TEXT("Blueprint'/Game/Carla/Blueprints/Vehicles/VehicleFactory'"));
    static ConstructorHelpers::FClassFinder<ACarlaActorFactory> WalkerFactoryBP(
        TEXT("Blueprint'/Game/Carla/Blueprints/Walkers/WalkerFactory'"));
    static ConstructorHelpers::FClassFinder<ACarlaActorFactory> PropFactoryBP(
        TEXT("Blueprint'/Game/Carla/Blueprints/Props/PropFactory'"));

    this->ActorFactories = TSet<TSubclassOf<ACarlaActorFactory>>{
        VehicleFactoryBP.Class,
        ASensorFactory::StaticClass(),
        WalkerFactoryBP.Class,
        PropFactoryBP.Class,
        ATriggerFactory::StaticClass(),
        AAIControllerFactory::StaticClass(),
        AStaticMeshFactory::StaticClass(),
        ADReyeVRFactory::StaticClass(), // this is what registers the DReyeVR actors
    };

    // read config variables
    ReadConfigValue("Game", "AutomaticallySpawnEgo", bDoSpawnEgoVehicle);
    ReadConfigValue("Game", "EgoVolumePercent", EgoVolumePercent);
    ReadConfigValue("Game", "NonEgoVolumePercent", NonEgoVolumePercent);
    ReadConfigValue("Game", "AmbientVolumePercent", AmbientVolumePercent);
    ReadConfigValue("Game", "DoSpawnEgoVehicleTransform", bDoSpawnEgoVehicleTransform);
    ReadConfigValue("Game", "SpawnEgoVehicleTransform", SpawnEgoVehicleTransform);

    // Recorder/replayer
    ReadConfigValue("Replayer", "UseCarlaSpectator", bUseCarlaSpectator);
    bool bEnableReplayInterpolation = false;
    ReadConfigValue("Replayer", "ReplayInterpolation", bEnableReplayInterpolation);
    bReplaySync = !bEnableReplayInterpolation; // synchronous => no interpolation!
}

void ADReyeVRGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Initialize player
    Player = UGameplayStatics::GetPlayerController(GetWorld(), 0);

    // Can we tick?
    SetActorTickEnabled(false); // make sure we do not tick ourselves

    // set all the volumes (ego, non-ego, ambient/world)
    SetVolume();

    // start input mapping
    SetupPlayerInputComponent();

    // spawn the DReyeVR pawn and possess it (first)
    SetupDReyeVRPawn();

    // Initialize the DReyeVR EgoVehicle and Sensor (second)
    SetupEgoVehicle();

    // Initialize DReyeVR spectator (third)
    SetupSpectator();
}

void ADReyeVRGameMode::SetupDReyeVRPawn()
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    DReyeVR_Pawn = GetWorld()->SpawnActor<ADReyeVRPawn>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    /// NOTE: the pawn is automatically possessed by player0
    // as the constructor has the AutoPossessPlayer != disabled
    // if you want to manually possess then you can do Player->Possess(DReyeVR_Pawn);
    if (DReyeVR_Pawn == nullptr)
    {
        LOG_ERROR("Unable to spawn DReyeVR pawn!")
    }
    else
    {
        DReyeVR_Pawn->BeginPlayer(Player);
        LOG("Successfully spawned DReyeVR pawn");
    }
}

bool ADReyeVRGameMode::SetupEgoVehicle()
{
    if (EgoVehiclePtr != nullptr || bDoSpawnEgoVehicle == false)
    {
        LOG("Not spawning new EgoVehicle");
        if (EgoVehiclePtr == nullptr)
        {
            LOG("EgoVehicle unavailable, possessing spectator by default")
            PossessSpectator(); // NOTE: need to call SetupSpectator before this!
        }
        return true;
    }
    ensure(DReyeVR_Pawn);

    TArray<AActor *> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEgoVehicle::StaticClass(), FoundActors);
    if (FoundActors.Num() > 0)
    {
        for (AActor *Vehicle : FoundActors)
        {
            LOG("Found EgoVehicle in world: %s", *(Vehicle->GetName()));
            EgoVehiclePtr = CastChecked<AEgoVehicle>(Vehicle);
            /// TODO: handle multiple ego-vehcles? (we should only ever have one!)
            break;
        }
    }
    else
    {
        LOG("Did not find EgoVehicle in map... spawning...");
        // use the provided transform if requested, else generate a spawn point
        FTransform SpawnPt = bDoSpawnEgoVehicleTransform ? SpawnEgoVehicleTransform : GetSpawnPoint();
        SpawnEgoVehicle(SpawnPt); // constructs and assigns EgoVehiclePtr
    }

    // finalize the EgoVehicle by installing the DReyeVR_Pawn to control it
    check(EgoVehiclePtr != nullptr);
    return (EgoVehiclePtr != nullptr);
}

void ADReyeVRGameMode::SetupSpectator()
{
    if (bUseCarlaSpectator)
    { // look for existing spectator in world
        UCarlaEpisode *Episode = UCarlaStatics::GetCurrentEpisode(GetWorld());
        if (Episode != nullptr)
            SpectatorPtr = Episode->GetSpectatorPawn();
        else if (Player != nullptr)
        {
            SpectatorPtr = Player->GetPawn();
        }
    }

    // spawn if necessary
    if (SpectatorPtr != nullptr)
    {
        LOG("Found available spectator in world");
    }
    else
    {
        LOG("Spawning DReyeVR Spectator Pawn in the world");
        FVector SpawnLocn;
        FRotator SpawnRotn;
        if (EgoVehiclePtr != nullptr)
        {
            SpawnLocn = EgoVehiclePtr->GetCameraPosn();
            SpawnRotn = EgoVehiclePtr->GetCameraRot();
        }
        // create new spectator pawn
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        SpawnParams.ObjectFlags |= RF_Transient;
        SpectatorPtr = GetWorld()->SpawnActor<ASpectatorPawn>(ASpectatorPawn::StaticClass(), // spectator
                                                              SpawnLocn, SpawnRotn, SpawnParams);
    }

    if (SpectatorPtr)
    {
        SpectatorPtr->SetActorHiddenInGame(true);                // make spectator invisible
        SpectatorPtr->GetRootComponent()->DestroyPhysicsState(); // no physics (just no-clip)
        SpectatorPtr->SetActorEnableCollision(false);            // no collisions
        LOG("Successfully initiated spectator actor");
    }
}

void ADReyeVRGameMode::BeginDestroy()
{
    Super::BeginDestroy();
    LOG("Finished Game");
}

void ADReyeVRGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    /// TODO: clean up replay init
    if (!bRecorderInitiated) // can't do this in constructor
    {
        // Initialize recorder/replayer
        SetupReplayer(); // once this is successfully run, it no longer gets executed
    }

    DrawBBoxes();
}

void ADReyeVRGameMode::SetupPlayerInputComponent()
{
    InputComponent = NewObject<UInputComponent>(this);
    InputComponent->RegisterComponent();
    // set up gameplay key bindings
    check(InputComponent);
    Player->PushInputComponent(InputComponent); // enable this InputComponent with the PlayerController
    // InputComponent->BindAction("ToggleCamera", IE_Pressed, this, &ADReyeVRGameMode::ToggleSpectator);
    InputComponent->BindAction("PlayPause_DReyeVR", IE_Pressed, this, &ADReyeVRGameMode::ReplayPlayPause);
    InputComponent->BindAction("FastForward_DReyeVR", IE_Pressed, this, &ADReyeVRGameMode::ReplayFastForward);
    InputComponent->BindAction("Rewind_DReyeVR", IE_Pressed, this, &ADReyeVRGameMode::ReplayRewind);
    InputComponent->BindAction("Restart_DReyeVR", IE_Pressed, this, &ADReyeVRGameMode::ReplayRestart);
    InputComponent->BindAction("Incr_Timestep_DReyeVR", IE_Pressed, this, &ADReyeVRGameMode::ReplaySpeedUp);
    InputComponent->BindAction("Decr_Timestep_DReyeVR", IE_Pressed, this, &ADReyeVRGameMode::ReplaySlowDown);
    // Driver Handoff examples
    InputComponent->BindAction("EgoVehicle_DReyeVR", IE_Pressed, this, &ADReyeVRGameMode::PossessEgoVehicle);
    InputComponent->BindAction("Spectator_DReyeVR", IE_Pressed, this, &ADReyeVRGameMode::PossessSpectator);
    InputComponent->BindAction("AI_DReyeVR", IE_Pressed, this, &ADReyeVRGameMode::HandoffDriverToAI);
}

void ADReyeVRGameMode::PossessEgoVehicle()
{
    if (EgoVehiclePtr == nullptr)
    {
        LOG_WARN("No EgoVehicle to possess! Attempting to remedy...");
        SetupEgoVehicle();
        if (EgoVehiclePtr == nullptr)
        {
            LOG_ERROR("Remedy failed, unable to possess EgoVehicle");
            return;
        }
    }
    EgoVehiclePtr->SetAutopilot(false);

    if (DReyeVR_Pawn == nullptr)
    {
        LOG_WARN("No DReyeVR pawn to possess EgoVehicle! Attempting to remedy...");
        SetupDReyeVRPawn();
        if (DReyeVR_Pawn == nullptr)
        {
            LOG_ERROR("Remedy failed, unable to possess EgoVehicle");
            return;
        }
        return;
    }

    { // check if already possessing EgoVehicle (DReyeVRPawn)
        ensure(Player != nullptr);
        if (Player->GetPawn() == DReyeVR_Pawn)
        {
            LOG("Currently possessing EgoVehicle");
            return;
        }
    }

    LOG("Possessing DReyeVR EgoVehicle");
    Player->Possess(DReyeVR_Pawn);
}

void ADReyeVRGameMode::PossessSpectator()
{
    if (SpectatorPtr == nullptr)
    {
        LOG_WARN("No spectator to possess! Attempting to remedy...");
        SetupSpectator();
        if (SpectatorPtr == nullptr)
        {
            LOG_ERROR("Remedy failed, unable to possess spectator");
            return;
        }
    }

    { // check if already possessing spectator
        ensure(Player != nullptr);
        if (Player->GetPawn() == SpectatorPtr)
        {
            LOG("Already possessing Spectator");
            return;
        }
    }

    if (EgoVehiclePtr)
    {
        // spawn from EgoVehicle head position
        const FVector &EgoLocn = EgoVehiclePtr->GetCameraPosn();
        const FRotator &EgoRotn = EgoVehiclePtr->GetCameraRot();
        SpectatorPtr->SetActorLocationAndRotation(EgoLocn, EgoRotn);
    }
    // repossess the ego vehicle
    Player->Possess(SpectatorPtr);
    LOG("Possessing spectator player");
}

void ADReyeVRGameMode::HandoffDriverToAI()
{
    if (EgoVehiclePtr == nullptr)
    {
        LOG_WARN("No EgoVehicle to handoff AI control! Attempting to remedy...");
        SetupEgoVehicle();
        if (EgoVehiclePtr == nullptr)
        {
            LOG_ERROR("Remedy failed, unable to access EgoVehicle");
            return;
        }
    }

    { // check if autopilot already enabled
        if (EgoVehiclePtr->GetAutopilotStatus() == true)
        {
            LOG("EgoVehicle autopilot already on");
            return;
        }
    }
    EgoVehiclePtr->SetAutopilot(true);
    LOG("Enabling EgoVehicle Autopilot");
}

void ADReyeVRGameMode::ReplayPlayPause()
{
    auto *Replayer = UCarlaStatics::GetReplayer(GetWorld());
    if (Replayer != nullptr && Replayer->IsEnabled())
    {
        LOG("Toggle Replayer Play-Pause");
        Replayer->PlayPause();
    }
}

void ADReyeVRGameMode::ReplayFastForward()
{
    auto *Replayer = UCarlaStatics::GetReplayer(GetWorld());
    if (Replayer != nullptr && Replayer->IsEnabled())
    {
        LOG("Advance replay by +1.0s");
        Replayer->Advance(1.0);
    }
}

void ADReyeVRGameMode::ReplayRewind()
{
    auto *Replayer = UCarlaStatics::GetReplayer(GetWorld());
    if (Replayer != nullptr && Replayer->IsEnabled())
    {
        LOG("Advance replay by -1.0s");
        Replayer->Advance(-1.0);
    }
}

void ADReyeVRGameMode::ReplayRestart()
{
    auto *Replayer = UCarlaStatics::GetReplayer(GetWorld());
    if (Replayer != nullptr && Replayer->IsEnabled())
    {
        LOG("Restarting recording replay...");
        Replayer->Restart();
    }
}

void ADReyeVRGameMode::ChangeTimestep(UWorld *World, double AmntChangeSeconds)
{
    if (bReplaySync)
    {
        LOG("Changing timestep of replay during a synchronous replay is not supported. Enable ReplayInterpolation to "
            "achieve this.")
        return;
    }
    ensure(World != nullptr);
    auto *Replayer = UCarlaStatics::GetReplayer(World);
    if (Replayer != nullptr && Replayer->IsEnabled())
    {
        double NewFactor = ReplayTimeFactor + AmntChangeSeconds;
        if (AmntChangeSeconds > 0)
        {
            if (NewFactor < ReplayTimeFactorMax)
            {
                LOG("Increase replay time factor: %.3fx -> %.3fx", ReplayTimeFactor, NewFactor);
                Replayer->SetTimeFactor(NewFactor);
            }
            else
            {
                LOG("Unable to increase replay time factor (%.3f) beyond %.3fx", ReplayTimeFactor, ReplayTimeFactorMax);
                Replayer->SetTimeFactor(ReplayTimeFactorMax);
            }
        }
        else
        {
            if (NewFactor > ReplayTimeFactorMin)
            {
                LOG("Decrease replay time factor: %.3fx -> %.3fx", ReplayTimeFactor, NewFactor);
                Replayer->SetTimeFactor(NewFactor);
            }
            else
            {
                LOG("Unable to decrease replay time factor (%.3f) below %.3fx", ReplayTimeFactor, ReplayTimeFactorMin);
                Replayer->SetTimeFactor(ReplayTimeFactorMin);
            }
        }
        ReplayTimeFactor = FMath::Clamp(NewFactor, ReplayTimeFactorMin, ReplayTimeFactorMax);
    }
}

void ADReyeVRGameMode::ReplaySpeedUp()
{
    ChangeTimestep(GetWorld(), AmntPlaybackIncr);
}

void ADReyeVRGameMode::ReplaySlowDown()
{
    ChangeTimestep(GetWorld(), -AmntPlaybackIncr);
}

void ADReyeVRGameMode::SetupReplayer()
{
    auto *Replayer = UCarlaStatics::GetReplayer(GetWorld());
    if (Replayer != nullptr)
    {
        Replayer->SetSyncMode(bReplaySync);
        if (bReplaySync)
        {
            LOG_WARN("Replay operating in frame-wise (1:1) synchronous mode (no replay interpolation)");
        }
        bRecorderInitiated = true;
    }
}

void ADReyeVRGameMode::DrawBBoxes()
{
#if 0
    TArray<AActor *> FoundActors;
    if (GetWorld() != nullptr)
    {
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACarlaWheeledVehicle::StaticClass(), FoundActors);
    }
    for (AActor *A : FoundActors)
    {
        std::string name = TCHAR_TO_UTF8(*A->GetName());
        if (A->GetName().Contains("DReyeVR"))
            continue; // skip drawing a bbox over the EgoVehicle
        if (BBoxes.find(name) == BBoxes.end())
        {
            BBoxes[name] = ADReyeVRCustomActor::CreateNew(SM_CUBE, MAT_TRANSLUCENT, GetWorld(), "BBox" + A->GetName());
        }
        const float DistThresh = 20.f; // meters before nearby bounding boxes become red
        ADReyeVRCustomActor *BBox = BBoxes[name];
        ensure(BBox != nullptr);
        if (BBox != nullptr)
        {
            BBox->Activate();
            BBox->MaterialParams.Opacity = 0.1f;
            FLinearColor Col = FLinearColor::Green;
            if (FVector::Distance(EgoVehiclePtr->GetActorLocation(), A->GetActorLocation()) < DistThresh * 100.f)
            {
                Col = FLinearColor::Red;
            }
            BBox->MaterialParams.BaseColor = Col;
            BBox->MaterialParams.Emissive = 0.1 * Col;

            FVector Origin;
            FVector BoxExtent;
            A->GetActorBounds(true, Origin, BoxExtent, false);
            // LOG("Origin: %s Extent %s"), *Origin.ToString(), *BoxExtent.ToString());
            // divide by 100 to get from m to cm, multiply by 2 bc the cube is scaled in both X and Y
            BBox->SetActorScale3D(2 * BoxExtent / 100.f);
            BBox->SetActorLocation(Origin);
            // extent already covers the rotation aspect since the bbox is dynamic and axis aligned
            // BBox->SetActorRotation(A->GetActorRotation());
        }
    }
#endif
}

void ADReyeVRGameMode::ReplayCustomActor(const DReyeVR::CustomActorData &RecorderData, const double Per)
{
    // first spawn the actor if not currently active
    const std::string ActorName = TCHAR_TO_UTF8(*RecorderData.Name);
    ADReyeVRCustomActor *A = nullptr;
    if (ADReyeVRCustomActor::ActiveCustomActors.find(ActorName) == ADReyeVRCustomActor::ActiveCustomActors.end())
    {
        /// TODO: also track KnownNumMaterials?
        A = ADReyeVRCustomActor::CreateNew(RecorderData.MeshPath, RecorderData.MaterialParams.MaterialPath, GetWorld(),
                                           RecorderData.Name);
    }
    else
    {
        A = ADReyeVRCustomActor::ActiveCustomActors[ActorName];
    }
    // ensure the actor is currently active (spawned)
    // now that we know this actor exists, update its internals
    if (A != nullptr)
    {
        A->SetInternals(RecorderData);
        A->Activate();
        A->Tick(Per); // update locations immediately
    }
}

void ADReyeVRGameMode::SetVolume()
{
    // update the non-ego volume percent
    ACarlaWheeledVehicle::Volume = NonEgoVolumePercent / 100.f;

    // for all in-world audio components such as ambient birdsong, fountain splashing, smoke, etc.
    for (TObjectIterator<UAudioComponent> Itr; Itr; ++Itr)
    {
        if (Itr->GetWorld() != GetWorld()) // World Check
        {
            continue;
        }
        Itr->SetVolumeMultiplier(AmbientVolumePercent / 100.f);
    }

    // for all in-world vehicles (including the EgoVehicle) manually set their volumes
    TArray<AActor *> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACarlaWheeledVehicle::StaticClass(), FoundActors);
    for (AActor *A : FoundActors)
    {
        ACarlaWheeledVehicle *Vehicle = Cast<ACarlaWheeledVehicle>(A);
        if (Vehicle != nullptr)
        {
            float NewVolume = ACarlaWheeledVehicle::Volume; // Non ego volume
            if (Vehicle->IsA(AEgoVehicle::StaticClass()))   // dynamic cast, requires -frrti
                NewVolume = EgoVolumePercent / 100.f;
            Vehicle->SetVolume(NewVolume);
        }
    }
}

void ADReyeVRGameMode::SpawnEgoVehicle(const FTransform &SpawnPt)
{
    UCarlaEpisode *Episode = UCarlaStatics::GetCurrentEpisode(GetWorld());
    check(Episode != nullptr);
    FActorDefinition EgoVehicleDefn = FindDefnInRegistry(Episode, AEgoVehicle::StaticClass());
    FActorDescription DReyeVRDescr;
    { // create a Description from the Definition to spawn the actor
        DReyeVRDescr.UId = EgoVehicleDefn.UId;
        DReyeVRDescr.Id = EgoVehicleDefn.Id;
        DReyeVRDescr.Class = EgoVehicleDefn.Class;
        // ensure this vehicle is denoted by the 'hero' attribute
        FActorAttribute HeroRole;
        HeroRole.Id = "role_name";
        HeroRole.Type = EActorAttributeType::String;
        HeroRole.Value = "hero";
        DReyeVRDescr.Variations.Add(HeroRole.Id, std::move(HeroRole));
    }
    // calls Episode::SpawnActor => SpawnActorWithInfo => ActorDispatcher->SpawnActor => SpawnFunctions[UId]
    EgoVehiclePtr = static_cast<AEgoVehicle *>(Episode->SpawnActor(SpawnPt, DReyeVRDescr));
}

FTransform ADReyeVRGameMode::GetSpawnPoint(int SpawnPointIndex) const
{
    ACarlaGameModeBase *GM = UCarlaStatics::GetGameMode(GetWorld());
    if (GM != nullptr)
    {
        TArray<FTransform> SpawnPoints = GM->GetSpawnPointsTransforms();
        size_t WhichPoint = 0; // default to first one
        if (SpawnPointIndex < 0)
            WhichPoint = FMath::RandRange(0, SpawnPoints.Num());
        else
            WhichPoint = FMath::Clamp(SpawnPointIndex, 0, SpawnPoints.Num());

        if (WhichPoint < SpawnPoints.Num()) // SpawnPoints could be empty
            return SpawnPoints[WhichPoint];
    }
    /// TODO: return a safe bet (position of the player start maybe?)
    return FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::OneVector);
}