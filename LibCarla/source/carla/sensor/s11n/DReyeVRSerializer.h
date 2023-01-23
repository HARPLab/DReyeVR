
#pragma once

#include "carla/Buffer.h"
#include "carla/Memory.h"
#include "carla/geom/Vector2D.h"
#include "carla/geom/Vector3D.h"
#include "carla/sensor/RawData.h"

#include <cstdint>
#include <string>

namespace carla
{
namespace sensor
{
class SensorData;

namespace s11n
{
class DReyeVRSerializer
{
  public:
    struct Data
    {
        /// TODO: refactor this struct to contain smaller structs similar to DReyeVR::AggregateData
        /// NOTE: this is missing some fields that can totally be added, but you get the idea.
        // Step 1: add new field field here in Data struct
        // Step 2: add new field in MSGPACK_DEFINE_ARRAY) in the SAME ORDER
        // Step 3: go to LibCarla/source/carla/sensor/data/DReyeVREvent.h and add a const getter
        // Step 4: go to PythonAPI/carla/source/libcarla/SensorData.cpp and add the getter to the list of available attributes just like the others
        int64_t TimestampCarla;
        int64_t TimestampDevice;
        int64_t FrameSequence;
        // camera
        geom::Vector3D CameraLocation;
        geom::Vector3D CameraRotation;
        // combined gaze
        geom::Vector3D GazeDir;
        geom::Vector3D GazeOrigin;
        bool GazeValid;
        float GazeVergence;
        // left gaze/eye
        geom::Vector3D LGazeDir;
        geom::Vector3D LGazeOrigin;
        bool LGazeValid;
        float LEyeOpenness;
        bool LEyeOpenValid;
        geom::Vector2D LPupilPos;
        bool LPupilPosValid;
        float LPupilDiameter;
        // right gaze/eye
        geom::Vector3D RGazeDir;
        geom::Vector3D RGazeOrigin;
        bool RGazeValid;
        float REyeOpenness;
        bool REyeOpenValid;
        geom::Vector2D RPupilPos;
        bool RPupilPosValid;
        float RPupilDiameter;
        // focus 
        std::string FocusActorName;
        geom::Vector3D FocusActorPoint;
        float FocusActorDist;
        // inputs
        float Throttle;
        float Steering;
        float Brake;
        bool ToggledReverse;
        bool HoldHandbrake;

        MSGPACK_DEFINE_ARRAY(TimestampCarla, TimestampDevice, FrameSequence, // timings
                             CameraLocation, CameraRotation,                 // camera
                             GazeDir, GazeOrigin, GazeValid, GazeVergence,   // combined gaze
                             LGazeDir, LGazeOrigin, LGazeValid, LEyeOpenness, LEyeOpenValid, LPupilPos, LPupilPosValid, LPupilDiameter, // left gaze/eye
                             RGazeDir, RGazeOrigin, RGazeValid, REyeOpenness, REyeOpenValid, RPupilPos, RPupilPosValid, RPupilDiameter, // right gaze/eye
                             FocusActorName, FocusActorPoint, FocusActorDist,         // focus info
                             Throttle, Steering, Brake, ToggledReverse, HoldHandbrake // user inputs
        )
    };

    static Data DeserializeRawData(const RawData &message)
    {
        return MsgPack::UnPack<Data>(message.begin(), message.size());
    }

    template <typename SensorT> static Buffer Serialize(const SensorT &, struct Data &&DataIn)
    {
        return MsgPack::Pack(DataIn);
    }
    static SharedPtr<SensorData> Deserialize(RawData &&data);
};

} // namespace s11n
} // namespace sensor
} // namespace carla