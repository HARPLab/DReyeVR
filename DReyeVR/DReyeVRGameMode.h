#pragma once

#include "Carla/Actor/DReyeVRCustomActor.h" // ADReyeVRCustomActor
#include "Carla/Game/CarlaGameModeBase.h"   // ACarlaGameModeBase
#include "Carla/Sensor/DReyeVRData.h"       // DReyeVR::
#include <unordered_map>                    // std::unordered_map

#include "DReyeVRGameMode.generated.h"

class AEgoVehicle;
class ADReyeVRPawn;

UCLASS()
class ADReyeVRGameMode : public ACarlaGameModeBase
{
    GENERATED_UCLASS_BODY()

  public:
    ADReyeVRGameMode();

    virtual void BeginPlay() override;

    virtual void BeginDestroy() override;

    virtual void Tick(float DeltaSeconds) override;

    // input handling
    void SetupPlayerInputComponent();

    // EgoVehicle functions
    enum DRIVER
    {
        HUMAN,
        SPECTATOR,
        AI,
    } ControlMode;
    void PossessEgoVehicle();
    void PossessSpectator();
    void HandoffDriverToAI();

    // Replay media functions
    void ChangeTimestep(UWorld *World, double AmntChangeSeconds);
    void ReplayPlayPause();
    void ReplayFastForward();
    void ReplayRewind();
    void ReplayRestart();
    void ReplaySpeedUp();
    void ReplaySlowDown();

    // Replayer
    void SetupReplayer();

    // Meta world functions
    void SetVolume();
    FTransform GetSpawnPoint(int SpawnPointIndex = 0) const;

    // Custom actors
    void ReplayCustomActor(const DReyeVR::CustomActorData &RecorderData, const double Per);
    void DrawBBoxes();
    std::unordered_map<std::string, ADReyeVRCustomActor *> BBoxes;

  private:
    bool bDoSpawnEgoVehicle = true; // spawn Ego on BeginPlay or not

    // for handling inputs and possessions
    void SetupDReyeVRPawn();
    void SetupSpectator();
    bool SetupEgoVehicle();
    class APlayerController *Player = nullptr;
    class ADReyeVRPawn *DReyeVR_Pawn = nullptr;
    class UClass *EgoVehicleBPClass = nullptr;

    // for toggling bw spectator mode
    bool bIsSpectating = true;
    class APawn *SpectatorPtr = nullptr;
    class AEgoVehicle *EgoVehiclePtr = nullptr;

    // for audio control
    float EgoVolumePercent;
    float NonEgoVolumePercent;
    float AmbientVolumePercent;

    bool bDoSpawnEgoVehicleTransform = false; // whether or not to use provided pose
    FVector SpawnEgoVehicleLocation;
    FRotator SpawnEgoVehicleRotation;

    // for recorder/replayer params
    const double AmntPlaybackIncr = 0.1; // how much the playback speed changes (seconds)
    double ReplayTimeFactor = 1.0;       // same as CarlaReplayer.h::TimeFactor (but local)
    double ReplayTimeFactorMin = 0.0;    // minimum playback of 0 (paused)
    double ReplayTimeFactorMax = 10.0;   // maximum of 10x playback
    bool bReplaySync = false;            // false allows for interpolation
    bool bRecorderInitiated = false;     // allows tick-wise checking for replayer/recorder
};