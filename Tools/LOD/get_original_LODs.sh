#!/bin/bash

OUT=OriginalLODs

mkdir -p ${OUT}

PATH_TO_4WHEELED=Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled

cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/Toyota_Prius/Vh_Car_ToyotaPrius_Rig.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/AudiA2_/SK_AudiA2.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/BmwGranTourer/SK_BMWGranTourer.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/BmwIsetta/SK_BMWIsetta.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/CarlaCola/SK_CarlaCola.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/DodgeCharger2020/SK_Charger2020.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/DodgeCharger2020/ChargerCop/SK_ChargerCop.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/Chevrolet/SK_ChevroletImpala.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/Citroen/SK_Citroen_C3.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/Jeep/SK_JeepWranglerRubicon.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/LincolnMKZ2020/SK_lincolnv5.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/Mercedes/SK_MercedesBenzCoupeC.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/MercedesCCC/SK_MercedesCCC.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/MIni/SK_MiniCooperS.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/Mustang/SK_Mustang_OLD.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/Nissan_Micra/SK_NissanMicra.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/Nissan_Patrol/SK_NissanPatrolST.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/Leon/SK_SeatLeon.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/AudiTT/SM_AudiTT_v1.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/DodgeCharge/SM_Charger_v2.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/Chevrolet/SM_Chevrolet_v2.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/Cybertruck/SM_Cybertruck_v2.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/AudiETron/SM_Etron.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/LincolnMKZ2017/SM_Lincoln_v2.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/Mustang/SM_Mustang_v2.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/Tesla/SM_TeslaM3_v2.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/Truck/SM_TestTruck.uasset ${OUT}
cp ${CARLA_ROOT}/${PATH_TO_4WHEELED}/VolkswagenT2/SM_Van_v2.uasset ${OUT}