#pragma once

#include "Carla/Actor/ActorSpawnResult.h"
#include "Carla/Actor/CarlaActorFactory.h"
#include "Carla/Actor/VehicleParameters.h"

#include "DReyeVRFactory.generated.h"

/// Factory in charge of spawning DReyeVR actors.
UCLASS()
class ADReyeVRFactory : public ACarlaActorFactory
{
    GENERATED_BODY()
  public:
    ADReyeVRFactory(const FObjectInitializer &ObjectInitializer);

    TArray<FActorDefinition> GetDefinitions() final;

    FActorSpawnResult SpawnActor(const FTransform &SpawnAtTransform, const FActorDescription &ActorDescription) final;

  private:
    void MakeVehicleDefinition(const FVehicleParameters &Parameters, FActorDefinition &Definition);
    void MakeSensorDefinition(const FString &Id, FActorDefinition &Definition);

    class UClass *EgoVehicleBPClass = nullptr;
};
