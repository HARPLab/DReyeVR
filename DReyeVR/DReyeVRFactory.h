#pragma once

#include "Carla/Actor/ActorSpawnResult.h"
#include "Carla/Actor/CarlaActorFactory.h"
#include "Carla/Actor/VehicleParameters.h"

#include <string>
#include <vector>

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

    // place the names of all your new custom EgoVehicle types here:
    /// IMPORTANT: make sure these match the ConfigFile AND Blueprint!!
    // We expect Config/EgoVehicle/XYZ.ini and Content/DReyeVR/EgoVehicles/XYZ/BP_XYZ.uasset
    const std::vector<std::string> VehicleTypes = {
        "TeslaM3",   // Tesla Model 3 (Default)
        "Mustang66", // Mustang66
        "Jeep",      // JeepWranglerRubicon
        "Vespa"      // Vespa (2WheeledVehicles)
        // add more here
    };

    TMap<FString, UClass *> BP_Classes;
};