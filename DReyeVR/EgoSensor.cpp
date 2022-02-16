#include "EgoSensor.h"

#include "Carla/Game/CarlaStatics.h"    // GetEpisode
#include "DReyeVRUtils.h"               // ReadConfigValue, ComputeClosestToRayIntersection
#include "Kismet/KismetMathLibrary.h"   // Sin, Cos, Normalize
#include "UObject/UObjectBaseUtility.h" // GetName

#ifdef _WIN32
#include <windows.h> // required for file IO in Windows
#endif

#include <string>

#ifndef NO_DREYEVR_EXCEPTIONS
#include <exception>
#include <typeinfo>

namespace carla
{
void throw_exception(const std::exception &e)
{
    // It should never reach this part.
    std::terminate();
}
} // namespace carla
#endif

AEgoSensor::AEgoSensor(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
    ReadConfigVariables();

    // Initialize the frame capture (constructor call to create USceneCaptureComponent2D)
    ConstructFrameCapture();
}

void AEgoSensor::ReadConfigVariables()
{
    ReadConfigValue("EgoSensor", "ActorRegistryID", EgoSensorID);
    ReadConfigValue("EgoSensor", "StreamSensorData", bStreamData);
    ReadConfigValue("EgoSensor", "MaxTraceLenM", MaxTraceLenM);
    ReadConfigValue("EgoSensor", "DrawDebugFocusTrace", bDrawDebugFocusTrace);
    ReadConfigValue("EgoSensor", "RecordFrames", bCaptureFrameData);
    ReadConfigValue("EgoSensor", "FrameWidth", FrameCapWidth);
    ReadConfigValue("EgoSensor", "FrameHeight", FrameCapHeight);
    ReadConfigValue("EgoSensor", "FrameDir", FrameCapLocation);
    ReadConfigValue("EgoSensor", "FrameName", FrameCapFilename);
}

void AEgoSensor::BeginPlay()
{
    Super::BeginPlay();

    World = GetWorld();
    ChronoStartTime = std::chrono::system_clock::now();

    // Initialize the eye tracker hardware
    InitEyeTracker();

    // Set up frame capture (after world has been initialized)
    InitFrameCapture();

    // Register EgoSensor with the CarlaActorRegistry
    Register();

    UE_LOG(LogTemp, Log, TEXT("Initialized DReyeVR EgoSensor"));
}

void AEgoSensor::BeginDestroy()
{
    DestroyEyeTracker();
    Super::BeginDestroy();
}

void AEgoSensor::ManualTick(float DeltaSeconds)
{
    if (!bIsReplaying) // only update the sensor with local values if not replaying
    {
        const float Timestamp = int64_t(1000.f * UGameplayStatics::GetRealTimeSeconds(World));
        /// TODO: query the eye tracker hardware asynchronously (not limited to UE4 tick)
        TickEyeTracker();                             // query the eye-tracker hardware for current data
        ComputeTraceFocusInfo(ECC_GameTraceChannel4); // compute gaze focus data
        ComputeEgoVars();                             // get all necessary ego-vehicle data

        // Update the internal sensor data that gets handed off to Carla (for recording/replaying/PythonAPI)
        GetData()->Update(Timestamp,                  // TimestampCarla (ms)
                          EyeSensorData,              // EyeTrackerData
                          EgoVars,                    // EgoVehicleVariables
                          FocusInfoData,              // FocusData
                          Vehicle->GetVehicleInputs() // User inputs
        );
    }
    TickFrameCapture();
    TickCount++;
}

/// ========================================== ///
/// ---------------:EYETRACKER:--------------- ///
/// ========================================== ///

void AEgoSensor::InitEyeTracker()
{
#if USE_SRANIPAL_PLUGIN
    bSRanipalEnabled = false;
    // initialize SRanipal framework for eye tracking
    UE_LOG(LogTemp, Warning, TEXT("Attempting to use SRanipal eye tracking"));
    // Initialize the SRanipal eye tracker (WINDOWS ONLY)
    SRanipalFramework = SRanipalEye_Framework::Instance();
    SRanipal = SRanipalEye_Core::Instance();
    // no easily discernible difference between v1 and v2
    /// TODO: use the status output from StartFramework to determine if SRanipal loaded successfully
    int Status = SRanipalFramework->StartFramework(SupportedEyeVersion::version1);
    if (Status == SRanipalEye_Framework::FrameworkStatus::ERROR_SRANIPAL || // matches the patch_sranipal.sh script
        Status == SRanipalEye_Framework::FrameworkStatus::NOT_SUPPORT)
    {
        UE_LOG(LogTemp, Error, TEXT("Unable to start SRanipal framework!"));
        return;
    }
    // SRanipal->SetEyeParameter_() // can set the eye gaze jitter parameter
    // see SRanipal_Eyes_Enums.h
    // Get the reference timing to synchronize the SRanipal timer with Carla
    UE_LOG(LogTemp, Log, TEXT("Successfully started SRanipal framework"));
    bSRanipalEnabled = true;
#else
    UE_LOG(LogTemp, Warning, TEXT("Not using SRanipal eye tracking"));
#endif
}

void AEgoSensor::DestroyEyeTracker()
{
#if USE_SRANIPAL_PLUGIN
    if (SRanipalFramework)
    {
        SRanipalFramework->StopFramework();
        SRanipalEye_Framework::DestroyEyeFramework();
    }
    if (SRanipal)
        SRanipalEye_Core::DestroyEyeModule();
#endif
}

void AEgoSensor::TickEyeTracker()
{
    /// TODO: move this function to an async thread to obtain 120hz data capture
    auto Combined = &(EyeSensorData.Combined);
    auto Left = &(EyeSensorData.Left);
    auto Right = &(EyeSensorData.Right);
#if USE_SRANIPAL_PLUGIN
    if (bSRanipalEnabled)
    {
        /// NOTE: the GazeRay is the normalized direction vector of the actual gaze "ray"
        // Getting real eye tracker data
        check(SRanipal != nullptr);
        // Assigns EyeOrigin and Gaze direction (normalized) of combined gaze
        Combined->GazeValid = SRanipal->GetGazeRay(GazeIndex::COMBINE, Combined->GazeOrigin, Combined->GazeDir);
        // Assign Left/Right Gaze direction
        /// NOTE: the eye gazes are reversed bc SRanipal has a bug in their closed libraries
        // see: https://forum.vive.com/topic/9306-possible-bug-in-unreal-sdk-for-leftright-eye-gazes
        if (SRANIPAL_EYE_SWAP_FIXED) // if the latest SRanipal does not have this bug
        {
            Left->GazeValid = SRanipal->GetGazeRay(GazeIndex::LEFT, Left->GazeOrigin, Left->GazeDir);
            Right->GazeValid = SRanipal->GetGazeRay(GazeIndex::RIGHT, Right->GazeOrigin, Right->GazeDir);
        }
        else // this is the default case which we were dealing with during development
        {
            Left->GazeValid = SRanipal->GetGazeRay(GazeIndex::LEFT, Left->GazeOrigin, Right->GazeDir);
            Right->GazeValid = SRanipal->GetGazeRay(GazeIndex::RIGHT, Right->GazeOrigin, Left->GazeDir);
        }
        // Assign Eye openness
        Left->EyeOpennessValid = SRanipal->GetEyeOpenness(EyeIndex::LEFT, Left->EyeOpenness);
        Right->EyeOpennessValid = SRanipal->GetEyeOpenness(EyeIndex::RIGHT, Right->EyeOpenness);
        // Assign Pupil positions
        Left->PupilPositionValid = SRanipal->GetPupilPosition(EyeIndex::LEFT, Left->PupilPosition);
        Right->PupilPositionValid = SRanipal->GetPupilPosition(EyeIndex::RIGHT, Right->PupilPosition);

        // Get the "EyeData" which holds useful information such as the timestamp
        int EyeDataStatus = SRanipal->GetEyeData_(&EyeData);
        if (EyeDataStatus == ViveSR::Error::WORK)
        {
            EyeSensorData.TimestampDevice = EyeData.timestamp;
            EyeSensorData.FrameSequence = EyeData.frame_sequence;
            // Assign Pupil Diameters
            Left->PupilDiameter = EyeData.verbose_data.left.pupil_diameter_mm;
            Right->PupilDiameter = EyeData.verbose_data.right.pupil_diameter_mm;
        }
    }
    else
    {
        ComputeDummyEyeData();
    }
#else
    ComputeDummyEyeData();
#endif
    Combined->Vergence = ComputeVergence(Left->GazeOrigin, Left->GazeDir, Right->GazeOrigin, Right->GazeDir);
    // FPlatformProcess::Sleep(0.00833f); // use in async thread to get 120hz
}

void AEgoSensor::ComputeDummyEyeData()
{
    // Function to make "dummy" eye data where the eye gaze just looks around in a CCW circle.
    // Useful for when the eye data is unavailable (Plugin not initialized, on Linux, etc.)
    auto Combined = &(EyeSensorData.Combined);
    auto Left = &(EyeSensorData.Left);
    auto Right = &(EyeSensorData.Right);
    // generate dummy values bc no hardware sensor is present
    EyeSensorData.TimestampDevice = int64_t(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - ChronoStartTime)
            .count());
    EyeSensorData.FrameSequence = TickCount; // get the current tick

    // generate gaze that rotates in CCW fashion around the camera ray
    const float TimeNow = EyeSensorData.TimestampDevice / 1000.f;
    Combined->GazeDir.X = 5.0;
    Combined->GazeDir.Y = UKismetMathLibrary::Cos(TimeNow);
    Combined->GazeDir.Z = UKismetMathLibrary::Sin(TimeNow);
    UKismetMathLibrary::Vector_Normalize(Combined->GazeDir, 0.0001);

    // Assign the origin position to the (3D space) origin
    Combined->GazeValid = true; // for our Linux case, this is valid
    Combined->GazeOrigin = FVector::ZeroVector;

    // Assign the endpoint of the combined position (faked in Linux) to the left & right gazes too
    Left->GazeDir = Combined->GazeDir;
    Left->GazeOrigin = Combined->GazeOrigin + 5 * FVector::LeftVector;
    Right->GazeDir = Combined->GazeDir;
    Right->GazeOrigin = Combined->GazeOrigin + 5 * FVector::RightVector;
}

void AEgoSensor::ComputeTraceFocusInfo(const ECollisionChannel TraceChannel, float TraceRadius)
{
    const float TraceLen = MaxTraceLenM * 100.f; // convert to m from cm
    const FRotator &WorldRot = GetData()->GetCameraRotationAbs();
    const FVector &WorldPos = GetData()->GetCameraLocationAbs();
    const FVector GazeOrigin = WorldPos + WorldRot.RotateVector(GetData()->GetGazeOrigin());
    const FVector GazeRay = TraceLen * WorldRot.RotateVector(GetData()->GetGazeDir());
    // Create collision information container.
    FCollisionQueryParams TraceParam;
    TraceParam = FCollisionQueryParams(FName("TraceParam"), true);
    TraceParam.AddIgnoredActor(Vehicle); // don't collide with the vehicle since that would be useless
    TraceParam.bTraceComplex = true;
    TraceParam.bReturnPhysicalMaterial = false;
    FHitResult Hit(EForceInit::ForceInit);
    bool bDidHit = false;

    // 0 for a point, >0 for a sphear trace
    TraceRadius = FMath::Max(TraceRadius, 0.f); // clamp to be positive

    if (TraceRadius == 0.f) // Single ray/line trace
    {
        bDidHit = World->LineTraceSingleByChannel(Hit, GazeOrigin, GazeOrigin + GazeRay, TraceChannel, TraceParam);
    }
    else // Sphear line trace
    {
        FCollisionShape Sphear = FCollisionShape();
        Sphear.SetSphere(TraceRadius);
        bDidHit = World->SweepSingleByChannel(Hit, GazeOrigin, GazeOrigin + GazeRay, FQuat(0.f, 0.f, 0.f, 0.f),
                                              TraceChannel, Sphear, TraceParam);
    }
    // Update fields
    FString ActorName = "None";
    if (Hit.Actor != nullptr)
        Hit.Actor->GetName(ActorName);
    // update internal data structure (see DReyeVRData::FocusInfo for default constructor)
    FocusInfoData = {
        Hit.Actor,    // pointer to actor being hit (if any, else nullptr)
        Hit.Location, // absolute (world) location of hit
        Hit.Normal,   // normal of hit surface (if hit)
        ActorName,    // name of the actor being hit (if any, else "None")
        Hit.Distance, // distance from ray start
        bDidHit,      // whether or not there was a hit
    };
    if (bDrawDebugFocusTrace)
    {
        DrawDebugSphere(World, FocusInfoData.HitPoint, 8.0f, 30, FColor::Blue);
        DrawDebugLine(World,
                      GazeOrigin,           // start line
                      GazeOrigin + GazeRay, // end line
                      FColor::Purple, false, -1, 0, 1);
    }
}

float AEgoSensor::ComputeVergence(const FVector &L0, const FVector &LDir, const FVector &R0, const FVector &RDir) const
{
    // Compute length of ray-to- intersection of the left and right eye gazes in 3D space (length in centimeters)
    return ComputeClosestToRayIntersection(L0, LDir, R0, RDir).Size(); // units are cm (default for UE4)
}

/// ========================================== ///
/// ---------------:EGOVARS:------------------ ///
/// ========================================== ///

void AEgoSensor::SetEgoVehicle(class AEgoVehicle *NewEgoVehicle)
{
    Vehicle = NewEgoVehicle;
    Camera = Vehicle->GetCamera();
    check(Vehicle);
}

void AEgoSensor::ComputeEgoVars()
{
    // See DReyeVRData::EgoVariables
    EgoVars.VehicleLocation = Vehicle->GetActorLocation();
    EgoVars.VehicleRotation = Vehicle->GetActorRotation();
    EgoVars.CameraLocation = Camera->GetRelativeLocation();
    EgoVars.CameraRotation = Camera->GetRelativeRotation();
    EgoVars.CameraLocationAbs = Camera->GetComponentLocation();
    EgoVars.CameraRotationAbs = Camera->GetComponentRotation();
    EgoVars.Velocity = Vehicle->GetVehicleForwardSpeed();
}

/// ========================================== ///
/// ---------------:FRAMECAP:----------------- ///
/// ========================================== ///

void AEgoSensor::ConstructFrameCapture()
{
    if (bCaptureFrameData)
    {
        // Frame capture
        CaptureRenderTarget = CreateDefaultSubobject<UTextureRenderTarget2D>(TEXT("CaptureRenderTarget_DReyeVR"));
        CaptureRenderTarget->CompressionSettings = TextureCompressionSettings::TC_Default;
        CaptureRenderTarget->SRGB = false;
        CaptureRenderTarget->bAutoGenerateMips = false;
        CaptureRenderTarget->bGPUSharedFlag = true;
        CaptureRenderTarget->ClearColor = FLinearColor::Black;
        CaptureRenderTarget->UpdateResourceImmediate(true);
        // CaptureRenderTarget->OverrideFormat = EPixelFormat::PF_FloatRGB;
        CaptureRenderTarget->AddressX = TextureAddress::TA_Clamp;
        CaptureRenderTarget->AddressY = TextureAddress::TA_Clamp;
        const bool bInForceLinearGamma = false;
        CaptureRenderTarget->InitCustomFormat(FrameCapWidth, FrameCapHeight, PF_B8G8R8A8, bInForceLinearGamma);
        check(CaptureRenderTarget->GetSurfaceWidth() > 0 && CaptureRenderTarget->GetSurfaceHeight() > 0);

        FrameCap = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("FrameCap"));
        FrameCap->SetupAttachment(Camera);
        FrameCap->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
        FrameCap->bCaptureOnMovement = false;
        FrameCap->bCaptureEveryFrame = false;
        FrameCap->bAlwaysPersistRenderingState = true;
        FrameCap->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

        FrameCap->Deactivate();
        FrameCap->TextureTarget = CaptureRenderTarget;
        FrameCap->UpdateContent();
        FrameCap->Activate();
    }
    if (FrameCap && !bCaptureFrameData)
    {
        FrameCap->Deactivate();
    }
}

void AEgoSensor::InitFrameCapture()
{
    if (bCaptureFrameData)
    {
        // create out dir
        /// TODO: add check for absolute paths
        FrameCapLocation = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + FrameCapLocation);
        UE_LOG(LogTemp, Log, TEXT("Outputting frame capture data to %s"), *FrameCapLocation);
        IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        if (!PlatformFile.DirectoryExists(*FrameCapLocation))
        {
#ifndef _WIN32
            // this only seems to work on Unix systems, else CreateDirectoryW is not linked?
            PlatformFile.CreateDirectory(*FrameCapLocation);
#else
            // using Windows system calls
            CreateDirectory(*FrameCapLocation, NULL);
#endif
        }
    }
}

void AEgoSensor::TickFrameCapture()
{
    // frame capture
    if (bCaptureFrameData && FrameCap && Camera)
    {
        FMinimalViewInfo DesiredView;
        Camera->GetCameraView(0, DesiredView);
        FrameCap->SetCameraView(DesiredView); // move camera to the Camera view
        FrameCap->CaptureScene();             // also available: CaptureSceneDeferred()
        const FString Suffix = FString::Printf(TEXT("%04d.png"), TickCount);
        SaveFrameToDisk(*CaptureRenderTarget, FPaths::Combine(FrameCapLocation, FrameCapFilename + Suffix));
    }
}

/// ========================================== ///
/// -----------------:OTHER:------------------ ///
/// ========================================== ///

void AEgoSensor::Register()
{
    // Register EgoSensor with ActorRegistry
    FCarlaActor::IdType ID = EgoSensorID;
    FActorDescription SensorDescr;
    SensorDescr.Id = "sensor.dreyevr.dreyevrsensor";
    FString RegistryTags = "EgoSensor,DReyeVR";
    UCarlaStatics::GetCurrentEpisode(World)->RegisterActor(*this, SensorDescr, RegistryTags, ID);
}