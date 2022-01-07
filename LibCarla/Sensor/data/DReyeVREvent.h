#pragma once

#include "carla/geom/Vector3D.h"
#include "carla/sensor/SensorData.h"
#include "carla/sensor/s11n/DReyeVRSerializer.h"

#include <cstdint>

namespace carla
{
namespace sensor
{
namespace data
{
class DReyeVREvent : public SensorData
{
    friend s11n::DReyeVRSerializer;

  protected:
    explicit DReyeVREvent(const RawData &data) : SensorData(data)
    {
        InternalData = s11n::DReyeVRSerializer::DeserializeRawData(data);
    }

  public:
    int64_t GetTimestampCarla() const
    {
        return InternalData.TimestampCarla;
    }
    int64_t GetTimestampDevice() const
    {
        return InternalData.TimestampDevice;
    }
    int64_t GetFrameSequence() const
    {
        return InternalData.FrameSequence;
    }
    const geom::Vector3D &GetGazeDir() const
    {
        return InternalData.GazeDir;
    }
    const geom::Vector3D &GetGazeOrigin() const
    {
        return InternalData.GazeOrigin;
    }
    bool GetGazeValid() const
    {
        return InternalData.GazeValid;
    }
    float GetGazeVergence() const
    {
        return InternalData.GazeVergence;
    }
    const geom::Vector3D &GetCameraLocation() const
    {
        return InternalData.CameraLocation;
    }
    const geom::Vector3D &GetCameraRotation() const
    {
        return InternalData.CameraRotation;
    }
    const geom::Vector3D &GetLGazeDir() const
    {
        return InternalData.LGazeDir;
    }
    const geom::Vector3D &GetLGazeOrigin() const
    {
        return InternalData.LGazeOrigin;
    }
    bool GetLGazeValid() const
    {
        return InternalData.LGazeValid;
    }
    const geom::Vector3D &GetRGazeDir() const
    {
        return InternalData.RGazeDir;
    }
    const geom::Vector3D &GetRGazeOrigin() const
    {
        return InternalData.RGazeOrigin;
    }
    bool GetRGazeValid() const
    {
        return InternalData.RGazeValid;
    }
    float GetLEyeOpenness() const
    {
        return InternalData.LEyeOpenness;
    }
    bool GetLEyeOpenValid() const
    {
        return InternalData.LEyeOpenValid;
    }
    float GetREyeOpenness() const
    {
        return InternalData.REyeOpenness;
    }
    bool GetREyeOpenValid() const
    {
        return InternalData.REyeOpenValid;
    }
    const geom::Vector2D &GetLPupilPos() const
    {
        return InternalData.LPupilPos;
    }
    bool GetLPupilPosValid() const
    {
        return InternalData.LPupilPosValid;
    }
    const geom::Vector2D &GetRPupilPos() const
    {
        return InternalData.RPupilPos;
    }
    bool GetRPupilPosValid() const
    {
        return InternalData.RPupilPosValid;
    }
    float GetLPupilDiam() const
    {
        return InternalData.LPupilDiameter;
    }
    float GetRPupilDiam() const
    {
        return InternalData.RPupilDiameter;
    }
    const std::string &GetFocusActorName() const
    {
        return InternalData.FocusActorName;
    }
    const geom::Vector3D &GetFocusActorPoint() const
    {
        return InternalData.FocusActorPoint;
    }
    float GetFocusActorDist() const
    {
        return InternalData.FocusActorDist;
    }
    float GetThrottle() const
    {
        return InternalData.Throttle;
    }
    float GetSteering() const
    {
        return InternalData.Steering;
    }
    float GetBrake() const
    {
        return InternalData.Brake;
    }
    bool GetToggledReverse() const
    {
        return InternalData.ToggledReverse;
    }
    bool GetHandbrake() const
    {
        return InternalData.HoldHandbrake;
    }

  private:
    carla::sensor::s11n::DReyeVRSerializer::Data InternalData;
};
} // namespace data
} // namespace sensor
} // namespace carla