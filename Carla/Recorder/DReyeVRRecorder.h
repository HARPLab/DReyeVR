#pragma once

#include <fstream>
#include <vector>
// DReyeVR include
#include "Carla/Sensor/DReyeVRData.h"

#pragma pack(push, 1)
struct DReyeVRDataRecorder
{
    DReyeVRDataRecorder() = default;
    DReyeVRDataRecorder(const DReyeVR::AggregateData *DataIn)
    {
        Data = (*DataIn);
    }
    DReyeVR::AggregateData Data;
    void Read(std::ifstream &InFile);
    void Write(std::ofstream &OutFile) const;
    std::string Print() const;
};
#pragma pack(pop)

class DReyeVRDataRecorders
{

  public:
    void Add(const DReyeVRDataRecorder &NewData);
    void Clear(void);
    void Write(std::ofstream &OutFile);

  private:
    // using a vector as a queue that holds everything, gets written and flushed on every tick
    std::vector<DReyeVRDataRecorder> AllData;
};
