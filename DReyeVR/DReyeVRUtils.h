#ifndef DREYEVR_UTIL
#define DREYEVR_UTIL

#include "Carla/Sensor/ShaderBasedSensor.h" // FSensorShader
#include "ConfigFile.h"                     // ConfigFile class
#include "CoreMinimal.h"
#include "Engine/Texture2D.h"              // UTexture2D
#include "HighResScreenshot.h"             // FHighResScreenshotConfig
#include "ImageWriteQueue.h"               // TImagePixelData
#include "ImageWriteTask.h"                // FImageWriteTask
#include <carla/image/CityScapesPalette.h> // CityScapesPalette
#include <fstream>                         // std::ifstream
#include <sstream>                         // std::istringstream
#include <string>
#include <unordered_map>

template <typename T>
T *SafePtrGet(const FString &Name, TWeakObjectPtr<T> &Ptr, const std::function<void(void)> &RemedyFunction)
{
    if (Ptr.IsValid())
        return Ptr.Get();
    // object was destroyed! possibly by external process (ex. map change)
    if (!Ptr.IsExplicitlyNull())
    { // dangling pointer!!
        LOG_WARN("Dangling pointer \"%s\" (%p) is invalid! Attempting to remedy", Ptr.Get(), *Name);
    }
    RemedyFunction();
    // try to remedy
    if (Ptr.IsValid())
        return Ptr.Get();
    LOG_ERROR("Unable to remedy (%s)", *Name);
    return nullptr;
}

// instead of vehicle.dreyevr.model3 or sensor.dreyevr.ego_sensor, we use "harplab" for category
// => harplab.dreyevr_vehicle.model3 & harplab.dreyevr_sensor.ego_sensor
// in PythonAPI use world.get_actors().filter("harplab.dreyevr_vehicle.*") or
// world.get_blueprint_library().filter("harplab.dreyevr_sensor.*") and you won't accidentally get these actors when
// performing filter("vehicle.*") or filter("sensor.*")
static const FString DReyeVRCategory("HarpLab");

template <typename T>
T *SafePtrGet(const FString &Name, TWeakObjectPtr<T> &Ptr, const std::function<void(void)> &RemedyFunction)
{
    if (Ptr.IsValid())
        return Ptr.Get();
    // object was destroyed! possibly by external process (ex. map change)
    if (!Ptr.IsExplicitlyNull())
    { // dangling pointer!!
        LOG_WARN("Dangling pointer \"%s\" (%p) is invalid! Attempting to remedy", Ptr.Get(), *Name);
    }
    RemedyFunction();
    // try to remedy
    if (Ptr.IsValid())
        return Ptr.Get();
    LOG_ERROR("Unable to remedy (%s)", *Name);
    return nullptr;
}

static FString UE4RefToClassPath(const FString &UE4ReferencePath)
{
    // converts (reference) strings of the type "Type'/Game/PATH/asset.asset'" to "/Game/PATH/asset.asset_C"
    // for use in ConstructorHelpers::FClassFinder<UObject>
    const FString NoneStr = FString(""); // replace with empty string ("")
    const FString SingleQuoteStr = FString("'");
    // find the start position in the string (ignore the type (Blueprint, SkeletalMesh, Skeleton, AnimBP, etc.))
    const int StartPos = UE4ReferencePath.Find(SingleQuoteStr, ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
    FString Ret = UE4ReferencePath.RightChop(StartPos);
    Ret.ReplaceInline(*SingleQuoteStr, *NoneStr, ESearchCase::CaseSensitive);
    Ret += "_C"; // to force class type suffix
    return Ret;
}

static FString CleanNameForDReyeVR(const FString &RawName)
{
    // should be equivalent to GetClass()->GetDisplayNameText().ToString()
    // for our purposes (spawning different type EgoVehicles)
    FString CleanName = RawName;
    CleanName.RemoveSpacesInline(); // one word
#define DELETE_INLINE(x) CleanName.ReplaceInline(*FString(x), *FString(""), ESearchCase::CaseSensitive);
    DELETE_INLINE("BP_");   // might start w/ BP_XYZ
    DELETE_INLINE("BP");    // might start w/ BPXYZ
    DELETE_INLINE("_C");    // might end with _C
    DELETE_INLINE("Ego");   // default object is EgoVehicle
    DELETE_INLINE("SKEL_"); // skeleton class starts with SKEL_

    return CleanName;
}

static FActorDefinition FindDefnInRegistry(const UCarlaEpisode *Episode, const UClass *ClassType)
{
    // searches through the registers actors (definitions) to find one with the matching class type
    check(Episode != nullptr);

    FActorDefinition FoundDefinition;
    bool bFoundDef = false;
    for (const auto &Defn : Episode->GetActorDefinitions())
    {
        if (Defn.Class == ClassType)
        {
            LOG("Found appropriate definition registered at UId: %d as \"%s\"", Defn.UId, *Defn.Id);
            FoundDefinition = Defn;
            bFoundDef = true;
            break; // assumes the first is the ONLY one matching this class (Ex. EgoVehicle, EgoSensor)
        }
    }
    if (!bFoundDef)
    {
        LOG_ERROR("Unable to find appropriate definition in registry!");
    }
    return FoundDefinition;
}

static FActorDefinition FindEgoVehicleDefinition(const UCarlaEpisode *Episode)
{

    FString LoadVehicle = "TeslaM3"; // default vehicle
    if (GeneralParams.Get<FString>("EgoVehicle", "VehicleType", LoadVehicle))
    {
        LOG("Loading new default EgoVehicle: \"%s\"", *LoadVehicle);
    }
    // searches through the registers actors (definitions) to find one with the matching class type
    check(Episode != nullptr);

    FActorDefinition FoundDefinition;
    bool bFoundDef = false;
    for (const auto &Defn : Episode->GetActorDefinitions())
    {
        const auto &LowerId = Defn.Id.ToLower(); // perform string comparisons on lowercase (ignore case)
        // contains both the DReyeVR category (HarpLab) and specific EgoVehicle
        if (LowerId.Contains(DReyeVRCategory.ToLower()) && LowerId.Contains(LoadVehicle.ToLower()))
        {
            LOG("Found appropriate definition for \"%s\" registered at UId: %d as \"%s\"", //
                *LoadVehicle, Defn.UId, *Defn.Id);
            FoundDefinition = Defn;
            bFoundDef = true;
            break; // assumes the first is the ONLY one matching this class (Ex. EgoVehicle, EgoSensor)
        }
    }
    if (!bFoundDef)
    {
        LOG_ERROR("Unable to find appropriate definition in registry!");
    }
    return FoundDefinition;
}

static FVector ComputeClosestToRayIntersection(const FVector &L0, const FVector &LDir, const FVector &R0,
                                               const FVector &RDir)
{
    // Recall that a 'line' can be defined as (L = origin(0) + t * direction(Dir)) for some t

    // Calculating shortest line segment intersecting both lines
    // Implementation sourced from http://paulbourke.net/geometry/pointlineplane/
    FVector L0R0 = L0 - R0; // segment between L origin and R origin
    if (L0R0.Size() == 0.f) // same origin
        return FVector::ZeroVector;
    const float epsilon = 0.00001f; // small positive real number

    // Calculating dot-product equation to find perpendicular shortest-line-segment
    float d1343 = L0R0.X * RDir.X + L0R0.Y * RDir.Y + L0R0.Z * RDir.Z;
    float d4321 = RDir.X * LDir.X + RDir.Y * LDir.Y + RDir.Z * LDir.Z;
    float d1321 = L0R0.X * LDir.X + L0R0.Y * LDir.Y + L0R0.Z * LDir.Z;
    float d4343 = RDir.X * RDir.X + RDir.Y * RDir.Y + RDir.Z * RDir.Z;
    float d2121 = LDir.X * LDir.X + LDir.Y * LDir.Y + LDir.Z * LDir.Z;
    float denom = d2121 * d4343 - d4321 * d4321;
    if (abs(denom) < epsilon)
        return FVector::ZeroVector; // no intersection, would cause div by 0 err
    float numer = d1343 * d4321 - d1321 * d4343;

    // calculate scalars (mu) that scale the unit direction XDir to reach the desired points
    float muL = numer / denom;                   // variable scale of direction vector for LEFT ray
    float muR = (d1343 + d4321 * (muL)) / d4343; // variable scale of direction vector for RIGHT ray

    // calculate the points on the respective rays that create the intersecting line
    FVector ptL = L0 + muL * LDir; // the point on the Left ray
    FVector ptR = R0 + muR * RDir; // the point on the Right ray

    FVector ShortestLineSeg = ptL - ptR; // the shortest line segment between the two rays
    // calculate the vector between the middle of the two endpoints and return its magnitude
    FVector ptM = (ptL + ptR) / 2.0f; // middle point between two endpoints of shortest-line-segment
    FVector oM = (L0 + R0) / 2.0f;    // midpoint between two (L & R) origins
    return ptM - oM;                  // Combined ray between midpoints of endpoints
}

static void GenerateSquareImage(TArray<FColor> &Src, const float Size, const FColor &Colour)
{
    // Used to initialize any grid-based image onto an array
    Src.Reserve(Size * Size); // allocate width*height space
    for (int i = 0; i < Size; i++)
    {
        for (int j = 0; j < Size; j++)
        {
            // RGBA colours
            FColor PixelColour;
            const int Thickness = Size / 10;
            bool LeftOrRight = (i < Thickness || i > Size - Thickness);
            bool TopOrBottom = (j < Thickness || j > Size - Thickness);
            if (LeftOrRight || TopOrBottom)
                PixelColour = Colour; // (semi-opaque red)
            else
                PixelColour = FColor(0, 0, 0, 0); // (fully transparent inside)
            Src.Add(PixelColour);
        }
    }
}

static void GenerateCrosshairImage(TArray<FColor> &Src, const float Size, const FColor &Colour)
{
    // Used to initialize any bitmap-based image that will be used as a
    Src.Reserve(Size * Size); // allocate width*height space
    for (int i = 0; i < Size; i++)
    {
        for (int j = 0; j < Size; j++)
        {
            // RGBA colours
            FColor PixelColour;

            const int x = i - Size / 2;
            const int y = j - Size / 2;
            const float Radius = Size / 3.f;
            const int RadThickness = 3 * Size / 100.f;
            const int LineLen = 4 * RadThickness;
            const float RadLo = Radius - LineLen;
            const float RadHi = Radius + LineLen;
            bool BelowRadius = (FMath::Square(x) + FMath::Square(y) <= FMath::Square(Radius + RadThickness));
            bool AboveRadius = (FMath::Square(x) + FMath::Square(y) >= FMath::Square(Radius - RadThickness));
            if (BelowRadius && AboveRadius)
                PixelColour = Colour; // (semi-opaque red)
            else
            {
                // Draw little rectangular markers
                const bool RightMarker = (RadLo < x && x < RadHi) && std::fabs(y) < RadThickness;
                const bool LeftMarker = (RadLo < -x && -x < RadHi) && std::fabs(y) < RadThickness;
                const bool TopMarker = (RadLo < y && y < RadHi) && std::fabs(x) < RadThickness;
                const bool BottomMarker = (RadLo < -y && -y < RadHi) && std::fabs(x) < RadThickness;
                if (RightMarker || LeftMarker || TopMarker || BottomMarker)
                    PixelColour = Colour; // (semi-opaque red)
                else
                    PixelColour = FColor(0, 0, 0, 0); // (fully transparent inside)
            }

            Src.Add(PixelColour);
        }
    }
}

static float CmPerSecondToXPerHour(const bool MilesPerHour)
{
    // convert cm/s to X/h
    // X = miles if MilesPerHour == true, else X = KM
    if (MilesPerHour)
    {
        return 0.0223694f;
    }
    return 0.036f;
}

static void SaveFrameToDisk(UTextureRenderTarget2D &RenderTarget, const FString &FilePath, const bool FileFormatJPG)
{
    FTextureRenderTargetResource *RTResource = RenderTarget.GameThread_GetRenderTargetResource();
    const size_t H = RenderTarget.GetSurfaceHeight();
    const size_t W = RenderTarget.GetSurfaceWidth();
    const FIntPoint DestSize(W, H);
    TImagePixelData<FColor> PixelData(DestSize);

    // Read pixels into array
    // heavily inspired by Carla's Carla/Sensor/PixelReader.cpp:WritePixelsToArray function
    TArray<FColor> Pixels;
    Pixels.AddUninitialized(H * W);
    FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);
    ReadPixelFlags.SetLinearToGamma(true);
    if (RTResource == nullptr)
    {
        LOG_ERROR("Missing render target!");
        return;
    }
    if (!RTResource->ReadPixels(Pixels, ReadPixelFlags))
        LOG_ERROR("Unable to read pixels!");

    // dump pixel array to disk
    PixelData.Pixels = Pixels;
    TUniquePtr<FImageWriteTask> ImageTask = MakeUnique<FImageWriteTask>();
    ImageTask->PixelData = MakeUnique<TImagePixelData<FColor>>(PixelData);
    ImageTask->Filename = FilePath;
    // LOG("Saving screenshot to %s", *FilePath);
    ImageTask->Format = FileFormatJPG ? EImageFormat::JPEG : EImageFormat::PNG; // lower quality, less storage
    ImageTask->CompressionQuality = (int32)EImageCompressionQuality::Default;
    ImageTask->bOverwriteFile = true;
    ImageTask->PixelPreProcessors.Add(TAsyncAlphaWrite<FColor>(255));
    FHighResScreenshotConfig &HighResScreenshotConfig = GetHighResScreenshotConfig();
    HighResScreenshotConfig.ImageWriteQueue->Enqueue(MoveTemp(ImageTask));
}

static UTexture2D *CreateTexture2DFromArray(const TArray<FColor> &Contents)
{
    const size_t Size = std::sqrt(Contents.Num());
    ensure(Size * Size == Contents.Num());
    UTexture2D *Texture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
    void *TextureData = Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(TextureData, Contents.GetData(), 4 * Contents.Num());
    Texture->PlatformData->Mips[0].BulkData.Unlock();
    Texture->UpdateResource();
    check(Texture);
    return Texture;
}

/// ========================================== ///
/// ----------------:SHADER:------------------ ///
/// ========================================== ///

static FSensorShader InitSemanticSegmentationShader(class UObject *Parent = nullptr)
{
    const FString Path =
        "Material'/Carla/PostProcessingMaterials/DReyeVR_SemanticSegmentation.DReyeVR_SemanticSegmentation'";
    UMaterial *MaterialFound = LoadObject<UMaterial>(nullptr, *Path);
    check(MaterialFound != nullptr);
    UMaterialInstanceDynamic *SemanticSegmentationMaterial =
        UMaterialInstanceDynamic::Create(MaterialFound, Parent, FName(TEXT("DReyeVR_SemanticSegmentationShader")));

    // create the array used for tag-colour segmentation
    TArray<FColor> TextureSrc;
    const size_t NumTags = carla::image::CityScapesPalette::GetNumberOfTags();
    const int TexSize = 256; // making this array a 16x16=256 length 2d array that holds the raw colours
    TextureSrc.Reserve(TexSize);
    for (int i = 0; i < TexSize; i++)
    {
        if (i < NumTags) // fill the first n (NumTags) with the tags directly
        {
            auto Colour = carla::image::CityScapesPalette::GetColor(i);
            TextureSrc.Add(FColor(Colour[0], Colour[1], Colour[2], 255));
        }
        else // fill the overflow with black
            TextureSrc.Add(FColor::Black);
    }

    UTexture2D *TagColourTexture = CreateTexture2DFromArray(TextureSrc);

    // update the tagger-colour matrix param so all the sampled colours are from the CITYSCAPES_PALETTE_MAP
    // defined in LibCarla/source/carla/image/CityScapesPalette.h
    SemanticSegmentationMaterial->SetTextureParameterValue("TagColours", TagColourTexture);
    return FSensorShader{SemanticSegmentationMaterial, 1.f};
}

static FSensorShader InitDepthShader(class UObject *Parent = nullptr)
{
    const FString Path = "Material'/Carla/PostProcessingMaterials/DReyeVR_DepthEffect.DReyeVR_DepthEffect'";
    UMaterial *MaterialFound = LoadObject<UMaterial>(nullptr, *Path);
    check(MaterialFound != nullptr);
    UMaterialInstanceDynamic *DepthMaterial =
        UMaterialInstanceDynamic::Create(MaterialFound, Parent, FName(TEXT("DReyeVR_DepthShader")));
    return FSensorShader{DepthMaterial, 1.f};
}

/// ========================================== ///
/// ------------:POSTPROCESSING:-------------- ///
/// ========================================== ///

// collection of shader factory functions so shaders can be easily regenerated at runtime (useful when GC'd)
static std::vector<std::function<FPostProcessSettings()>> ShaderFactory = {};

static size_t GetNumberOfShaders()
{
    return ShaderFactory.size();
}

static FPostProcessSettings CreatePostProcessingParams(const std::vector<FSensorShader> &Shaders)
{
    // modifying from here: https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/Engine/FPostProcessSettings/
    FPostProcessSettings PP;
    PP.bOverride_VignetteIntensity = true;
    PP.VignetteIntensity = GeneralParams.Get<float>("CameraParams", "VignetteIntensity");

    PP.bOverride_ScreenPercentage = true;
    PP.ScreenPercentage = GeneralParams.Get<float>("CameraParams", "ScreenPercentage");

    PP.bOverride_BloomIntensity = true;
    PP.BloomIntensity = GeneralParams.Get<float>("CameraParams", "BloomIntensity");

    PP.bOverride_SceneFringeIntensity = true;
    PP.SceneFringeIntensity = GeneralParams.Get<float>("CameraParams", "SceneFringeIntensity");

    PP.bOverride_LensFlareIntensity = true;
    PP.LensFlareIntensity = GeneralParams.Get<float>("CameraParams", "LensFlareIntensity");

    PP.bOverride_GrainIntensity = true;
    PP.GrainIntensity = GeneralParams.Get<float>("CameraParams", "GrainIntensity");

    PP.bOverride_MotionBlurAmount = true;
    PP.MotionBlurAmount = GeneralParams.Get<float>("CameraParams", "MotionBlurIntensity");

    // append shaders to this postprocess effect
    for (const FSensorShader &ShaderInfo : Shaders)
    {
        ensure(ShaderInfo.PostProcessMaterial != nullptr);
        PP.AddBlendable(ShaderInfo.PostProcessMaterial, ShaderInfo.Weight);
    }

    return PP;
}

static void InitShaderFactory()
{
    // initializes the static (global) ShaderFactory container with the factory functions
    // to generate the shaders defined here.

    ShaderFactory.clear(); // clear all old shaders
    ShaderFactory = {};
// helper lambda #define to reduce boilerplate code
#define SHADER_LAMBDA(x) []() { return CreatePostProcessingParams(x); }
    // denote the order of the shaders that we will use as lambdas to create their shader
    ShaderFactory.push_back(SHADER_LAMBDA({}));                                 // rgb (no postprocessing)
    ShaderFactory.push_back(SHADER_LAMBDA({InitSemanticSegmentationShader()})); // semantics
    ShaderFactory.push_back(SHADER_LAMBDA({InitDepthShader()}));                // depth

    /// TODO: add more shaders here
    /// TODO: use enum for shaders
}

static FPostProcessSettings CreatePostProcessingEffect(size_t Idx)
{
    if (GetNumberOfShaders() == 0)
    {
        InitShaderFactory();
        ensure(GetNumberOfShaders() > 0);
    }
    // check the index is valid, and call the shader factory function to be used immediately
    Idx = std::min(Idx, GetNumberOfShaders() - 1);
    /// NOTE: this can be slow (as it needs to load objects (shaders) from disk and potentially recompile them),
    // so be wary of using this in a performance-critical section
    return ShaderFactory[Idx]();
}

static FHitResult SimpleRayTrace(const UWorld *World, const FVector &Start, const FVector &End,
                                 const std::vector<const AActor *> &Ignored = {})
{
    // run a trace from the start to the end to sample visibility channel
    FCollisionQueryParams TraceParam;
    TraceParam = FCollisionQueryParams(FName("RayTrace"), true);
    for (const AActor *A : Ignored)
    {
        TraceParam.AddIgnoredActor(A);
    }
    TraceParam.bTraceComplex = true;
    TraceParam.bReturnPhysicalMaterial = false;
    FHitResult Hit(EForceInit::ForceInit);
    World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, TraceParam);
    DrawDebugLine(World,
                  Start, // start line
                  End,   // end line
                  FColor::Green, false, -1, 0, 1);
    return Hit;
}

#endif