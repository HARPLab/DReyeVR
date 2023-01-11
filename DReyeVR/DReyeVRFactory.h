#pragma once

#include "Carla/Actor/ActorSpawnResult.h"
#include "Carla/Actor/CarlaActorFactory.h"

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
    class UClass *EgoVehicleBPClass = nullptr;
};
