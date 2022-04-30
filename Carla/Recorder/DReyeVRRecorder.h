#pragma once

#include <fstream>
#include <vector>
// DReyeVR include
#include "Carla/Sensor/DReyeVRData.h"

// custom DReyeVR recorder data type for sensor recordings and custom actors

#pragma pack(push, 1)
template <typename T> struct DReyeVRDataRecorder
{
    DReyeVRDataRecorder<T>() = default;
    DReyeVRDataRecorder<T>(const T *DataIn)
    {
        Data = (*DataIn);
    }
    T Data;
    void Read(std::ifstream &InFile)
    {
        Data.Read(InFile);
    }
    void Write(std::ofstream &OutFile) const
    {
        Data.Write(OutFile);
    }
    std::string GetUniqueName() const
    {
        return Data.GetUniqueName();
    }
    std::string Print() const
    {
        std::ostringstream oss;
        oss << TCHAR_TO_UTF8(*Data.ToString());
        return oss.str();
    }
};
#pragma pack(pop)

template <typename T, uint8_t N> class DReyeVRDataRecorders
{

  public:
    void Add(const DReyeVRDataRecorder<T> &NewData)
    {
        AllData.push_back(NewData);
    }
    void Clear(void)
    {
        AllData.clear();
    }
    void Write(std::ofstream &OutFile)
    {
        // write the packet id
        WriteValue<char>(OutFile, static_cast<char>(PacketId));
        std::streampos PosStart = OutFile.tellp();

        // write a dummy packet size
        uint32_t Total = 0;
        WriteValue<uint32_t>(OutFile, Total);

        // write total records
        Total = AllData.size();
        WriteValue<uint16_t>(OutFile, Total);

        for (auto &Snapshot : AllData)
            Snapshot.Write(OutFile);

        // write the real packet size
        std::streampos PosEnd = OutFile.tellp();
        Total = PosEnd - PosStart - sizeof(uint32_t);
        OutFile.seekp(PosStart, std::ios::beg);
        WriteValue<uint32_t>(OutFile, Total);
        OutFile.seekp(PosEnd, std::ios::beg);
    }

  private:
    // using a vector as a queue that holds everything, gets written and flushed on every tick
    std::vector<DReyeVRDataRecorder<T>> AllData;
    uint8_t PacketId = N;
};
