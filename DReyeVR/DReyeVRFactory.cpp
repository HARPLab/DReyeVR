#include "DReyeVRFactory.h"
#include "Carla.h"                                     // to avoid linker errors
#include "Carla/Actor/ActorBlueprintFunctionLibrary.h" // UActorBlueprintFunctionLibrary
#include "Carla/Actor/VehicleParameters.h"             // FVehicleParameters
#include "Carla/Game/CarlaEpisode.h"                   // UCarlaEpisode
#include "EgoSensor.h"                                 // AEgoSensor
#include "EgoVehicle.h"                                // AEgoVehicle

#define EgoVehicleBP_Str "/Game/DReyeVR/EgoVehicle/BP_model3.BP_model3_C"

// instead of vehicle.dreyevr.model3 or sensor.dreyevr.ego_sensor, we use "harplab" for category
// => harplab.dreyevr_vehicle.model3 & harplab.dreyevr_sensor.ego_sensor
// in PythonAPI use world.get_actors().filter("harplab.dreyevr_vehicle.*") or
// world.get_blueprint_library().filter("harplab.dreyevr_sensor.*") and you won't accidentally get these actors when
// performing filter("vehicle.*") or filter("sensor.*")
#define CATEGORY TEXT("HARPLab")

ADReyeVRFactory::ADReyeVRFactory(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
    // https://forums.unrealengine.com/t/cdo-constructor-failed-to-find-thirdperson-c-template-mannequin-animbp/99003
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
        Parameters.Model = "Model3";
        Parameters.ObjectType = EgoVehicleBP_Str;
        Parameters.Class = AEgoVehicle::StaticClass();
        Parameters.NumberOfWheels = 4;

        ADReyeVRFactory::MakeVehicleDefinition(Parameters, EgoVehicleDef);
    }

    FActorDefinition EgoSensorDef;
    {
        const FString Id = "Ego_Sensor";
        ADReyeVRFactory::MakeSensorDefinition(Id, EgoSensorDef);
    }

    return {EgoVehicleDef, EgoSensorDef};
}

// copied and modified from UActorBlueprintFunctionLibrary
FActorDefinition MakeGenericDefinition(const FString &Category, const FString &Type, const FString &Id)
{
    FActorDefinition Definition;

    TArray<FString> Tags = {Category.ToLower(), Type.ToLower(), Id.ToLower()};
    Definition.Id = FString::Join(Tags, TEXT("."));
    Definition.Tags = FString::Join(Tags, TEXT(","));
    return Definition;
}

void ADReyeVRFactory::MakeVehicleDefinition(const FVehicleParameters &Parameters, FActorDefinition &Definition)
{
    // assign the ID/Tags with category (ex. "vehicle.tesla.model3" => "harplab.dreyevr.model3")
    Definition = MakeGenericDefinition(CATEGORY, TEXT("DReyeVR_Vehicle"), Parameters.Model);
    Definition.Class = Parameters.Class;

    FActorAttribute ActorRole;
    ActorRole.Id = "role_name";
    ActorRole.Type = EActorAttributeType::String;
    ActorRole.Value = "hero";
    Definition.Attributes.Emplace(ActorRole);

    FActorAttribute ObjectType;
    ObjectType.Id = "object_type";
    ObjectType.Type = EActorAttributeType::String;
    ObjectType.Value = Parameters.ObjectType;
    Definition.Attributes.Emplace(ObjectType);

    FActorAttribute NumberOfWheels;
    NumberOfWheels.Id = "number_of_wheels";
    NumberOfWheels.Type = EActorAttributeType::Int;
    NumberOfWheels.Value = FString::FromInt(Parameters.NumberOfWheels);
    Definition.Attributes.Emplace(NumberOfWheels);

    FActorAttribute Generation;
    Generation.Id = "generation";
    Generation.Type = EActorAttributeType::Int;
    Generation.Value = FString::FromInt(Parameters.Generation);
    Definition.Attributes.Emplace(Generation);
}

void ADReyeVRFactory::MakeSensorDefinition(const FString &Id, FActorDefinition &Definition)
{
    Definition = MakeGenericDefinition(CATEGORY, TEXT("DReyeVR_Sensor"), Id);
    Definition.Class = AEgoSensor::StaticClass();
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

    auto SpawnSingleton = [World, SpawnParameters](UClass *ActorClass, const FString &Id, const FTransform &SpawnAt,
                                                   const std::function<AActor *()> &SpawnFn) -> AActor * {
        // function to spawn a singleton: only one actor can exist in the world at once
        ensure(World != nullptr);
        AActor *SpawnedSingleton = nullptr;
        TArray<AActor *> Found;
        UGameplayStatics::GetAllActorsOfClass(World, ActorClass, Found);
        if (Found.Num() == 0)
        {
            LOG("Spawning DReyeVR actor (\"%s\") at: %s", *Id, *SpawnAt.ToString());
            SpawnedSingleton = SpawnFn();
        }
        else
        {
            LOG_WARN("Requested to spawn another DReyeVR actor (\"%s\") but one already exists in the world!", *Id);
            ensure(Found.Num() == 1); // should only have one other that was previously spawned
            SpawnedSingleton = Found[0];
        }
        return SpawnedSingleton;
    };

    if (ActorDescription.Class == AEgoVehicle::StaticClass())
    {
        // check if an EgoVehicle already exists, if so, don't spawn another.
        /// NOTE: multi-ego-vehicle is not officially supported by DReyeVR, but it could be an interesting extension
        SpawnedActor = SpawnSingleton(ActorDescription.Class, ActorDescription.Id, SpawnAtTransform, [&]() {
            // EgoVehicle needs the special EgoVehicleBPClass since they depend on the EgoVehicle Blueprint
            return World->SpawnActor<AEgoVehicle>(EgoVehicleBPClass, SpawnAtTransform, SpawnParameters);
        });
    }
    else if (ActorDescription.Class == AEgoSensor::StaticClass())
    {
        // there should only ever be one DReyeVR sensor in the world!
        SpawnedActor = SpawnSingleton(ActorDescription.Class, ActorDescription.Id, SpawnAtTransform, [&]() {
            return World->SpawnActor<AEgoSensor>(ActorDescription.Class, SpawnAtTransform, SpawnParameters);
        });
    }
    else
    {
        LOG_ERROR("Unknown actor class in DReyeVR factory!");
    }
    return FActorSpawnResult(SpawnedActor);
}
