#include <string>
#include <unordered_map>

namespace DReyeVR
{

/// ========================================== ///
/// ----------------:EYEDATA:----------------- ///
/// ========================================== ///

void EyeData::Read(std::ifstream &InFile)
{
    ReadFVector(InFile, GazeDir);
    ReadFVector(InFile, GazeOrigin);
    ReadValue<bool>(InFile, GazeValid);
}

void EyeData::Write(std::ofstream &OutFile) const
{
    WriteFVector(OutFile, GazeDir);
    WriteFVector(OutFile, GazeOrigin);
    WriteValue<bool>(OutFile, GazeValid);
}

FString EyeData::ToString() const
{
    FString Print;
    Print += FString::Printf(TEXT("GazeDir:%s,"), *GazeDir.ToString());
    Print += FString::Printf(TEXT("GazeOrigin:%s,"), *GazeOrigin.ToString());
    Print += FString::Printf(TEXT("GazeValid:%d,"), GazeValid);
    return Print;
}

/// ========================================== ///
/// ------------:COMBINEDEYEDATA:------------- ///
/// ========================================== ///

void CombinedEyeData::Read(std::ifstream &InFile)
{
    EyeData::Read(InFile);
    ReadValue<float>(InFile, Vergence);
}

void CombinedEyeData::Write(std::ofstream &OutFile) const
{
    EyeData::Write(OutFile);
    WriteValue<float>(OutFile, Vergence);
}

FString CombinedEyeData::ToString() const
{
    FString Print = EyeData::ToString();
    Print += FString::Printf(TEXT("GazeVergence:%.4f,"), Vergence);
    return Print;
}

/// ========================================== ///
/// -------------:SINGLEEYEDATA:-------------- ///
/// ========================================== ///

void SingleEyeData::Read(std::ifstream &InFile)
{
    EyeData::Read(InFile);
    ReadValue<float>(InFile, EyeOpenness);
    ReadValue<bool>(InFile, EyeOpennessValid);
    ReadValue<float>(InFile, PupilDiameter);
    ReadFVector2D(InFile, PupilPosition);
    ReadValue<bool>(InFile, PupilPositionValid);
}

void SingleEyeData::Write(std::ofstream &OutFile) const
{
    EyeData::Write(OutFile);
    WriteValue<float>(OutFile, EyeOpenness);
    WriteValue<bool>(OutFile, EyeOpennessValid);
    WriteValue<float>(OutFile, PupilDiameter);
    WriteFVector2D(OutFile, PupilPosition);
    WriteValue<bool>(OutFile, PupilPositionValid);
}

FString SingleEyeData::ToString() const
{
    FString Print = EyeData::ToString();
    Print += FString::Printf(TEXT("EyeOpenness:%.4f,"), EyeOpenness);
    Print += FString::Printf(TEXT("EyeOpennessValid:%d,"), EyeOpennessValid);
    Print += FString::Printf(TEXT("PupilDiameter:%.4f,"), PupilDiameter);
    Print += FString::Printf(TEXT("PupilPosition:%s,"), *PupilPosition.ToString());
    Print += FString::Printf(TEXT("PupilPositionValid:%d,"), PupilPositionValid);
    return Print;
}

/// ========================================== ///
/// --------------:EGOVARIABLES:-------------- ///
/// ========================================== ///

void EgoVariables::Read(std::ifstream &InFile)
{
    ReadFVector(InFile, CameraLocation);
    ReadFRotator(InFile, CameraRotation);
    ReadFVector(InFile, CameraLocationAbs);
    ReadFRotator(InFile, CameraRotationAbs);
    ReadFVector(InFile, VehicleLocation);
    ReadFRotator(InFile, VehicleRotation);
    ReadValue<float>(InFile, Velocity);
}

void EgoVariables::Write(std::ofstream &OutFile) const
{
    WriteFVector(OutFile, CameraLocation);
    WriteFRotator(OutFile, CameraRotation);
    WriteFVector(OutFile, CameraLocationAbs);
    WriteFRotator(OutFile, CameraRotationAbs);
    WriteFVector(OutFile, VehicleLocation);
    WriteFRotator(OutFile, VehicleRotation);
    WriteValue<float>(OutFile, Velocity);
}

FString EgoVariables::ToString() const
{
    FString Print;
    Print += FString::Printf(TEXT("VehicleLoc:%s,"), *VehicleLocation.ToString());
    Print += FString::Printf(TEXT("VehicleRot:%s,"), *VehicleRotation.ToString());
    Print += FString::Printf(TEXT("VehicleVel:%.4f,"), Velocity);
    Print += FString::Printf(TEXT("CameraLoc:%s,"), *CameraLocation.ToString());
    Print += FString::Printf(TEXT("CameraRot:%s,"), *CameraRotation.ToString());
    Print += FString::Printf(TEXT("CameraLocAbs:%s,"), *CameraLocationAbs.ToString());
    Print += FString::Printf(TEXT("CameraRotAbs:%s,"), *CameraRotationAbs.ToString());
    return Print;
}

/// ========================================== ///
/// --------------:USERINPUTS:---------------- ///
/// ========================================== ///

void UserInputs::Read(std::ifstream &InFile)
{
    ReadValue<float>(InFile, Throttle);
    ReadValue<float>(InFile, Steering);
    ReadValue<float>(InFile, Brake);
    ReadValue<bool>(InFile, ToggledReverse);
    ReadValue<bool>(InFile, TurnSignalLeft);
    ReadValue<bool>(InFile, TurnSignalRight);
    ReadValue<bool>(InFile, HoldHandbrake);
}

void UserInputs::Write(std::ofstream &OutFile) const
{
    WriteValue<float>(OutFile, Throttle);
    WriteValue<float>(OutFile, Steering);
    WriteValue<float>(OutFile, Brake);
    WriteValue<bool>(OutFile, ToggledReverse);
    WriteValue<bool>(OutFile, TurnSignalLeft);
    WriteValue<bool>(OutFile, TurnSignalRight);
    WriteValue<bool>(OutFile, HoldHandbrake);
}

FString UserInputs::ToString() const
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

/// ========================================== ///
/// ---------------:FOCUSINFO:---------------- ///
/// ========================================== ///

void FocusInfo::Read(std::ifstream &InFile)
{
    ReadFString(InFile, ActorNameTag);
    ReadValue<bool>(InFile, bDidHit);
    ReadFVector(InFile, HitPoint);
    ReadFVector(InFile, Normal);
    ReadValue<float>(InFile, Distance);
}

void FocusInfo::Write(std::ofstream &OutFile) const
{
    WriteFString(OutFile, ActorNameTag);
    WriteValue<bool>(OutFile, bDidHit);
    WriteFVector(OutFile, HitPoint);
    WriteFVector(OutFile, Normal);
    WriteValue<float>(OutFile, Distance);
}

FString FocusInfo::ToString() const
{
    FString Print;
    Print += FString::Printf(TEXT("Hit:%d,"), bDidHit);
    Print += FString::Printf(TEXT("Distance:%.4f,"), Distance);
    Print += FString::Printf(TEXT("HitPoint:%s,"), *HitPoint.ToString());
    Print += FString::Printf(TEXT("HitNormal:%s,"), *Normal.ToString());
    Print += FString::Printf(TEXT("ActorName:%s,"), *ActorNameTag);
    return Print;
}

/// ========================================== ///
/// ---------------:EYETRACKER:--------------- ///
/// ========================================== ///

void EyeTracker::Read(std::ifstream &InFile)
{
    ReadValue<int64_t>(InFile, TimestampDevice);
    ReadValue<int64_t>(InFile, FrameSequence);
    Combined.Read(InFile);
    Left.Read(InFile);
    Right.Read(InFile);
}

void EyeTracker::Write(std::ofstream &OutFile) const
{
    WriteValue<int64_t>(OutFile, TimestampDevice);
    WriteValue<int64_t>(OutFile, FrameSequence);
    Combined.Write(OutFile);
    Left.Write(OutFile);
    Right.Write(OutFile);
}

FString EyeTracker::ToString() const
{
    FString Print;
    Print += FString::Printf(TEXT("TimestampDevice:%ld,"), long(TimestampDevice));
    Print += FString::Printf(TEXT("FrameSequence:%ld,"), long(FrameSequence));
    Print += FString::Printf(TEXT("COMBINED:{%s},"), *Combined.ToString());
    Print += FString::Printf(TEXT("LEFT:{%s},"), *Left.ToString());
    Print += FString::Printf(TEXT("RIGHT:{%s},"), *Right.ToString());
    return Print;
}

/// ========================================== ///
/// ------------:CONFIGFILEDATA:-------------- ///
/// ========================================== ///

void ConfigFileData::Set(const std::string &Contents)
{
    StringContents = FString(Contents.c_str());
}

void ConfigFileData::Read(std::ifstream &InFile)
{
    ReadFString(InFile, StringContents);
}

void ConfigFileData::Write(std::ofstream &OutFile) const
{
    WriteFString(OutFile, StringContents);
}

FString ConfigFileData::ToString() const
{
    return StringContents;
}

/// ========================================== ///
/// -------------:AGGREGATEDATA:-------------- ///
/// ========================================== ///

int64_t AggregateData::GetTimestampCarla() const
{
    return TimestampCarlaUE4;
}

int64_t AggregateData::GetTimestampDevice() const
{
    return EyeTrackerData.TimestampDevice;
}

int64_t AggregateData::GetFrameSequence() const
{
    return EyeTrackerData.FrameSequence;
}

float AggregateData::GetGazeVergence() const
{
    return EyeTrackerData.Combined.Vergence; // in cm (default UE4 units)
}

const FVector &AggregateData::GetGazeDir(DReyeVR::Gaze Index) const
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

const FVector &AggregateData::GetGazeOrigin(DReyeVR::Gaze Index) const
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

bool AggregateData::GetGazeValidity(DReyeVR::Gaze Index) const
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

float AggregateData::GetEyeOpenness(DReyeVR::Eye Index) const // returns eye openness as a percentage [0,1]
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

bool AggregateData::GetEyeOpennessValidity(DReyeVR::Eye Index) const
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

float AggregateData::GetPupilDiameter(DReyeVR::Eye Index) const // returns diameter in mm
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

const FVector2D &AggregateData::GetPupilPosition(DReyeVR::Eye Index) const
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

bool AggregateData::GetPupilPositionValidity(DReyeVR::Eye Index) const
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
const FVector &AggregateData::GetCameraLocation() const
{
    return EgoVars.CameraLocation;
}

const FRotator &AggregateData::GetCameraRotation() const
{
    return EgoVars.CameraRotation;
}

const FVector &AggregateData::GetCameraLocationAbs() const
{
    return EgoVars.CameraLocationAbs;
}

const FRotator &AggregateData::GetCameraRotationAbs() const
{
    return EgoVars.CameraRotationAbs;
}

float AggregateData::GetVehicleVelocity() const
{
    return EgoVars.Velocity; // returns ego velocity in cm/s
}

const FVector &AggregateData::GetVehicleLocation() const
{
    return EgoVars.VehicleLocation;
}

const FRotator &AggregateData::GetVehicleRotation() const
{
    return EgoVars.VehicleRotation;
}

// focus
const FString &AggregateData::GetFocusActorName() const
{
    return FocusData.ActorNameTag;
}

const FVector &AggregateData::GetFocusActorPoint() const
{
    return FocusData.HitPoint;
}

float AggregateData::GetFocusActorDistance() const
{
    return FocusData.Distance;
}

const DReyeVR::UserInputs &AggregateData::GetUserInputs() const
{
    return Inputs;
}

void AggregateData::UpdateCamera(const FVector &NewCameraLoc, const FRotator &NewCameraRot)
{
    EgoVars.CameraLocation = NewCameraLoc;
    EgoVars.CameraRotation = NewCameraRot;
}

void AggregateData::UpdateCameraAbs(const FVector &NewCameraLocAbs, const FRotator &NewCameraRotAbs)
{
    EgoVars.CameraLocationAbs = NewCameraLocAbs;
    EgoVars.CameraRotationAbs = NewCameraRotAbs;
}

void AggregateData::UpdateVehicle(const FVector &NewVehicleLoc, const FRotator &NewVehicleRot)
{
    EgoVars.VehicleLocation = NewVehicleLoc;
    EgoVars.VehicleRotation = NewVehicleRot;
}

void AggregateData::Update(int64_t NewTimestamp, const struct EyeTracker &NewEyeData,
                           const struct EgoVariables &NewEgoVars, const struct FocusInfo &NewFocus,
                           const struct UserInputs &NewInputs)
{
    TimestampCarlaUE4 = NewTimestamp;
    EyeTrackerData = NewEyeData;
    EgoVars = NewEgoVars;
    FocusData = NewFocus;
    Inputs = NewInputs;
}

void AggregateData::Read(std::ifstream &InFile)
{
    /// CAUTION: make sure the order of writes/reads is the same
    ReadValue<int64_t>(InFile, TimestampCarlaUE4);
    EgoVars.Read(InFile);
    EyeTrackerData.Read(InFile);
    FocusData.Read(InFile);
    Inputs.Read(InFile);
}

void AggregateData::Write(std::ofstream &OutFile) const
{
    /// CAUTION: make sure the order of writes/reads is the same
    WriteValue<int64_t>(OutFile, GetTimestampCarla());
    EgoVars.Write(OutFile);
    EyeTrackerData.Write(OutFile);
    FocusData.Write(OutFile);
    Inputs.Write(OutFile);
}

FString AggregateData::ToString() const
{
    FString print;
    print += FString::Printf(TEXT("  [DReyeVR]TimestampCarla:%ld,\n"), long(TimestampCarlaUE4));
    print += FString::Printf(TEXT("  [DReyeVR]EyeTracker:%s,\n"), *EyeTrackerData.ToString());
    print += FString::Printf(TEXT("  [DReyeVR]FocusInfo:%s,\n"), *FocusData.ToString());
    print += FString::Printf(TEXT("  [DReyeVR]EgoVariables:%s,\n"), *EgoVars.ToString());
    print += FString::Printf(TEXT("  [DReyeVR]UserInputs:%s,\n"), *Inputs.ToString());
    return print;
}

/// ========================================== ///
/// ------------:CUSTOMACTORDATA:------------- ///
/// ========================================== ///

void CustomActorData::MaterialParamsStruct::Apply(class UMaterialInstanceDynamic *DynamicMaterial) const
{
    /// PARAMS:
    // these are either scalar (float) or vector (FLinearColor) attributes baked into the texture as follows

    /// SCALAR:
    // "Metallic" -> controls how metal-like your surface looks like, default 0
    // "Specular" -> used to scale the current amount of specularity on non-metallic surfaces. Bw [0, 1], default 0.5
    // "Roughness" -> Controls how rough the material is. [0 (smooth/mirror), 1(rough/diffuse)], default 0
    // "Anisotropy" -> Determines the extent the specular highlight is stretched along the tangent. Bw [0, 1], default 0
    // "Opacity" -> How opaque is this material NOTE: ONLY APPLIES TO TRANSLUCENT MATERIAL. Bw [0, 1], default 0.15

    /// VECTOR:
    // "BaseColor" -> defines the overall colour of the material (each channel is clamped to [0, 1])
    // "Emissive" -> controls which parts of the material appear to glow

    /// NOTE: Opacity only gets applied when the material is based on the TranslucentParamMaterial, all the other scalar
    // params only get applied in the opaque case.

    // assign material params
    if (DynamicMaterial != nullptr)
    {
        DynamicMaterial->SetScalarParameterValue("Metallic", Metallic);
        DynamicMaterial->SetScalarParameterValue("Specular", Specular);
        DynamicMaterial->SetScalarParameterValue("Roughness", Roughness);
        DynamicMaterial->SetScalarParameterValue("Anisotropy", Anisotropy);
        DynamicMaterial->SetScalarParameterValue("Opacity", Opacity);
        DynamicMaterial->SetVectorParameterValue("BaseColor", BaseColor);
        DynamicMaterial->SetVectorParameterValue("Emissive", Emissive);
    }
}

void CustomActorData::MaterialParamsStruct::Read(std::ifstream &InFile)
{
    ReadValue<float>(InFile, Metallic);
    ReadValue<float>(InFile, Specular);
    ReadValue<float>(InFile, Roughness);
    ReadValue<float>(InFile, Anisotropy);
    ReadValue<float>(InFile, Opacity);
    ReadFLinearColor(InFile, BaseColor);
    ReadFLinearColor(InFile, Emissive);
    ReadFString(InFile, MaterialPath);
}

void CustomActorData::MaterialParamsStruct::Write(std::ofstream &OutFile) const
{
    WriteValue<float>(OutFile, Metallic);
    WriteValue<float>(OutFile, Specular);
    WriteValue<float>(OutFile, Roughness);
    WriteValue<float>(OutFile, Anisotropy);
    WriteValue<float>(OutFile, Opacity);
    WriteFLinearColor(OutFile, BaseColor);
    WriteFLinearColor(OutFile, Emissive);
    WriteFString(OutFile, MaterialPath);
}

FString PrintFLinearColor(const FLinearColor &F)
{
    // so the print output is consistent with FVector::ToString(), FVector2D::ToString(), FRotator::ToString()
    // and the parser can treat this similarly
    return FString::Printf(TEXT("R=%.6f G=%.6f B=%.6f A=%.6f"), F.R, F.G, F.B, F.A);
}

FString CustomActorData::MaterialParamsStruct::ToString() const
{
    FString Print = "";
    Print += FString::Printf(TEXT("Metallic:%.3f,"), Metallic);
    Print += FString::Printf(TEXT("Specular:%.3f,"), Specular);
    Print += FString::Printf(TEXT("Roughness:%.3f,"), Roughness);
    Print += FString::Printf(TEXT("Anisotropy:%.3f,"), Anisotropy);
    Print += FString::Printf(TEXT("Opacity:%.3f,"), Opacity);
    Print += FString::Printf(TEXT("BaseColor:%s,"), *PrintFLinearColor(BaseColor));
    Print += FString::Printf(TEXT("Emissive:%s,"), *PrintFLinearColor(Emissive));
    Print += FString::Printf(TEXT("Path:%s,"), *MaterialPath);
    return Print;
}

void CustomActorData::Read(std::ifstream &InFile)
{
    // 9 dof
    ReadFVector(InFile, Location);
    ReadFRotator(InFile, Rotation);
    ReadFVector(InFile, Scale3D);
    // visual properties
    ReadFString(InFile, MeshPath);
    // material properties
    MaterialParams.Read(InFile);
    // other
    ReadFString(InFile, Other);
    ReadFString(InFile, Name);
}

void CustomActorData::Write(std::ofstream &OutFile) const
{
    // 9 dof
    WriteFVector(OutFile, Location);
    WriteFRotator(OutFile, Rotation);
    WriteFVector(OutFile, Scale3D);
    // visual properties
    WriteFString(OutFile, MeshPath);
    // material properties
    MaterialParams.Write(OutFile);
    // other
    WriteFString(OutFile, Other);
    WriteFString(OutFile, Name);
}

FString CustomActorData::ToString() const
{
    FString Print = "  [DReyeVR_CA]";
    Print += FString::Printf(TEXT("Name:%s,"), *Name);
    Print += FString::Printf(TEXT("Location:%s,"), *Location.ToString());
    Print += FString::Printf(TEXT("Rotation:%s,"), *Rotation.ToString());
    Print += FString::Printf(TEXT("Scale3D:%s,"), *Scale3D.ToString());
    Print += FString::Printf(TEXT("MeshPath:%s,"), *MeshPath);
    Print += FString::Printf(TEXT("Material:{%s},"), *MaterialParams.ToString());
    Print += FString::Printf(TEXT("Other:%s,"), *Other);
    return Print;
}

std::string CustomActorData::GetUniqueName() const
{
    return TCHAR_TO_UTF8(*Name);
}

}; // namespace DReyeVR