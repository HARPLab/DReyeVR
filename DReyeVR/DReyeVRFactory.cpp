#include "DReyeVRFactory.h"
#include "Carla.h"                                     // to avoid linker errors
#include "Carla/Actor/ActorBlueprintFunctionLibrary.h" // UActorBlueprintFunctionLibrary
#include "Carla/Actor/VehicleParameters.h"             // FVehicleParameters
#include "Carla/Game/CarlaEpisode.h"                   // UCarlaEpisode
#include "DReyeVRGameMode.h"                           // ADReyeVRGameMode
#include "DReyeVRUtils.h"                              // DReyeVRCategory
#include "EgoSensor.h"                                 // AEgoSensor
#include "EgoVehicle.h"                                // AEgoVehicle

ADReyeVRFactory::ADReyeVRFactory(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
    for (const std::string &NameStdStr : VehicleTypes)
    {
        const FString Name = FString(NameStdStr.c_str());
        ConfigFile VehicleParams(FPaths::Combine(CarlaUE4Path, TEXT("Config/EgoVehicles"), Name + ".ini"));
        FString BP_Path;
        // make sure we can load the BP path and its nonempty (else construct a C++ EgoVehicle class)
        if (VehicleParams.bIsValid() && VehicleParams.Get<FString>("Blueprint", "Path", BP_Path) && !BP_Path.IsEmpty())
        {
            ConstructorHelpers::FObjectFinder<UClass> BlueprintObject(*UE4RefToClassPath(BP_Path));
            BP_Classes.Add(Name, BlueprintObject.Object);
        }
        else
        {
            LOG_WARN("Unable to load custom EgoVehicle \"%s\"", *Name);
        }
    }
}

TArray<FActorDefinition> ADReyeVRFactory::GetDefinitions()
{
    TArray<FActorDefinition> Definitions;

    for (auto &BP_Class_pair : BP_Classes)
    {
        FActorDefinition Def;
        FVehicleParameters Parameters;
        Parameters.Model = BP_Class_pair.Key; // vehicle type
        /// TODO: BP_Path??
        Parameters.ObjectType = BP_Class_pair.Key;
        Parameters.Class = BP_Class_pair.Value;
        /// TODO: manage number of wheels? (though carla's 2-wheeled are just secret 4-wheeled)
        Parameters.NumberOfWheels = 4;

        ADReyeVRFactory::MakeVehicleDefinition(Parameters, Def);
        Definitions.Add(Def);
    }

    FActorDefinition EgoSensorDef;
    {
        const FString Id = "ego_sensor";
        ADReyeVRFactory::MakeSensorDefinition(Id, EgoSensorDef);
        Definitions.Add(EgoSensorDef);
    }

    return Definitions;
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
    Definition = MakeGenericDefinition(DReyeVRCategory, TEXT("DReyeVR_Vehicle"), Parameters.Model);
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
    Definition = MakeGenericDefinition(DReyeVRCategory, TEXT("DReyeVR_Sensor"), Id);
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

    if (UClass::FindCommonBase(ActorDescription.Class, AEgoVehicle::StaticClass()) ==
        AEgoVehicle::StaticClass()) // is EgoVehicle or derived class
    {
        // see if this requested actor description is one of the available EgoVehicles
        /// NOTE: multi-ego-vehicle is not officially supported by DReyeVR, but it could be an interesting extension
        for (const auto &AvailableEgoVehicles : BP_Classes)
        {
            const FString &Name = AvailableEgoVehicles.Key;
            if (ActorDescription.Id.ToLower().Contains(Name.ToLower())) // contains name
            {
                // check if an EgoVehicle already exists, if so, don't spawn another.
                SpawnedActor = SpawnSingleton(ActorDescription.Class, ActorDescription.Id, SpawnAtTransform, [&]() {
                    auto *Class = AvailableEgoVehicles.Value;
                    return World->SpawnActor<AEgoVehicle>(Class, SpawnAtTransform, SpawnParameters);
                });
            }
        }
        // update the GameMode's EgoVehicle in case it was spawned by someone else
        auto *Game = Cast<ADReyeVRGameMode>(UGameplayStatics::GetGameMode(World));
        if (Game != nullptr)
            Game->SetEgoVehicle(Cast<AEgoVehicle>(SpawnedActor));
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

    if (SpawnedActor == nullptr)
    {
        LOG_WARN("Unable to spawn DReyeVR actor (\"%s\")", *ActorDescription.Id)
    }
    return FActorSpawnResult(SpawnedActor);
}
