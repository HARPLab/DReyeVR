#include "LevelScript.h"
#include "Carla/Game/CarlaStatics.h"           // UCarlaStatics::GetRecorder
#include "Carla/Sensor/DReyeVRSensor.h"        // ADReyeVRSensor
#include "Carla/Vehicle/CarlaWheeledVehicle.h" // ACarlaWheeledVehicle
#include "Components/AudioComponent.h"         // UAudioComponent
#include "EgoVehicle.h"                        // AEgoVehicle
#include "HeadMountedDisplayFunctionLibrary.h" // IsHeadMountedDisplayAvailable
#include "Kismet/GameplayStatics.h"            // GetPlayerController
#include "UObject/UObjectIterator.h"           // TObjectInterator

ADReyeVRLevel::ADReyeVRLevel(FObjectInitializer const &FO) : Super(FO)
{
    // initialize stuff here
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bStartWithTickEnabled = false;

    ReadConfigValue("Level", "EgoVolumePercent", EgoVolumePercent);
    ReadConfigValue("Level", "NonEgoVolumePercent", NonEgoVolumePercent);
    ReadConfigValue("Level", "AmbientVolumePercent", AmbientVolumePercent);
}

void ADReyeVRLevel::BeginPlay()
{
    Super::ReceiveBeginPlay();

    // Initialize player
    Player = UGameplayStatics::GetPlayerController(GetWorld(), 0);

    // Can we tick?
    SetActorTickEnabled(false); // make sure we do not tick ourselves

    // enable input tracking
    InputEnabled();

    // set all the volumes (ego, non-ego, ambient/world)
    SetVolume();

    // start input mapping
    SetupPlayerInputComponent();

    // Find the ego vehicle in the world
    /// TODO: optionally, spawn ego-vehicle here with parametrized inputs
    FindEgoVehicle();

    // Initialize DReyeVR spectator
    SetupSpectator();

    // Initialize control mode
    /// TODO: read in initial control mode from .ini
    ControlMode = DRIVER::HUMAN;
    switch (ControlMode)
    {
    case (DRIVER::HUMAN):
        PossessEgoVehicle();
        break;
    case (DRIVER::SPECTATOR):
        PossessSpectator();
        break;
    case (DRIVER::AI):
        HandoffDriverToAI();
        break;
    }
}

bool ADReyeVRLevel::FindEgoVehicle()
{
    if (EgoVehiclePtr != nullptr)
        return true;
    TArray<AActor *> FoundEgoVehicles;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEgoVehicle::StaticClass(), FoundEgoVehicles);
    for (AActor *Vehicle : FoundEgoVehicles)
    {
        UE_LOG(LogTemp, Log, TEXT("Found EgoVehicle in world: %s"), *(Vehicle->GetName()));
        EgoVehiclePtr = CastChecked<AEgoVehicle>(Vehicle);
        EgoVehiclePtr->SetLevel(this);
        if (!AI_Player)
            AI_Player = EgoVehiclePtr->GetController();
        /// TODO: handle multiple ego-vehcles? (we should only ever have one!)
        return true;
    }
    UE_LOG(LogTemp, Error, TEXT("Did not find EgoVehicle"));
#if 0
    UE_LOG(LogTemp, Log, TEXT("Did not find EgoVehicle, spawning"));
    FActorSpawnParameters EgoVehicleSpawnInfo;
    EgoVehicleSpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    FTransform Spawn = GetSpawnPoint();
    FSoftObjectPath Ref(
        "Blueprint'/Game/Carla/Blueprints/Vehicles/DReyeVR/BP_EgoVehicle_DReyeVR.BP_EgoVehicle_DReyeVR'");
    UObject *Obj = Ref.ResolveObject();
    UBlueprint *Blueprint = Cast<UBlueprint>(Obj);
    if (Blueprint != nullptr)
    {
        EgoVehiclePtr = GetWorld()->SpawnActor<AEgoVehicle>(Blueprint->GeneratedClass, // TODO: refactor so
                                                            Spawn.GetLocation(),       // that the EgoVehicle class
                                                            Spawn.Rotator(),           // contains its own vehicle mesh
                                                            EgoVehicleSpawnInfo);      // properties without BP
    }
    if (EgoVehiclePtr == nullptr)
        UE_LOG(LogTemp, Error, TEXT("Spawning EgoVehicle failed!"))
#endif
    return (EgoVehiclePtr != nullptr);
}

void ADReyeVRLevel::SetupSpectator()
{
    /// TODO: fix bug where HMD is not detected on package BeginPlay()
    // if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
    const bool bEnableVRSpectator = true;
    if (bEnableVRSpectator)
    {
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
    else
    {
        UCarlaEpisode *Episode = UCarlaStatics::GetCurrentEpisode(GetWorld());
        if (Episode != nullptr)
            SpectatorPtr = Episode->GetSpectatorPawn();
        else
        {
            if (Player != nullptr)
            {
                SpectatorPtr = Player->GetPawn();
            }
        }
    }
}

void ADReyeVRLevel::BeginDestroy()
{
    Super::BeginDestroy();
    UE_LOG(LogTemp, Log, TEXT("Finished Level"));
}

void ADReyeVRLevel::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (EgoVehiclePtr && SpectatorPtr && AI_Player)
    {
        if (ControlMode == DRIVER::AI) // when AI is controlling EgoVehicle
        {
            // Physically attach the Controller to the specified Pawn, so that our position reflects theirs.
            // const FVector &NewVehiclePosn = EgoVehiclePtr->GetNextCameraPosn(DeltaSeconds); // for pre-physics tick
            const FVector &NewVehiclePosn = EgoVehiclePtr->GetCameraPosn(); // for post-physics tick
            SpectatorPtr->SetActorLocationAndRotation(NewVehiclePosn, EgoVehiclePtr->GetCameraRot());
        }
    }
}

void ADReyeVRLevel::SetupPlayerInputComponent()
{
    InputComponent = NewObject<UInputComponent>(this);
    InputComponent->RegisterComponent();
    // set up gameplay key bindings
    check(InputComponent);
    // InputComponent->BindAction("ToggleCamera", IE_Pressed, this, &ADReyeVRLevel::ToggleSpectator);
    InputComponent->BindAction("PlayPause_DReyeVR", IE_Pressed, this, &ADReyeVRLevel::PlayPause);
    InputComponent->BindAction("FastForward_DReyeVR", IE_Pressed, this, &ADReyeVRLevel::FastForward);
    InputComponent->BindAction("Rewind_DReyeVR", IE_Pressed, this, &ADReyeVRLevel::Rewind);
    InputComponent->BindAction("Restart_DReyeVR", IE_Pressed, this, &ADReyeVRLevel::Restart);
    InputComponent->BindAction("Incr_Timestep_DReyeVR", IE_Pressed, this, &ADReyeVRLevel::IncrTimestep);
    InputComponent->BindAction("Decr_Timestep_DReyeVR", IE_Pressed, this, &ADReyeVRLevel::DecrTimestep);
    // Driver Handoff examples
    InputComponent->BindAction("EgoVehicle_DReyeVR", IE_Pressed, this, &ADReyeVRLevel::PossessEgoVehicle);
    InputComponent->BindAction("Spectator_DReyeVR", IE_Pressed, this, &ADReyeVRLevel::PossessSpectator);
    InputComponent->BindAction("AI_DReyeVR", IE_Pressed, this, &ADReyeVRLevel::HandoffDriverToAI);
}

void ADReyeVRLevel::PossessEgoVehicle()
{
    if (!EgoVehiclePtr)
    {
        UE_LOG(LogTemp, Error, TEXT("No EgoVehicle to possess. Searching..."));
        if (!FindEgoVehicle())
            return;
    }
    check(EgoVehiclePtr != nullptr);
    // repossess the ego vehicle
    Player->Possess(EgoVehiclePtr);
    UE_LOG(LogTemp, Log, TEXT("Possessing DReyeVR EgoVehicle"));
    this->ControlMode = DRIVER::HUMAN;
}

void ADReyeVRLevel::PossessSpectator()
{
    // check if already possessing spectator
    if (Player->GetPawn() == SpectatorPtr && ControlMode != DRIVER::AI)
        return;
    if (!SpectatorPtr)
    {
        UE_LOG(LogTemp, Error, TEXT("No spectator to possess"));
        SetupSpectator();
        if (SpectatorPtr == nullptr)
        {
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
    UE_LOG(LogTemp, Log, TEXT("Possessing spectator player"));
    this->ControlMode = DRIVER::SPECTATOR;
}

void ADReyeVRLevel::HandoffDriverToAI()
{
    if (!EgoVehiclePtr)
    {
        UE_LOG(LogTemp, Error, TEXT("No EgoVehicle for AI handoff. Searching... "));
        if (!FindEgoVehicle())
            return;
    }
    if (!AI_Player)
    {
        UE_LOG(LogTemp, Error, TEXT("No EgoVehicle for AI handoff"));
        return;
    }
    check(AI_Player != nullptr);
    PossessSpectator();
    AI_Player->Possess(EgoVehiclePtr);
    UE_LOG(LogTemp, Log, TEXT("Handoff to AI driver"));
    this->ControlMode = DRIVER::AI;
}

void ADReyeVRLevel::PlayPause()
{
    UE_LOG(LogTemp, Log, TEXT("Toggle Play-Pause"));
    UCarlaStatics::GetRecorder(GetWorld())->RecPlayPause();
}

void ADReyeVRLevel::FastForward()
{
    UCarlaStatics::GetRecorder(GetWorld())->RecFastForward();
}

void ADReyeVRLevel::Rewind()
{
    UCarlaStatics::GetRecorder(GetWorld())->RecRewind();
}

void ADReyeVRLevel::Restart()
{
    UE_LOG(LogTemp, Log, TEXT("Restarting recording"));
    UCarlaStatics::GetRecorder(GetWorld())->RecRestart();
}

void ADReyeVRLevel::IncrTimestep()
{
    UCarlaStatics::GetRecorder(GetWorld())->IncrTimeFactor(0.1);
}

void ADReyeVRLevel::DecrTimestep()
{
    UCarlaStatics::GetRecorder(GetWorld())->IncrTimeFactor(-0.1);
}

void ADReyeVRLevel::SetVolume()
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

FTransform ADReyeVRLevel::GetSpawnPoint(int SpawnPointIndex) const
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