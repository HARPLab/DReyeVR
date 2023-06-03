#pragma once

#include "Carla/Sensor/DReyeVRData.h" // DReyeVR namespace
#include "GameFramework/Actor.h"      // AActor

#include <unordered_map> // std::unordered_map
#include <utility>       // std::pair
#include <vector>        // std::vector

#include "DReyeVRCustomActor.generated.h"

// define some paths to common custom actor types
#define MAT_OPAQUE "Material'/Game/DReyeVR/Custom/OpaqueParamMaterial.OpaqueParamMaterial'"
#define MAT_TRANSLUCENT "Material'/Game/DReyeVR/Custom/TranslucentParamMaterial.TranslucentParamMaterial'"
#define SM_SPHERE "StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"
#define SM_CUBE "StaticMesh'/Engine/BasicShapes/Cube.Cube'"
#define SM_CONE "StaticMesh'/Engine/BasicShapes/Cone.Cone'"
// add more custom static meshes here! they don't need to be BasicShapes

UCLASS()
class CARLA_API ADReyeVRCustomActor : public AActor // abstract class
{
    GENERATED_BODY()

  public:
    ADReyeVRCustomActor(const FObjectInitializer &ObjectInitializer);
    ~ADReyeVRCustomActor() = default;

    /// factory function to create a new instance of a given type
    static ADReyeVRCustomActor *CreateNew(const FString &SM_Path, const FString &Mat_Path, UWorld *World,
                                          const FString &Name);

    virtual void Tick(float DeltaSeconds) override;

    void Activate();
    void Deactivate();
    bool IsActive() const
    {
        return bIsActive;
    }

    void SetActorRecordEnabled(const bool bEnabled)
    {
        bShouldRecord = bEnabled;
    }

    const bool GetShouldRecord() const
    {
        return bShouldRecord;
    }

    void Initialize(const FString &Name);

    void SetInternals(const DReyeVR::CustomActorData &In);

    const DReyeVR::CustomActorData &GetInternals() const;

    static std::unordered_map<std::string, class ADReyeVRCustomActor *> ActiveCustomActors;

    inline class UStaticMeshComponent *GetMesh()
    {
        return ActorMesh;
    }

    // function to dynamically change the material params of the object at runtime
    void AssignMat(const FString &Path);
    void UpdateMaterial();
    struct DReyeVR::CustomActorData::MaterialParamsStruct MaterialParams;

  private:
    void BeginPlay() override;
    void BeginDestroy() override;
    bool bIsActive = false;    // initially deactivated
    bool bShouldRecord = true; // should record in the Carla Recorder/Replayer

    bool AssignSM(const FString &Path, UWorld *World);

    class DReyeVR::CustomActorData Internals;

    UPROPERTY(EditAnywhere, Category = "Mesh")
    class UStaticMeshComponent *ActorMesh = nullptr;
    static int AllMeshCount;

    UPROPERTY(EditAnywhere, Category = "Materials")
    class UMaterialInstanceDynamic *DynamicMat = nullptr;
};