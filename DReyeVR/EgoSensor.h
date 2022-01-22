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
// for some reason the SRanipal code (v1.3.1.1) has an enum called "ERROR" in SRanipal/Public/SRanipal_Enums.h:28
// which is used in SRanipal/Private/SRanipal_Enums.cpp:50 & 62. However, it appears that Carla has its own #define for
// ERROR which then makes the compiler complain about multiple constants. The simplest *workaround* for this is to
// rename the ERROR in the above files to something like SR_ERROR or anything but a commonly used #define
#include "SRanipalEye_Framework.h"
#endif

#include "EgoSensor.generated.h"

class AEgoVehicle;

UCLASS()
class CARLAUE4_API AEgoSensor : public ADReyeVRSensor
{
    GENERATED_BODY()

  public:
    AEgoSensor(const FObjectInitializer &ObjectInitializer);

    void PrePhysTick(float DeltaSeconds);

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
    ViveSR::anipal::Eye::EyeData *EyeData;    // SRanipal_Eyes_Enums.h
    bool bSRanipalEnabled;                    // Whether or not the framework has been loaded
#endif
    struct DReyeVR::EyeTracker EyeSensorData; // data from eye tracker
    struct DReyeVR::FocusInfo FocusInfoData;  // data from the focus computed from eye gaze
    int64_t DeviceTickStartTime;              // reference timestamp (ms) since the eye tracker started ticking
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