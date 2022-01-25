#pragma once

#include "Carla/Sensor/DReyeVRData.h"           // DReyeVR namespace
#include "Carla/Sensor/DReyeVRSensor.h"         // ADReyeVRSensor
#include "Components/SceneCaptureComponent2D.h" // USceneCaptureComponent2D
#include "EgoVehicle.h"                         // AEgoVehicle;
#include <chrono>                               // timing threads
#include <cstdint>

// #define USE_SRANIPAL_PLUGIN true // handled in .Build.cs file
#define SRANIPAL_EYE_SWAP_FIXED false // as of v1.3.1.1 this bug is unfortunately still present

#ifndef _WIN32
// unset the #define if on anything other than Windows bc the SRanipal libraries only work on Windows :(
#undef USE_SRANIPAL_PLUGIN
#define USE_SRANIPAL_PLUGIN false
#endif

#if USE_SRANIPAL_PLUGIN
/// NOTE: Can only use SRanipal on Windows machines
#include "SRanipalEye.h"      // SRanipal Module Framework
#include "SRanipalEye_Core.h" // SRanipal Eye Tracker
#include "ViveSR_Enums.h"     // ViveSR::Error::WORK
// Make sure to patch sranipal before using it here!
#include "SRanipalEye_Framework.h" // StartFramework
#endif

#include "EgoSensor.generated.h"

class AEgoVehicle;

UCLASS()
class CARLAUE4_API AEgoSensor : public ADReyeVRSensor
{
    GENERATED_BODY()

  public:
    AEgoSensor(const FObjectInitializer &ObjectInitializer);

    void ManualTick(float DeltaSeconds); // Tick called explicitly from DReyeVR EgoVehicle

    void SetEgoVehicle(class AEgoVehicle *EgoVehicle); // provide access to EgoVehicle (and by extension its camera)

  protected:
    void BeginPlay();
    void BeginDestroy();

    class UWorld *World; // to get info about the world: time, frames, etc.

  private:
    int64_t TickCount = 0; // how many ticks have been executed
    void ReadConfigVariables();

    ////////////////:EYETRACKER:////////////////
    void InitEyeTracker();
    void DestroyEyeTracker();
    void ComputeDummyEyeData(); // when no hardware sensor is present
    void TickEyeTracker();      // tick hardware sensor
    void ComputeTraceFocusInfo(const ECollisionChannel TraceChannel, float TraceRadius = 0.f);
    float ComputeVergence(const FVector &L0, const FVector &LDir, const FVector &R0, const FVector &RDir) const;
#if USE_SRANIPAL_PLUGIN
    SRanipalEye_Core *SRanipal;               // SRanipalEye_Core.h
    SRanipalEye_Framework *SRanipalFramework; // SRanipalEye_Framework.h
    ViveSR::anipal::Eye::EyeData EyeData;     // SRanipal_Eyes_Enums.h
    bool bSRanipalEnabled;                    // Whether or not the framework has been loaded
#endif
    struct DReyeVR::EyeTracker EyeSensorData;                           // data from eye tracker
    struct DReyeVR::FocusInfo FocusInfoData;                            // data from the focus computed from eye gaze
    std::chrono::time_point<std::chrono::system_clock> ChronoStartTime; // std::chrono time at BeginPlay

    ////////////////:EGOVARS:////////////////
    void ComputeEgoVars();
    class AEgoVehicle *Vehicle;           // the DReyeVR EgoVehicle
    struct DReyeVR::EgoVariables EgoVars; // data from vehicle that is getting tracked

    ////////////////:FRAMECAPTURE:////////////////
    void ConstructFrameCapture(); // needs to be called in the constructor
    void InitFrameCapture();      // needs to be called in BeginPlay
    void TickFrameCapture();
    class UCameraComponent *Camera; // for frame capture views
    class UTextureRenderTarget2D *CaptureRenderTarget = nullptr;
    class USceneCaptureComponent2D *FrameCap = nullptr;
    FString FrameCapLocation; // relative to game dir
    FString FrameCapFilename; // gets ${tick}.png suffix
    int FrameCapWidth;
    int FrameCapHeight;
    bool bCaptureFrameData;

    ////////////////:OTHER:////////////////
    int EgoSensorID;
    void Register();
};