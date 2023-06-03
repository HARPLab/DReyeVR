#include "DReyeVRCustomActor.h"
#include "Carla/Game/CarlaStatics.h"           // GetEpisode
#include "Carla/Sensor/DReyeVRSensor.h"        // ADReyeVRSensor::bIsReplaying
#include "Materials/MaterialInstance.h"        // UMaterialInstance
#include "Materials/MaterialInstanceDynamic.h" // UMaterialInstanceDynamic
#include "UObject/UObjectGlobals.h"            // LoadObject, NewObject

#include <string>

// HACK: assuming you have no more than 10 unique material instances on a single static mesh
// this is what is used to broadacast the single DynamicMat instance to all material slots in the SM
// and calls SetMaterial which grows an internal array (of pointers) to match the index:
// https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/Components/UMeshComponent/SetMaterial/
#define MAX_POSSIBLE_MATERIALS 10

std::unordered_map<std::string, class ADReyeVRCustomActor *> ADReyeVRCustomActor::ActiveCustomActors = {};
int ADReyeVRCustomActor::AllMeshCount = 0;

ADReyeVRCustomActor *ADReyeVRCustomActor::CreateNew(const FString &SM_Path, const FString &Mat_Path, UWorld *World,
                                                    const FString &Name)
{
    check(World != nullptr);
    FActorSpawnParameters SpawnInfo;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ADReyeVRCustomActor *Actor =
        World->SpawnActor<ADReyeVRCustomActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
    Actor->Initialize(Name);

    if (Actor->AssignSM(SM_Path, World))
    {
        Actor->Internals.MeshPath = SM_Path;
        Actor->AssignMat(Mat_Path);
    }

    return Actor;
}

ADReyeVRCustomActor::ADReyeVRCustomActor(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;

    // turning off physics interaction
    this->SetActorEnableCollision(false);

    // done in child class
    Internals.Location = this->GetActorLocation();
    Internals.Rotation = this->GetActorRotation();
    Internals.Scale3D = this->GetActorScale3D();
}

bool ADReyeVRCustomActor::AssignSM(const FString &Path, UWorld *World)
{
    UStaticMesh *SM = LoadObject<UStaticMesh>(nullptr, *Path);
    ensure(SM != nullptr);
    ensure(World != nullptr);
    if (SM && World)
    {
        // Using static AllMeshCount to create a unique component name every time
        FName MeshName(*("Mesh" + FString::FromInt(ADReyeVRCustomActor::AllMeshCount)));
        ActorMesh = NewObject<UStaticMeshComponent>(this, MeshName);
        ensure(ActorMesh != nullptr);
        ActorMesh->SetupAttachment(this->GetRootComponent());
        this->SetRootComponent(ActorMesh);
        ActorMesh->SetStaticMesh(SM);
        ActorMesh->SetVisibility(false);
        ActorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ActorMesh->RegisterComponentWithWorld(World);
        this->AddInstanceComponent(ActorMesh);
    }
    else
    {
        DReyeVR_LOG_ERROR("Unable to create static mesh: %s", *Path);
        return false;
    }
    ADReyeVRCustomActor::AllMeshCount++;
    return true;
}

void ADReyeVRCustomActor::AssignMat(const FString &MaterialPath)
{
    // MaterialPath should be one of {MAT_OPAQUE, MAT_TRANSLUCENT} to receive params
    UMaterial *Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
    ensure(Material != nullptr);

    // create sole dynamic material
    DynamicMat = UMaterialInstanceDynamic::Create(Material, this);
    MaterialParams.Apply(DynamicMat);           // apply the parameters to this dynamic material
    MaterialParams.MaterialPath = MaterialPath; // for now does not change over time

    if (DynamicMat != nullptr && ActorMesh != nullptr)
        for (int i = 0; i < MAX_POSSIBLE_MATERIALS; i++)
            ActorMesh->SetMaterial(i, DynamicMat);
    else
        DReyeVR_LOG_ERROR("Unable to access material asset: %s", *MaterialPath)
}

void ADReyeVRCustomActor::Initialize(const FString &Name)
{
    Internals.Name = Name;
    ADReyeVRCustomActor::ActiveCustomActors[TCHAR_TO_UTF8(*Name)] = this;
}

void ADReyeVRCustomActor::BeginPlay()
{
    Super::BeginPlay();
}

void ADReyeVRCustomActor::BeginDestroy()
{
    this->Deactivate(); // remove from global static table
    Super::BeginDestroy();
}

void ADReyeVRCustomActor::Deactivate()
{
    const std::string s = TCHAR_TO_UTF8(*Internals.Name);
    if (ADReyeVRCustomActor::ActiveCustomActors.find(s) != ADReyeVRCustomActor::ActiveCustomActors.end())
    {
        ADReyeVRCustomActor::ActiveCustomActors.erase(s);
    }
    this->SetActorHiddenInGame(true);
    if (ActorMesh)
        ActorMesh->SetVisibility(false);
    this->SetActorTickEnabled(false);
    this->bIsActive = false;
}

void ADReyeVRCustomActor::Activate()
{
    const std::string s = TCHAR_TO_UTF8(*Internals.Name);
    if (ADReyeVRCustomActor::ActiveCustomActors.find(s) == ADReyeVRCustomActor::ActiveCustomActors.end())
        ADReyeVRCustomActor::ActiveCustomActors[s] = this;
    else
        ensure(ADReyeVRCustomActor::ActiveCustomActors[s] == this);
    this->SetActorHiddenInGame(false);
    if (ActorMesh)
        ActorMesh->SetVisibility(true);
    this->SetActorTickEnabled(true);
    this->bIsActive = true;
}

void ADReyeVRCustomActor::Tick(float DeltaSeconds)
{
    if (ADReyeVRSensor::bIsReplaying)
    {
        // update world state with internals
        this->SetActorLocation(Internals.Location);
        this->SetActorRotation(Internals.Rotation);
        this->SetActorScale3D(Internals.Scale3D);
        this->MaterialParams = Internals.MaterialParams;
    }
    else
    {
        // update internals with world state
        Internals.Location = this->GetActorLocation();
        Internals.Rotation = this->GetActorRotation();
        Internals.Scale3D = this->GetActorScale3D();
        Internals.MaterialParams = MaterialParams;
    }
    UpdateMaterial();
    /// TODO: use other string?
}

void ADReyeVRCustomActor::UpdateMaterial()
{
    // update the materials according to the params
    MaterialParams.Apply(DynamicMat);
}

void ADReyeVRCustomActor::SetInternals(const DReyeVR::CustomActorData &InData)
{
    Internals = InData;
}

const DReyeVR::CustomActorData &ADReyeVRCustomActor::GetInternals() const
{
    return Internals;
}