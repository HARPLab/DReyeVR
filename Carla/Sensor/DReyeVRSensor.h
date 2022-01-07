#pragma once

#include "Carla/Actor/ActorDefinition.h"  // FActorDefinition
#include "Carla/Actor/ActorDescription.h" // FActorDescription
#include "Carla/Game/CarlaEpisode.h"      // UCarlaEpisode
#include "Carla/Sensor/Sensor.h"          // ASensor
#include "DReyeVRData.h"                  // AggregateData struct
#include <cstdint>                        // int64_t
#include <string>

#include "DReyeVRSensor.generated.h"

class UCarlaEpisode;

UCLASS()
class CARLA_API ADReyeVRSensor : public ASensor
{
    GENERATED_BODY()

  public:
    ADReyeVRSensor(const FObjectInitializer &ObjectInitializer);

    static FActorDefinition GetSensorDefinition();

    void Set(const FActorDescription &ActorDescription) override;

    void SetOwner(AActor *Owner) override;

    virtual void PostPhysTick(UWorld *W, ELevelTick TickType, float DeltaSeconds) override;

    // everything stored in the sensor is held in this struct
    /// TODO: make this non-static and use a smarter scheme for cross-class communication
    static class DReyeVR::AggregateData *Data;

    class DReyeVR::AggregateData *GetData()
    {
        return ADReyeVRSensor::Data;
    }

    const class DReyeVR::AggregateData *GetData() const
    {
        // read-only variant of GetData
        return ADReyeVRSensor::Data;
    }

    bool IsReplaying() const;
    void UpdateWithReplayData(const class DReyeVR::AggregateData &RecorderData, const double Per); // starts replaying
    void StopReplaying();

  protected:
    void BeginPlay() override;
    void BeginDestroy() override;
    bool bIsReplaying = false; // initially not replaying

    UWorld *World; // to get info about the world: time, frames, etc.

    bool bStreamData = true;

    static void InterpPositionAndRotation(const FVector &Pos1, const FRotator &Rot1, const FVector &Pos2,
                                          const FRotator &Rot2, const double Per, FVector &Location,
                                          FRotator &Rotation);
};