#include "Carla/Actor/ActorInfo.h"
#include "Carla/Actor/ActorData.h"
#include "Carla/Vehicle/CarlaWheeledVehicle.h"
#include "Carla/Walker/WalkerController.h"
#include "Carla/Traffic/TrafficLightState.h"

#include "Carla/Actor/ActorDefinition.h"
#include "Carla/Actor/ActorDescription.h"
#include "Carla/Actor/ActorRegistry.h"

#include "Carla/Game/CarlaEpisode.h"
#include "EgoSensor.h"      
#include "Carla/Sensor/DReyeVRData.h"                 // DReyeVR namespace

//Unreal
#include "Tickable.h"
#include "CoreMinimal.h"

typedef struct ZonesType {
    float Fstart = 325.0;
    float Fend = 35.0;
    float Rstart = 25.0;
    float Rend = 155.0;
    float Bstart = 145.0;
    float Bend = 215.0;
    float Lstart = 205.0;
    float Lend = 335.0;
} ZoneInfo;

typedef struct BitsEncodeT {
    int Forward = 1;
    int Right = 2;
    int Back = 4;
    int Left = 8;
    int VehicleType = 16;
    int WalkerType = 0;
} BitsEncode;

class FAwarenessManager 
{
public:
    FAwarenessManager(class AActor *EgoVehicle, bool AwarenessModeEnabled, 
                    bool AwarenessVelocityMode, bool AwarenessPositionMode)
    {
        EgoVehiclePtr = EgoVehicle;
        IsEnabled = true;
        VelMode = true;
        PosMode = false;
    }
    void Tick(float DeltaTime);
    void SetAwarenessManager(UCarlaEpisode *Episode, UWorld *World, AEgoSensor *EgoSensor);
    void Destroy();    
    ZoneInfo Zones;
    BitsEncode Encode;
    bool PressedFwdV = false;
    bool PressedRightV = false;
    bool PressedLeftV = false;
    bool PressedBackV = false;
    bool PressedFwdW = false;
    bool PressedRightW = false;
    bool PressedLeftW = false;
    bool PressedBackW = false;
    FVector GazeOrigin;
private:
    class UWorld *World;
    UCarlaEpisode *Episode = nullptr;
    class AActor *EgoVehiclePtr = nullptr;
    class AEgoSensor *EgoSensor;
    bool IsEnabled;
    bool VelMode;
    bool PosMode;
    void render_ids(const std::vector <FCarlaActor*> &VisibleRaw, float DeltaTime);
    //UPROPERTY(EditAnywhere, Category="Collision");
    void is_visible_debug(const AActor* UEActor, float Time);
    void is_visible_debug2(const AActor* UEActor, float Time);
    void display_answer(FCarlaActor* Actor, int64_t Answer, float Time);
    bool is_visible(const AActor* UEActor);
};