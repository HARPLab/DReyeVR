#include <math.h>
#include "Carla/Actor/CarlaActor.h"
#include "Carla/Actor/ActorInfo.h"
#include "Carla/Actor/ActorData.h"
#include "Carla/Actor/ActorRegistry.h"
#include "carla/rpc/ActorState.h"
#include "Carla/Game/CarlaStatics.h"  
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"
#include "carla/rpc/VehicleLightState.h"

#include "AwarenessManager.h"

typedef struct Direction {
    bool forward = false;
    bool backward = false;
    bool left = false;
    bool right = false;
} DirType;

void FAwarenessManager::render_ids(const std::vector <FCarlaActor*> &VisibleRaw, float DeltaTime) {
    for (FCarlaActor* Actor : VisibleRaw) {
        FVector BBox_Offset, BBox_Extent;
        int64_t id = Actor->GetActorId();
        FString id_s = FString::Printf(TEXT("%d"), id);
        Actor->GetActor()->GetActorBounds(true, BBox_Offset, BBox_Extent, false);
        float Height = 2 * BBox_Extent.Z; // extent is only half the "volume" (extension from center)
        FVector Pos = Actor->GetActor()->GetActorLocation() - EgoVehiclePtr->GetActorLocation();
        DrawDebugString(World, Pos, id_s, EgoVehiclePtr, FColor::Blue, DeltaTime);
    }
}

void FAwarenessManager::display_answer(FCarlaActor* Actor, int64_t Answer, float Time) {
    FVector BBox_Offset, BBox_Extent;
    int64_t id = Actor->GetActorId();
    FString id_s = FString::Printf(TEXT("%d"), id);
    Actor->GetActor()->GetActorBounds(true, BBox_Offset, BBox_Extent, false);
    float Height = 2 * BBox_Extent.Z; // extent is only half the "volume" (extension from center)
    FVector Pos = Actor->GetActor()->GetActorLocation() + (Height * FVector::UpVector) - EgoVehiclePtr->GetActorLocation();
    FString s = "";
    if (Answer & Encode.Forward) s += 'F';
    if (Answer & Encode.Back) s += 'B';
    if (Answer & Encode.Left) s += 'L';
    if (Answer & Encode.Right) s += 'R';
    DrawDebugString(World, Pos, s, EgoVehiclePtr, FColor::Red, Time);
}

void draw_color_sphere(FCarlaActor* Actor, UWorld *World, FColor color) {
    FVector BBox_Offset, BBox_Extent;
    Actor->GetActor()->GetActorBounds(true, BBox_Offset, BBox_Extent, false);
    float Height = 2 * BBox_Extent.Z; // extent is only half the "volume" (extension from center)
    FVector Pos = Actor->GetActor()->GetActorLocation() + (Height + 100.f) * FVector::UpVector;
    DrawDebugSphere(World, Pos, 25.0f, 12, color);
}

/*void render_ids() {
    std::cout << "!!! " << VisibleRaw.size() << std::endl;
    for (FCarlaActor* Actor : VisibleRaw) {
        FVector BBox_Offset, BBox_Extent;
        int64_t id = Actor->GetActorId();
        FString id_s = FString::Printf(TEXT("%d"), id);
        Actor->GetActor()->GetActorBounds(true, BBox_Offset, BBox_Extent, false);
        float Height = 2 * BBox_Extent.Z; // extent is only half the "volume" (extension from center)
        FVector Pos = Actor->GetActor()->GetActorLocation() + (Height + 100.f) * FVector::UpVector;
        DrawDebugString(World, Pos, id_s, Actor->GetActor(), FColor::Turquoise);
    }
}*/



float get_angle(FVector v, FVector u, FVector up) {
    float angle = fabs(acos(FVector::DotProduct(v, u) / u.Size() / v.Size())) * 180.0 / PI;
    bool sign = FVector::DotProduct(FVector::CrossProduct(v, u), up) >= 0;
    if (!sign) angle = 360.0 - angle;
    return angle;
}

bool float_equal(float a, float b, float Tol) {
    return (a * Tol > fabs(a - b));
}

void GetDirection(FVector Dirvector, AActor *EgoVehiclePtr, ZoneInfo *Zones, DirType *Dir) {
    // Get the base direction vectors in the ego vehicle orientation
    FVector Forward = EgoVehiclePtr->GetActorForwardVector();
    FVector Backward = -Forward;
    FVector Right = EgoVehiclePtr->GetActorRightVector();
    FVector Left = -Right;

    FVector V = FVector::VectorPlaneProject(Dirvector, FVector::CrossProduct(Forward, Right));
    float angle = get_angle(Forward, V, EgoVehiclePtr->GetActorUpVector());
    
    if (Zones->Rstart <= angle && angle <= Zones->Rend) {
        Dir->right = true;
    }
    if (Zones->Bstart <= angle && angle <= Zones->Bend) {
        Dir->backward = true;
    }
    if (Zones->Lstart <= angle && angle <= Zones->Lend) {
        Dir->left = true;
    }
    if (Zones->Fstart <= angle || angle <= Zones->Fend) {
        Dir->forward = true;
    }
                                                
}

void FAwarenessManager::is_visible_debug(const AActor* UEActor, float Time) {
    FHitResult Hit;
    FVector TraceStart = EgoVehiclePtr->GetActorLocation();
  
    FVector AOrigin, BBox_Extent;
    UEActor->GetActorBounds(true, AOrigin, BBox_Extent, false);
    float Z = BBox_Extent.Z / 2; // extent is only half the "volume" (extension from center)
    float Y = BBox_Extent.Y / 2;
    float X = BBox_Extent.X / 2;
    FVector Zv = (UEActor->GetActorUpVector()) * Z;
    FVector Yv = (UEActor->GetActorRightVector()) * Y;
    FVector Xv = (UEActor->GetActorForwardVector()) * X;

    FVector TraceEnd0 = AOrigin;
    std::vector<FVector> TraceEnds = {};
    std::vector<int> sign = {1, -1};
    TraceEnds.push_back(AOrigin);
    for (int xs : sign) {
        for (int ys : sign) {
            for (int zs : sign) {
                TraceEnds.push_back(AOrigin + Xv * xs + Yv * ys + Zv * zs);
            }
        }
    }
    TEnumAsByte<ECollisionChannel> TraceChannelProperty = ECC_Visibility;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(EgoVehiclePtr);
    QueryParams.AddIgnoredActor(UEActor);

    bool visible = false;
    for (FVector TraceEnd : TraceEnds) {
        bool was_hit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TraceChannelProperty, QueryParams);
        if (was_hit) {
            DrawDebugLine(World, TraceStart, TraceEnd, FColor::Red, false, Time, 0, 1.5f);
        } else {
            DrawDebugLine(World, TraceStart, TraceEnd, FColor::Blue, false, Time, 0, 1.5f);
        }
        visible = visible || (!was_hit);
    }
}

void FAwarenessManager::is_visible_debug2(const AActor* UEActor, float Time) {
    FHitResult Hit;
    FVector TraceStart = EgoVehiclePtr->GetActorLocation();
  
    FVector AOrigin, BBox_Extent;
    UEActor->GetActorBounds(true, AOrigin, BBox_Extent, false);
    float Z = BBox_Extent.Z / 2; // extent is only half the "volume" (extension from center)
    float Y = BBox_Extent.Y / 2;
    float X = BBox_Extent.X / 2;
    FVector Zv = (UEActor->GetActorUpVector()) * Z;
    FVector Yv = (UEActor->GetActorRightVector()) * Y;
    FVector Xv = (UEActor->GetActorForwardVector()) * X;

    FVector TraceEnd0 = AOrigin;
    std::vector<FVector> TraceEnds = {};
    std::vector<int> sign = {1, -1};
    TraceEnds.push_back(AOrigin);
    for (int xs : sign) {
        for (int ys : sign) {
            for (int zs : sign) {
                TraceEnds.push_back(AOrigin + Xv * xs + Yv * ys + Zv * zs);
            }
        }
    }
    TEnumAsByte<ECollisionChannel> TraceChannelProperty = ECC_Visibility;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(EgoVehiclePtr);
    QueryParams.AddIgnoredActor(UEActor);

    bool visible = false;
    for (FVector TraceEnd : TraceEnds) {
        bool was_hit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TraceChannelProperty, QueryParams);
        visible = visible || (!was_hit);
    }
    if (visible) {
        DrawDebugLine(World, TraceStart, TraceEnd0, FColor::Blue, false, Time, 0, 1.5f);
    } else {
        DrawDebugLine(World, TraceStart, TraceEnd0, FColor::Red, false, Time, 0, 1.5f);
    }
       
}

bool FAwarenessManager::is_visible(const AActor* UEActor) {
    FHitResult Hit;
    FVector TraceStart = EgoVehiclePtr->GetActorLocation();
  
    FVector AOrigin, BBox_Extent;
    UEActor->GetActorBounds(true, AOrigin, BBox_Extent, false);
    float Z = BBox_Extent.Z / 2; // extent is only half the "volume" (extension from center)
    float Y = BBox_Extent.Y / 2;
    float X = BBox_Extent.X / 2;
    FVector Zv = (UEActor->GetActorUpVector()) * Z;
    FVector Yv = (UEActor->GetActorRightVector()) * Y;
    FVector Xv = (UEActor->GetActorForwardVector()) * X;

    FVector TraceEnd0 = AOrigin;
    std::vector<FVector> TraceEnds = {};
    std::vector<int> sign = {1, -1};
    TraceEnds.push_back(AOrigin);
    for (int xs : sign) {
        for (int ys : sign) {
            for (int zs : sign) {
                TraceEnds.push_back(AOrigin + Xv * xs + Yv * ys + Zv * zs);
            }
        }
    }
    TEnumAsByte<ECollisionChannel> TraceChannelProperty = ECC_Visibility;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(EgoVehiclePtr);
    QueryParams.AddIgnoredActor(UEActor);

    bool visible = false;
    for (FVector TraceEnd : TraceEnds) {
        visible = visible || !(World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TraceChannelProperty, QueryParams));
    }
    return visible;
}

void FAwarenessManager::Tick(float DeltaTime)
{
//#if 0 // fails if registry is not perfectly aligned with world (fails on --reloadWorld)
    const FActorRegistry &Registry = Episode->GetActorRegistry();
    FVector ego_velocity = EgoVehiclePtr->GetVelocity();
    std::vector <DReyeVR::AwarenessActor> Visible;
    std::vector<FCarlaActor*> VisibleRaw;
    for (auto It = Registry.begin(); It != Registry.end(); ++It)
    {
        FCarlaActor* Actor = It.Value().Get();
        const AActor *UEActor = Actor->GetActor();
        const FCarlaActor* CarlaActor = Registry.FindCarlaActor(UEActor);
        assert(CarlaActor == Actor);
        if (Actor == nullptr || UEActor == EgoVehiclePtr)
            continue; 

        FCarlaActor::ActorType type = Actor->GetActorType();
        if (type != FCarlaActor::ActorType::Vehicle && type != FCarlaActor::ActorType::Walker) {
            continue;
        }

        FVector Forward = EgoVehiclePtr->GetActorForwardVector();
        FVector Backward = -Forward;
        FVector Right = EgoVehiclePtr->GetActorRightVector();
        FVector Left = -Right;

        //if (WasRecentlyRendered(UEActor)) {

        if (UEActor->WasRecentlyRendered()) {
            if (!is_visible(UEActor))
                continue;
            //is_visible_debug(UEActor, DeltaTime);
            //draw_color_sphere(Actor, World, FColor::Blue);
            DirType ActorDir;
            FVector AbsDirVector;
            FVector vel = UEActor->GetVelocity();
            FVector loc = UEActor->GetActorLocation();
            if (VelMode) {
                AbsDirVector = vel;
            } else if (PosMode) {
                AbsDirVector = loc - EgoVehiclePtr->GetActorLocation();
            } else {
                return;
            }
            GetDirection(AbsDirVector, EgoVehiclePtr, &Zones, &ActorDir);
            int ans = 0;
            if (ActorDir.forward) ans += Encode.Forward;
            if (ActorDir.right) ans += Encode.Right;
            if (ActorDir.backward) ans += Encode.Back;
            if (ActorDir.left) ans += Encode.Left;
            //display_answer(Actor, ans, DeltaTime);
            if (type == FCarlaActor::ActorType::Vehicle) {
                ans += Encode.VehicleType;
            } else {
                ans += Encode.WalkerType;
            }

            DReyeVR::AwarenessActor AwActor;
            AwActor.ActorId = Actor->GetActorId();
            AwActor.ActorLocation = loc;
            AwActor.ActorVelocity = vel;
            AwActor.Answer = ans;
            Visible.push_back(AwActor);
            VisibleRaw.push_back(Actor);
        }      
    }
    int UserInput = 0;
    if (PressedFwdV) UserInput = Encode.Forward | Encode.VehicleType;
    if (PressedRightV) UserInput = Encode.Right | Encode.VehicleType;
    if (PressedBackV) UserInput = Encode.Back | Encode.VehicleType;
    if (PressedLeftV) UserInput = Encode.Left | Encode.VehicleType;
    if (PressedFwdW) UserInput = Encode.Forward | Encode.WalkerType;
    if (PressedRightW) UserInput = Encode.Right | Encode.WalkerType;
    if (PressedBackW) UserInput = Encode.Back | Encode.WalkerType;
    if (PressedLeftW) UserInput = Encode.Left | Encode.WalkerType;
    PressedFwdV = false;
    PressedRightV = false;
    PressedBackV = false;
    PressedLeftV = false;
    PressedFwdW = false;
    PressedRightW = false;
    PressedBackW = false;
    PressedLeftW = false;
    EgoSensor->GetData()->SetAwarenessData(Visible.size(), Visible, VisibleRaw, UserInput);
    //render_ids(VisibleRaw, DeltaTime);
//#endif
}

void FAwarenessManager::SetAwarenessManager(UCarlaEpisode *EpisodeIn, UWorld *WorldIn, ADReyeVRSensor *EgoSensorIn) 
{
    World = WorldIn;
    Episode = EpisodeIn;
    EgoSensor = EgoSensorIn;
    EgoSensor->GetData()->AwarenessMode = true;
}
