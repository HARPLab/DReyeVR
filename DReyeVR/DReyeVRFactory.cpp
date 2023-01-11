#include "DReyeVRFactory.h"
#include "Carla.h"                                     // to avoid linker errors
#include "Carla/Actor/ActorBlueprintFunctionLibrary.h" // UActorBlueprintFunctionLibrary
#include "Carla/Actor/VehicleParameters.h"             // FVehicleParameters
#include "Carla/Game/CarlaEpisode.h"                   // UCarlaEpisode
#include "EgoSensor.h"                                 // AEgoSensor
#include "EgoVehicle.h"                                // AEgoVehicle

#define EgoVehicleBP_Str "/Game/Carla/Blueprints/Vehicles/DReyeVR/BP_EgoVehicle_DReyeVR.BP_EgoVehicle_DReyeVR_C"

ADReyeVRFactory::ADReyeVRFactory(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
    // get ego vehicle bp (can use UTF8_TO_TCHAR if making EgoVehicleBP_Str a variable)
    static ConstructorHelpers::FObjectFinder<UClass> EgoVehicleBP(TEXT(EgoVehicleBP_Str));
    EgoVehicleBPClass = EgoVehicleBP.Object;
    ensure(EgoVehicleBPClass != nullptr);
}

TArray<FActorDefinition> ADReyeVRFactory::GetDefinitions()
{
    FActorDefinition EgoVehicleDef;
    {
        FVehicleParameters Parameters;
        Parameters.Make = "DReyeVR";
        Parameters.Model = "Model3";
        Parameters.ObjectType = EgoVehicleBP_Str;
        Parameters.Class = AEgoVehicle::StaticClass();

        // need to create an FActorDefinition from our FActorDescription for some reason -_-
        bool Success = false;
        UActorBlueprintFunctionLibrary::MakeVehicleDefinition(Parameters, Success, EgoVehicleDef);
        if (!Success)
        {
            LOG_ERROR("Unable to create DReyeVR vehicle definition!");
        }
        EgoVehicleDef.Class = Parameters.Class;
    }

    FActorDefinition EgoSensorDef;
    {
        const FString Type = "DReyeVR";
        const FString Id = "Ego_Sensor";
        EgoSensorDef = UActorBlueprintFunctionLibrary::MakeGenericSensorDefinition(Type, Id);
        EgoSensorDef.Class = AEgoSensor::StaticClass();
    }

    return {EgoVehicleDef, EgoSensorDef};
}

FActorSpawnResult ADReyeVRFactory::SpawnActor(const FTransform &SpawnAtTransform,
                                              const FActorDescription &ActorDescription)
{
    auto *World = GetWorld();
    if (World == nullptr)
    {
        LOG_ERROR("cannot spawn \"%s\" into an empty world.", *ActorDescription.Id);
        return {};
    }

    AActor *SpawnedActor = nullptr;

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (ActorDescription.Class == AEgoVehicle::StaticClass())
    {
        LOG("Spawning EgoVehicle (\"%s\") at: %s", *ActorDescription.Id, *SpawnAtTransform.ToString());
        SpawnedActor = World->SpawnActor<AEgoVehicle>(EgoVehicleBPClass, SpawnAtTransform, SpawnParameters);
    }
    else if (ActorDescription.Class == AEgoSensor::StaticClass())
    {
        LOG("Spawning EgoSensor (\"%s\") at: %s", *ActorDescription.Id, *SpawnAtTransform.ToString());
        SpawnedActor = World->SpawnActor<AEgoSensor>(ActorDescription.Class, SpawnAtTransform, SpawnParameters);
    }
    else
    {
        LOG_ERROR("Unknown actor class in DReyeVR factory!");
    }
    return FActorSpawnResult(SpawnedActor);
}
