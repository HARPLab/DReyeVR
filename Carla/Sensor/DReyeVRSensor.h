#pragma once

#include "Carla/Actor/ActorDefinition.h"  // FActorDefinition
#include "Carla/Actor/ActorDescription.h" // FActorDescription
#include "Carla/Game/CarlaEpisode.h"      // UCarlaEpisode
#include "Carla/Sensor/Sensor.h"          // ASensor
#include "DReyeVRData.h"                  // AggregateData, CustomActorData
#include <cstdint>                        // int64_t
#include <string>
#include <vector>

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
    static class DReyeVR::ConfigFileData *ConfigFile;

    class DReyeVR::AggregateData *GetData()
    {
        return ADReyeVRSensor::Data;
    }

    const class DReyeVR::AggregateData *GetData() const
    {
        // read-only variant of GetData
        return const_cast<const class DReyeVR::AggregateData *>(ADReyeVRSensor::Data);
    }

    bool IsReplaying() const;
    virtual void UpdateData(const class DReyeVR::AggregateData &RecorderData, const double Per); // starts replaying
    virtual void UpdateData(const class DReyeVR::ConfigFileData &RecorderData, const double Per);
    virtual void UpdateData(const class DReyeVR::CustomActorData &RecorderData, const double Per);
    void StopReplaying();
    virtual void TakeScreenshot()
    {
        /// TODO: make this a pure virtual function (abstract class)
        DReyeVR_LOG_WARN("Not implemented! Implement in EgoSensor!");
    };

    static class ADReyeVRSensor *GetDReyeVRSensor(class UWorld *World = nullptr);
    static bool bIsReplaying;

  protected:
    void BeginPlay() override;
    void BeginDestroy() override;

    class UWorld *World;
    static class UWorld *sWorld; // to get info about the world: time, frames, etc.

    bool bStreamData = true;

    static class ADReyeVRSensor *DReyeVRSensorPtr;
    static void InterpPositionAndRotation(const FVector &Pos1, const FRotator &Rot1, const FVector &Pos2,
                                          const FRotator &Rot2, const double Per, FVector &Location,
                                          FRotator &Rotation);
};