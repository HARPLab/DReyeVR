#include "carla/sensor/s11n/DReyeVRSerializer.h"
#include "carla/sensor/data/DReyeVREvent.h"

namespace carla
{
    namespace sensor
    {
        namespace s11n
        {
            SharedPtr<SensorData> DReyeVRSerializer::Deserialize(RawData &&data)
            {
                return SharedPtr<SensorData>(new data::DReyeVREvent(std::move(data)));
            }
        } // namespace s11n
    }     // namespace sensor
} // namespace carla