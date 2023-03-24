#pragma once

#include "Carla/Recorder/CarlaRecorderHelpers.h" // WriteValue, WriteFVector, WriteFString, ...
#include "Materials/MaterialInstanceDynamic.h"   // UMaterialInstanceDynamic
#include <chrono>                                // timing threads
#include <cstdint>                               // int64_t
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "Carla/Actor/ActorInfo.h"
#include "Carla/Actor/ActorData.h"
#include "Carla/Vehicle/CarlaWheeledVehicle.h"
#include "Carla/Walker/WalkerController.h"
#include "Carla/Traffic/TrafficLightState.h"

#include "Carla/Actor/ActorDefinition.h"
#include "Carla/Actor/ActorDescription.h"
#include "Carla/Actor/ActorRegistry.h"

namespace DReyeVR
{

struct CARLA_API DataSerializer
{
    DataSerializer() = default;
    virtual ~DataSerializer() = default;

    virtual void Read(std::ifstream &InFile) = 0;
    virtual void Write(std::ofstream &OutFile) const = 0;
    virtual FString ToString() const = 0;
};

struct CARLA_API EyeData : public DataSerializer
{
    FVector GazeDir = FVector::ZeroVector;
    FVector GazeOrigin = FVector::ZeroVector;
    bool GazeValid = false;

    void Read(std::ifstream &InFile) override;
    void Write(std::ofstream &OutFile) const override;
    FString ToString() const override;
};

struct CARLA_API CombinedEyeData : EyeData
{
    float Vergence = 0.f; // in cm (default UE4 units)

    void Read(std::ifstream &InFile) override;
    void Write(std::ofstream &OutFile) const override;
    FString ToString() const override;
};

struct CARLA_API SingleEyeData : EyeData
{
    float EyeOpenness = 0.f;
    bool EyeOpennessValid = false;
    float PupilDiameter = 0.f;
    FVector2D PupilPosition = FVector2D::ZeroVector;
    bool PupilPositionValid = false;

    void Read(std::ifstream &InFile) override;
    void Write(std::ofstream &OutFile) const override;
    FString ToString() const override;
};

struct CARLA_API EgoVariables : public DataSerializer
{
    // World coordinate Ego vehicle location & rotation
    FVector VehicleLocation = FVector::ZeroVector;
    FRotator VehicleRotation = FRotator::ZeroRotator;
    // Relative Camera position and orientation (for HMD offsets)
    FVector CameraLocation = FVector::ZeroVector;
    FRotator CameraRotation = FRotator::ZeroRotator;
    // Absolute Camera position and orientation (includes vehicle & HMD offset)
    FVector CameraLocationAbs = FVector::ZeroVector;
    FRotator CameraRotationAbs = FRotator::ZeroRotator;
    // Ego variables
    float Velocity = 0.f; // note this is in cm/s (default UE4 units)

    void Read(std::ifstream &InFile) override;
    void Write(std::ofstream &OutFile) const override;
    FString ToString() const override;
};

struct CARLA_API UserInputs : public DataSerializer
{
    // User inputs
    float Throttle = 0.f;
    float Steering = 0.f;
    float Brake = 0.f;
    bool ToggledReverse = false;
    bool TurnSignalLeft = false;
    bool TurnSignalRight = false;
    bool HoldHandbrake = false;
    // Add more inputs here!

    void Read(std::ifstream &InFile) override;
    void Write(std::ofstream &OutFile) const override;
    FString ToString() const override;
};

struct CARLA_API FocusInfo : public DataSerializer
{
    // substitute for SRanipal FFocusInfo in SRanipal_Eyes_Enums.h
    TWeakObjectPtr<class AActor> Actor;
    FVector HitPoint; // in world space (absolute location)
    FVector Normal;
    FString ActorNameTag = "None"; // Tag of the actor being focused on
    float Distance;
    bool bDidHit;

    void Read(std::ifstream &InFile) override;
    void Write(std::ofstream &OutFile) const override;
    FString ToString() const override;
};

struct CARLA_API EyeTracker : public DataSerializer
{
    int64_t TimestampDevice = 0; // timestamp from the eye tracker device (with its own clock)
    int64_t FrameSequence = 0;   // "Frame sequence" of SRanipal or just the tick frame in UE4
    CombinedEyeData Combined;
    SingleEyeData Left;
    SingleEyeData Right;

    void Read(std::ifstream &InFile) override;
    void Write(std::ofstream &OutFile) const override;
    FString ToString() const override;
};

enum class Gaze
{
    COMBINED, // default for functions
    RIGHT,
    LEFT,
};

enum class Eye
{
    RIGHT,
    LEFT,
};

struct AwarenessActor
{
    int64_t ActorId;
    FVector ActorLocation;
    FVector ActorVelocity;
    int64_t Answer = 0;

    void Read(std::ifstream &InFile);
    void Write(std::ofstream &OutFile) const;
    FString ToString() const;
};

struct AwarenessInfo
{
    int64_t VisibleTotal = 0;
    std::vector<FCarlaActor*> VisibleRaw;
    std::vector<AwarenessActor> Visible;
    int UserInput = 0;
    void Read(std::ifstream &InFile);
    void Write(std::ofstream &OutFile) const;
    FString ToString() const;
};


// all DReyeVR sensor data is held here
class CARLA_API AggregateData : public DataSerializer
{
  public:
    AggregateData() = default;
    /////////////////////////:GETTERS://////////////////////////////

    int64_t GetTimestampCarla() const;
    int64_t GetTimestampDevice() const;
    int64_t GetFrameSequence() const;
    float GetGazeVergence() const;
    const FVector &GetGazeDir(DReyeVR::Gaze Index = DReyeVR::Gaze::COMBINED) const;
    const FVector &GetGazeOrigin(DReyeVR::Gaze Index = DReyeVR::Gaze::COMBINED) const;
    bool GetGazeValidity(DReyeVR::Gaze Index = DReyeVR::Gaze::COMBINED) const;
    float GetEyeOpenness(DReyeVR::Eye Index) const; // returns eye openness as a percentage [0,1]
    bool GetEyeOpennessValidity(DReyeVR::Eye Index) const;
    float GetPupilDiameter(DReyeVR::Eye Index) const; // returns diameter in mm
    const FVector2D &GetPupilPosition(DReyeVR::Eye Index) const;
    bool GetPupilPositionValidity(DReyeVR::Eye Index) const;

    // from EgoVars
    const FVector &GetCameraLocation() const;
    const FRotator &GetCameraRotation() const;
    const FVector &GetCameraLocationAbs() const;
    const FRotator &GetCameraRotationAbs() const;
    float GetVehicleVelocity() const;
    const FVector &GetVehicleLocation() const;
    const FRotator &GetVehicleRotation() const;
    std::string GetUniqueName() const;

    // focus
    const FString &GetFocusActorName() const;
    const FVector &GetFocusActorPoint() const;
    float GetFocusActorDistance() const;
    const DReyeVR::UserInputs &GetUserInputs() const;

    // Awareness
    bool AwarenessMode = true;
    const DReyeVR::AwarenessInfo &GetAwarenessData() const;
    void SetAwarenessData(const int64_t NewVisibleTotal, 
                          const std::vector<DReyeVR::AwarenessActor> &NewVisible,
                          const std::vector<FCarlaActor*> &NewVisibleRaw,
                          const int64_t NewUserInput);


    ////////////////////:SETTERS://////////////////////
    void UpdateCamera(const FVector &NewCameraLoc, const FRotator &NewCameraRot);
    void UpdateCameraAbs(const FVector &NewCameraLocAbs, const FRotator &NewCameraRotAbs);
    void UpdateVehicle(const FVector &NewVehicleLoc, const FRotator &NewVehicleRot);
    void Update(int64_t NewTimestamp, const struct EyeTracker &NewEyeData, const struct EgoVariables &NewEgoVars,
                const struct FocusInfo &NewFocus, const struct UserInputs &NewInputs);

    ////////////////////:SERIALIZATION://////////////////////
    void Read(std::ifstream &InFile) override;
    void Write(std::ofstream &OutFile) const override;
    FString ToString() const override;

  private:
    int64_t TimestampCarlaUE4; // Carla Timestamp (EgoSensor Tick() event) in milliseconds
    struct EyeTracker EyeTrackerData;
    struct EgoVariables EgoVars;
    struct FocusInfo FocusData;
    struct UserInputs Inputs;
    struct AwarenessInfo AwarenessData;
};

class CARLA_API CustomActorData : public DataSerializer
{
  public:
    FString Name; // unique actor name of this actor
    // 9 dof (location, rotation, scale) for non-rigid body in 3D space
    FVector Location;
    FRotator Rotation;
    FVector Scale3D;
    // visual properties
    FString MeshPath;
    // material properties
    struct CARLA_API MaterialParamsStruct : public DataSerializer
    {
        /// for an explanation of these, see MaterialParamsStruct::Apply
        // in DReyeVRData.inl
        float Metallic = 1.f;
        float Specular = 0.f;
        float Roughness = 1.f;
        float Anisotropy = 1.f;
        float Opacity = 1.f;
        FLinearColor BaseColor = FLinearColor::Red;
        // usually a scale factor like 500 is good to emit bright light on a sunny day
        FLinearColor Emissive = 500.f * FLinearColor::Red;
        FString MaterialPath;
        void Apply(class UMaterialInstanceDynamic *Material) const;

        void Read(std::ifstream &InFile) override;
        void Write(std::ofstream &OutFile) const override;
        FString ToString() const override;
    };
    MaterialParamsStruct MaterialParams;
    // other
    FString Other; // any other data deemed necessary to record

    CustomActorData() = default;

    void Read(std::ifstream &InFile) override;
    void Write(std::ofstream &OutFile) const override;
    FString ToString() const override;
    std::string GetUniqueName() const;
};

}; // namespace DReyeVR