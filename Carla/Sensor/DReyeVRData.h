#pragma once
#ifndef DREYEVR_SENSOR_DATA
#define DREYEVR_SENSOR_DATA

#include "Carla/Recorder/CarlaRecorderHelpers.h" // WriteValue, WriteFVector, WriteFString, ...
#include <chrono>                                // timing threads
#include <cstdint>                               // int64_t
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

/// NOTE: all functions here are inline to avoid nasty linker errors. Though this can
// probably be refactored to have a proper associated .cpp file

namespace DReyeVR
{
struct EyeData
{
    FVector GazeDir = FVector::ZeroVector;
    FVector GazeOrigin = FVector::ZeroVector;
    bool GazeValid = false;
    void Read(std::ifstream &InFile)
    {
        ReadFVector(InFile, GazeDir);
        ReadFVector(InFile, GazeOrigin);
        ReadValue<bool>(InFile, GazeValid);
    }
    void Write(std::ofstream &OutFile) const
    {
        WriteFVector(OutFile, GazeDir);
        WriteFVector(OutFile, GazeOrigin);
        WriteValue<bool>(OutFile, GazeValid);
    }
    FString ToString() const
    {
        FString Print;
        Print += FString::Printf(TEXT("GazeDir:%s,"), *GazeDir.ToString());
        Print += FString::Printf(TEXT("GazeOrigin:%s,"), *GazeOrigin.ToString());
        Print += FString::Printf(TEXT("GazeValid:%d,"), GazeValid);
        return Print;
    }
};

struct CombinedEyeData : EyeData
{
    float Vergence = 0.f; // in cm (default UE4 units)
    void Read(std::ifstream &InFile)
    {
        EyeData::Read(InFile);
        ReadValue<float>(InFile, Vergence);
    }
    void Write(std::ofstream &OutFile) const
    {
        EyeData::Write(OutFile);
        WriteValue<float>(OutFile, Vergence);
    }
    FString ToString() const
    {
        FString Print = EyeData::ToString();
        Print += FString::Printf(TEXT("GazeVergence:%.4f,"), Vergence);
        return Print;
    }
};

struct SingleEyeData : EyeData
{
    float EyeOpenness = 0.f;
    bool EyeOpennessValid = false;
    float PupilDiameter = 0.f;
    FVector2D PupilPosition = FVector2D::ZeroVector;
    bool PupilPositionValid = false;

    void Read(std::ifstream &InFile)
    {
        EyeData::Read(InFile);
        ReadValue<float>(InFile, EyeOpenness);
        ReadValue<bool>(InFile, EyeOpennessValid);
        ReadValue<float>(InFile, PupilDiameter);
        ReadFVector2D(InFile, PupilPosition);
        ReadValue<bool>(InFile, PupilPositionValid);
    }
    void Write(std::ofstream &OutFile) const
    {
        EyeData::Write(OutFile);
        WriteValue<float>(OutFile, EyeOpenness);
        WriteValue<bool>(OutFile, EyeOpennessValid);
        WriteValue<float>(OutFile, PupilDiameter);
        WriteFVector2D(OutFile, PupilPosition);
        WriteValue<bool>(OutFile, PupilPositionValid);
    }
    FString ToString() const
    {
        FString Print = EyeData::ToString();
        Print += FString::Printf(TEXT("EyeOpenness:%.4f,"), EyeOpenness);
        Print += FString::Printf(TEXT("EyeOpennessValid:%d,"), EyeOpennessValid);
        Print += FString::Printf(TEXT("PupilDiameter:%.4f,"), PupilDiameter);
        Print += FString::Printf(TEXT("PupilPosition:%s,"), *PupilPosition.ToString());
        Print += FString::Printf(TEXT("PupilPositionValid:%d,"), PupilPositionValid);
        return Print;
    }
};

struct EgoVariables
{
    // World coordinate Ego vehicle location & rotation
    FVector VehicleLocation = FVector::ZeroVector;
    FRotator VehicleRotation = FRotator::ZeroRotator;
    // Relative Camera position and orientation
    FVector CameraLocation = FVector::ZeroVector;
    FRotator CameraRotation = FRotator::ZeroRotator;
    // Ego variables
    float Velocity = 0.f; // note this is in cm/s (default UE4 units)
    void Read(std::ifstream &InFile)
    {
        ReadFVector(InFile, CameraLocation);
        ReadFRotator(InFile, CameraRotation);
        ReadFVector(InFile, VehicleLocation);
        ReadFRotator(InFile, VehicleRotation);
        ReadValue<float>(InFile, Velocity);
    }
    void Write(std::ofstream &OutFile) const
    {
        WriteFVector(OutFile, CameraLocation);
        WriteFRotator(OutFile, CameraRotation);
        WriteFVector(OutFile, VehicleLocation);
        WriteFRotator(OutFile, VehicleRotation);
        WriteValue<float>(OutFile, Velocity);
    }
    FString ToString() const
    {
        FString Print;
        Print += FString::Printf(TEXT("VehicleLoc:%s,"), *VehicleLocation.ToString());
        Print += FString::Printf(TEXT("VehicleRot:%s,"), *VehicleRotation.ToString());
        Print += FString::Printf(TEXT("VehicleVel:%.4f,"), Velocity);
        Print += FString::Printf(TEXT("CameraLoc:%s,"), *CameraLocation.ToString());
        Print += FString::Printf(TEXT("CameraRot:%s,"), *CameraRotation.ToString());
        return Print;
    }
};

struct UserInputs
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

    void Read(std::ifstream &InFile)
    {
        ReadValue<float>(InFile, Throttle);
        ReadValue<float>(InFile, Steering);
        ReadValue<float>(InFile, Brake);
        ReadValue<bool>(InFile, ToggledReverse);
        ReadValue<bool>(InFile, TurnSignalLeft);
        ReadValue<bool>(InFile, TurnSignalRight);
        ReadValue<bool>(InFile, HoldHandbrake);
    }
    void Write(std::ofstream &OutFile) const
    {
        WriteValue<float>(OutFile, Throttle);
        WriteValue<float>(OutFile, Steering);
        WriteValue<float>(OutFile, Brake);
        WriteValue<bool>(OutFile, ToggledReverse);
        WriteValue<bool>(OutFile, TurnSignalLeft);
        WriteValue<bool>(OutFile, TurnSignalRight);
        WriteValue<bool>(OutFile, HoldHandbrake);
    }
    FString ToString() const
    {
        FString Print;
        Print += FString::Printf(TEXT("Throttle:%.4f,"), Throttle);
        Print += FString::Printf(TEXT("Steering:%.4f,"), Steering);
        Print += FString::Printf(TEXT("Brake:%.4f,"), Brake);
        Print += FString::Printf(TEXT("ToggledReverse:%d,"), ToggledReverse);
        Print += FString::Printf(TEXT("TurnSignalLeft:%d,"), TurnSignalLeft);
        Print += FString::Printf(TEXT("TurnSignalRight:%d,"), TurnSignalRight);
        Print += FString::Printf(TEXT("HoldHandbrake:%d,"), HoldHandbrake);
        return Print;
    }
};

struct FocusInfo
{
    // substitute for SRanipal FFocusInfo in SRanipal_Eyes_Enums.h
    TWeakObjectPtr<class AActor> Actor;
    FVector HitPointRelative;
    FVector HitPointAbsolute;
    FVector Normal;
    FString ActorNameTag = "None"; // Tag of the actor being focused on
    float Distance;
    bool bDidHit;

    void Read(std::ifstream &InFile)
    {
        ReadFString(InFile, ActorNameTag);
        ReadValue<bool>(InFile, bDidHit);
        ReadFVector(InFile, HitPointRelative);
        ReadFVector(InFile, HitPointAbsolute);
        ReadFVector(InFile, Normal);
        ReadValue<float>(InFile, Distance);
    }
    void Write(std::ofstream &OutFile) const
    {
        WriteFString(OutFile, ActorNameTag);
        WriteValue<bool>(OutFile, bDidHit);
        WriteFVector(OutFile, HitPointRelative);
        WriteFVector(OutFile, HitPointAbsolute);
        WriteFVector(OutFile, Normal);
        WriteValue<float>(OutFile, Distance);
    }
    FString ToString() const
    {
        FString Print;
        Print += FString::Printf(TEXT("Hit:%d,"), bDidHit);
        Print += FString::Printf(TEXT("Distance:%.4f,"), Distance);
        Print += FString::Printf(TEXT("HitPointRelative:%s,"), *HitPointRelative.ToString());
        Print += FString::Printf(TEXT("HitPointAbsolute:%s,"), *HitPointAbsolute.ToString());
        Print += FString::Printf(TEXT("HitNormal:%s,"), *Normal.ToString());
        Print += FString::Printf(TEXT("ActorName:%s,"), *ActorNameTag);
        return Print;
    }
};

struct EyeTracker
{
    int64_t TimestampDevice = 0; // timestamp from the eye tracker device (with its own clock)
    int64_t FrameSequence = 0;   // "Frame sequence" of SRanipal or just the tick frame in UE4
    CombinedEyeData Combined;
    SingleEyeData Left;
    SingleEyeData Right;
    void Read(std::ifstream &InFile)
    {
        ReadValue<int64_t>(InFile, TimestampDevice);
        ReadValue<int64_t>(InFile, FrameSequence);
        Combined.Read(InFile);
        Left.Read(InFile);
        Right.Read(InFile);
    }
    void Write(std::ofstream &OutFile) const
    {
        WriteValue<int64_t>(OutFile, TimestampDevice);
        WriteValue<int64_t>(OutFile, FrameSequence);
        Combined.Write(OutFile);
        Left.Write(OutFile);
        Right.Write(OutFile);
    }
    FString ToString() const
    {
        FString Print;
        Print += FString::Printf(TEXT("TimestampDevice:%ld,"), long(TimestampDevice));
        Print += FString::Printf(TEXT("FrameSequence:%ld,"), long(FrameSequence));
        Print += FString::Printf(TEXT("COMBINED:{%s},"), *Combined.ToString());
        Print += FString::Printf(TEXT("LEFT:{%s},"), *Left.ToString());
        Print += FString::Printf(TEXT("RIGHT:{%s},"), *Right.ToString());
        return Print;
    }
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

class AggregateData // all DReyeVR sensor data is held here
{
  public:
    AggregateData() = default;
    /////////////////////////:GETTERS://////////////////////////////

    int64_t GetTimestampCarla() const
    {
        return TimestampCarlaUE4;
    }
    int64_t GetTimestampDevice() const
    {
        return EyeTrackerData.TimestampDevice;
    }
    int64_t GetFrameSequence() const
    {
        return EyeTrackerData.FrameSequence;
    }
    float GetGazeVergence() const
    {
        return EyeTrackerData.Combined.Vergence; // in cm (default UE4 units)
    }
    const FVector &GetGazeDir(DReyeVR::Gaze Index = DReyeVR::Gaze::COMBINED) const
    {
        switch (Index)
        {
        case DReyeVR::Gaze::LEFT:
            return EyeTrackerData.Left.GazeDir;
        case DReyeVR::Gaze::RIGHT:
            return EyeTrackerData.Right.GazeDir;
        case DReyeVR::Gaze::COMBINED:
            return EyeTrackerData.Combined.GazeDir;
        default: // need a default case for MSVC >:(
            return EyeTrackerData.Combined.GazeDir;
        }
    }
    const FVector &GetGazeOrigin(DReyeVR::Gaze Index = DReyeVR::Gaze::COMBINED) const
    {
        switch (Index)
        {
        case DReyeVR::Gaze::LEFT:
            return EyeTrackerData.Left.GazeOrigin;
        case DReyeVR::Gaze::RIGHT:
            return EyeTrackerData.Right.GazeOrigin;
        case DReyeVR::Gaze::COMBINED:
            return EyeTrackerData.Combined.GazeOrigin;
        default: // need a default case for MSVC >:(
            return EyeTrackerData.Combined.GazeOrigin;
        }
    }
    bool GetGazeValidity(DReyeVR::Gaze Index = DReyeVR::Gaze::COMBINED) const
    {
        switch (Index)
        {
        case DReyeVR::Gaze::LEFT:
            return EyeTrackerData.Left.GazeValid;
        case DReyeVR::Gaze::RIGHT:
            return EyeTrackerData.Right.GazeValid;
        case DReyeVR::Gaze::COMBINED:
            return EyeTrackerData.Combined.GazeValid;
        default: // need a default case for MSVC >:(
            return EyeTrackerData.Combined.GazeValid;
        }
    }
    float GetEyeOpenness(DReyeVR::Eye Index) const // returns eye openness as a percentage [0,1]
    {
        switch (Index)
        {
        case DReyeVR::Eye::LEFT:
            return EyeTrackerData.Left.EyeOpenness;
        case DReyeVR::Eye::RIGHT:
            return EyeTrackerData.Right.EyeOpenness;
        default: // need a default case for MSVC >:(
            return EyeTrackerData.Right.EyeOpenness;
        }
    }
    bool GetEyeOpennessValidity(DReyeVR::Eye Index) const
    {
        switch (Index)
        {
        case DReyeVR::Eye::LEFT:
            return EyeTrackerData.Left.EyeOpennessValid;
        case DReyeVR::Eye::RIGHT:
            return EyeTrackerData.Right.EyeOpennessValid;
        default: // need a default case for MSVC >:(
            return EyeTrackerData.Right.EyeOpennessValid;
        }
    }
    float GetPupilDiameter(DReyeVR::Eye Index) const // returns diameter in mm
    {
        switch (Index)
        {
        case DReyeVR::Eye::LEFT:
            return EyeTrackerData.Left.PupilDiameter;
        case DReyeVR::Eye::RIGHT:
            return EyeTrackerData.Right.PupilDiameter;
        default: // need a default case for MSVC >:(
            return EyeTrackerData.Right.PupilDiameter;
        }
    }
    const FVector2D &GetPupilPosition(DReyeVR::Eye Index) const
    {
        switch (Index)
        {
        case DReyeVR::Eye::LEFT:
            return EyeTrackerData.Left.PupilPosition;
        case DReyeVR::Eye::RIGHT:
            return EyeTrackerData.Right.PupilPosition;
        default: // need a default case for MSVC >:(
            return EyeTrackerData.Right.PupilPosition;
        }
    }
    bool GetPupilPositionValidity(DReyeVR::Eye Index) const
    {
        switch (Index)
        {
        case DReyeVR::Eye::LEFT:
            return EyeTrackerData.Left.PupilPositionValid;
        case DReyeVR::Eye::RIGHT:
            return EyeTrackerData.Right.PupilPositionValid;
        default: // need a default case for MSVC >:(
            return EyeTrackerData.Right.PupilPositionValid;
        }
    }

    // from EgoVars
    const FVector &GetCameraLocation() const
    {
        return EgoVars.CameraLocation;
    }
    const FRotator &GetCameraRotation() const
    {
        return EgoVars.CameraRotation;
    }
    float GetVehicleVelocity() const
    {
        return EgoVars.Velocity; // returns ego velocity in cm/s
    }
    const FVector &GetVehicleLocation() const
    {
        return EgoVars.VehicleLocation;
    }
    const FRotator &GetVehicleRotation() const
    {
        return EgoVars.VehicleRotation;
    }
    // focus
    const FString &GetFocusActorName() const
    {
        return FocusData.ActorNameTag;
    }
    const FVector &GetFocusActorPoint() const
    {
        return FocusData.HitPointAbsolute;
    }
    float GetFocusActorDistance() const
    {
        return FocusData.Distance;
    }
    const DReyeVR::UserInputs &GetUserInputs() const
    {
        return Inputs;
    }
    ////////////////////:SETTERS://////////////////////

    void UpdateCamera(const FVector &NewCameraLoc, const FRotator &NewCameraRot)
    {
        EgoVars.CameraLocation = NewCameraLoc;
        EgoVars.CameraRotation = NewCameraRot;
    }

    void UpdateVehicle(const FVector &NewVehicleLoc, const FRotator &NewVehicleRot)
    {
        EgoVars.VehicleLocation = NewVehicleLoc;
        EgoVars.VehicleRotation = NewVehicleRot;
    }

    void Update(int64_t NewTimestamp, const struct EyeTracker &NewEyeData, const struct EgoVariables &NewEgoVars,
                const struct FocusInfo &NewFocus, const struct UserInputs &NewInputs)
    {
        TimestampCarlaUE4 = NewTimestamp;
        EyeTrackerData = NewEyeData;
        EgoVars = NewEgoVars;
        FocusData = NewFocus;
        Inputs = NewInputs;
    }

    ////////////////////:SERIALIZATION://////////////////////
    void Read(std::ifstream &InFile)
    {
        /// CAUTION: make sure the order of writes/reads is the same
        ReadValue<int64_t>(InFile, TimestampCarlaUE4);
        EgoVars.Read(InFile);
        EyeTrackerData.Read(InFile);
        FocusData.Read(InFile);
        Inputs.Read(InFile);
    }

    void Write(std::ofstream &OutFile) const
    {
        /// CAUTION: make sure the order of writes/reads is the same
        WriteValue<int64_t>(OutFile, GetTimestampCarla());
        EgoVars.Write(OutFile);
        EyeTrackerData.Write(OutFile);
        FocusData.Write(OutFile);
        Inputs.Write(OutFile);
    }

    FString ToString() const
    {
        FString print;
        print += FString::Printf(TEXT("[DReyeVR]TimestampCarla:%ld,\n"), long(TimestampCarlaUE4));
        print += FString::Printf(TEXT("[DReyeVR]EyeTracker:%s,\n"), *EyeTrackerData.ToString());
        print += FString::Printf(TEXT("[DReyeVR]FocusInfo:%s,\n"), *FocusData.ToString());
        print += FString::Printf(TEXT("[DReyeVR]EgoVariables:%s,\n"), *EgoVars.ToString());
        print += FString::Printf(TEXT("[DReyeVR]UserInputs:%s,\n"), *Inputs.ToString());
        return print;
    }

  private:
    int64_t TimestampCarlaUE4; // Carla Timestamp (EgoSensor Tick() event) in milliseconds
    struct EyeTracker EyeTrackerData;
    struct EgoVariables EgoVars;
    struct FocusInfo FocusData;
    struct UserInputs Inputs;
};
}; // namespace DReyeVR

#endif