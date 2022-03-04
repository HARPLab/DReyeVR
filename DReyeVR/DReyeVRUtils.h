#ifndef DREYEVR_UTIL
#define DREYEVR_UTIL

#include "CoreMinimal.h"
#include "HighResScreenshot.h" // FHighResScreenshotConfig
#include "ImageWriteQueue.h"   // TImagePixelData
#include "ImageWriteTask.h"    // FImageWriteTask
#include <fstream>             // std::ifstream
#include <sstream>             // std::istringstream
#include <string>
#include <unordered_map>

/// this is the file where we'll read all DReyeVR specific configs
static const FString ConfigFilePath =
    FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()), TEXT("Config"), TEXT("DReyeVRConfig.ini"));

static std::unordered_map<std::string, FString> Params = {};

static std::string CreateVariableName(const std::string &Section, const std::string &Variable)
{
    return Section + "/" + Variable; // encoding the variable alongside its section
}
static std::string CreateVariableName(const FString &Section, const FString &Variable)
{
    return CreateVariableName(std::string(TCHAR_TO_UTF8(*Section)), std::string(TCHAR_TO_UTF8(*Variable)));
}

static void ReadDReyeVRConfig()
{
    /// TODO: add feature to "hot-reload" new params during runtime
    UE_LOG(LogTemp, Warning, TEXT("Reading config from %s"), *ConfigFilePath);
    /// performs a single pass over the config file to collect all variables into Params
    std::ifstream ConfigFile(TCHAR_TO_ANSI(*ConfigFilePath));
    if (ConfigFile)
    {
        std::string Line;
        std::string Section = "";
        while (std::getline(ConfigFile, Line))
        {
            // std::string stdKey = std::string(TCHAR_TO_UTF8(*Key));
            if (Line[0] == ';') // ignore comments
                continue;
            std::istringstream iss_Line(Line);
            if (Line[0] == '[') // test section
            {
                std::getline(iss_Line, Section, ']');
                Section = Section.substr(1); // skip leading '['
                continue;
            }
            std::string Key;
            if (std::getline(iss_Line, Key, '=')) // gets left side of '=' into FileKey
            {
                std::string Value;
                if (std::getline(iss_Line, Value, ';')) // gets left side of ';' for comments
                {
                    std::string VariableName = CreateVariableName(Section, Key);
                    FString VariableValue = FString(Value.c_str());
                    Params[VariableName] = VariableValue;
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Unable to open the config file %s"), *ConfigFilePath);
    }
    // for (auto &e : Params){
    //     UE_LOG(LogTemp, Warning, TEXT("%s: %s"), *FString(e.first.c_str()), *e.second);
    // }
}

static void EnsureConfigsUpdated()
{
    // used to ensure the configs file has been read and contents updated
    if (Params.size() == 0)
        ReadDReyeVRConfig();
}

static void ReadConfigValue(const FString &Section, const FString &Variable, bool &Value)
{
    EnsureConfigsUpdated();
    std::string VariableName = CreateVariableName(Section, Variable);
    if (Params.find(VariableName) != Params.end())
        Value = Params[VariableName].ToBool();
    else
        UE_LOG(LogTemp, Error, TEXT("No variable matching %s found"), *FString(VariableName.c_str()));
}
static void ReadConfigValue(const FString &Section, const FString &Variable, int &Value)
{
    EnsureConfigsUpdated();
    std::string VariableName = CreateVariableName(Section, Variable);
    if (Params.find(VariableName) != Params.end())
        Value = FCString::Atoi(*Params[VariableName]);
    else
        UE_LOG(LogTemp, Error, TEXT("No variable matching %s found"), *FString(VariableName.c_str()));
}
static void ReadConfigValue(const FString &Section, const FString &Variable, float &Value)
{
    EnsureConfigsUpdated();
    std::string VariableName = CreateVariableName(Section, Variable);
    if (Params.find(VariableName) != Params.end())
        Value = FCString::Atof(*Params[VariableName]);
    else
        UE_LOG(LogTemp, Error, TEXT("No variable matching %s found"), *FString(VariableName.c_str()));
}
static void ReadConfigValue(const FString &Section, const FString &Variable, FVector &Value)
{
    EnsureConfigsUpdated();
    std::string VariableName = CreateVariableName(Section, Variable);
    if (Params.find(VariableName) != Params.end())
    {
        if (Value.InitFromString(Params[VariableName]) == false)
        {
            UE_LOG(LogTemp, Error, TEXT("Unable to construct FVector for %s from %s"), *FString(VariableName.c_str()),
                   *(Params[VariableName]));
        }
    }
    else
        UE_LOG(LogTemp, Error, TEXT("No variable matching %s found"), *FString(VariableName.c_str()));
}
static void ReadConfigValue(const FString &Section, const FString &Variable, FRotator &Value)
{
    EnsureConfigsUpdated();
    std::string VariableName = CreateVariableName(Section, Variable);
    if (Params.find(VariableName) != Params.end())
    {
        if (Value.InitFromString(Params[VariableName]) == false)
        {
            UE_LOG(LogTemp, Error, TEXT("Unable to construct FRotator for %s from %s"), *FString(VariableName.c_str()),
                   *(Params[VariableName]));
        }
    }
    else
        UE_LOG(LogTemp, Error, TEXT("No variable matching %s found"), *FString(VariableName.c_str()));
}
static void ReadConfigValue(const FString &Section, const FString &Variable, FString &Value)
{
    EnsureConfigsUpdated();
    std::string VariableName = CreateVariableName(Section, Variable);
    if (Params.find(VariableName) != Params.end())
        Value = Params[VariableName];
    else
        UE_LOG(LogTemp, Error, TEXT("No variable matching %s found"), *FString(VariableName.c_str()));
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
        UE_LOG(LogTemp, Error, TEXT("Missing render target!"));
        return;
    }
    if (!RTResource->ReadPixels(Pixels, ReadPixelFlags))
        UE_LOG(LogTemp, Error, TEXT("Unable to read pixels!"));

    // dump pixel array to disk
    PixelData.Pixels = Pixels;
    TUniquePtr<FImageWriteTask> ImageTask = MakeUnique<FImageWriteTask>();
    ImageTask->PixelData = MakeUnique<TImagePixelData<FColor>>(PixelData);
    ImageTask->Filename = FilePath;
    UE_LOG(LogTemp, Log, TEXT("Saving screenshot to %s"), *FilePath);
    ImageTask->Format = FileFormatJPG ? EImageFormat::JPEG : EImageFormat::PNG; // lower quality, less storage
    ImageTask->CompressionQuality = (int32)EImageCompressionQuality::Default;
    ImageTask->bOverwriteFile = true;
    ImageTask->PixelPreProcessors.Add(TAsyncAlphaWrite<FColor>(255));
    FHighResScreenshotConfig &HighResScreenshotConfig = GetHighResScreenshotConfig();
    HighResScreenshotConfig.ImageWriteQueue->Enqueue(MoveTemp(ImageTask));
}

#endif