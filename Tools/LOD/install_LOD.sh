#!/bin/bash

DReyeVR_ROOT=$PWD
CARLA_ROOT=$1
PLATFORM=$2

R='\033[0;31m'         # Red
G='\033[0;32m'         # Green
Y='\033[0;33m'         # Yellow
B='\033[0;34m'         # Blue
NC='\033[0m'           # No Color

if [ -z "$1" ]; then
    echo -e "${R}Enter the location for Carla${NC}"
    exit 1
fi

if [ -z "$2" ]; then
    echo -e "${R}Enter the platform for downloading LODs: One of \"Windows\"/\"Linux\"/\"Original\"${NC}"
    exit 1
fi

echo -e "\nInstalling custom LOD's for DReyeVR"

if [ ! -f ${CARLA_ROOT}/CHANGELOG.md ] || [[ `head -n 1 ${CARLA_ROOT}/CHANGELOG.md` != "## CARLA 0.9.13" ]]; then
    echo -e "${R}Could not verify a correct Carla 0.9.13 installation${NC}"
    exit 1
else    
    echo -e "${G}Found a Carla 0.9.13 installation at ${CARLA_ROOT}${NC}"
fi

LINUX='https://docs.google.com/uc?export=download&id=1OqDOCAflENnXvbJCogBEmRhHQpEF1aKE'
WINDOWS='https://docs.google.com/uc?export=download&id=191tiK25MJ9C7-5Q1-sHt1mp4_EaefjqM'
ORIGINAL='https://docs.google.com/uc?export=download&id=1Vc4e43xZuXOJF_3-r3n3QU-yE5sjiAfw'

if [ ${PLATFORM} == "Linux" ]; then
    CONTENT=${LINUX}
    FILENAME="LinuxLODs.zip"
elif [ ${PLATFORM} == "Windows" ]; then
    CONTENT=${WINDOWS}
    FILENAME="WindowsLODs.zip"
elif [ ${PLATFORM} == "Original" ]; then
    CONTENT=${ORIGINAL}
    FILENAME="OriginalLODs.zip"
else
    echo -e "${R}Could not verify a correct platform (must one of \"Windows\"/\"Linux\"/\"Original\")${NC}"
    exit 1
fi
echo -e "${B}Downloading ${FILENAME} LODs package...${NC}"

wget -r -nv ${CONTENT} -O ${FILENAME}

OUT_DIR=${FILENAME::-4} # get rid of .zip

unzip ${FILENAME} -d ${OUT_DIR}

echo -e "${B}Copying files from ${OUT_DIR}...${NC}"

mkdir -p $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/DReyeVR/
cp SM_Vehicle_LODSettings.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/DReyeVR/
# now copy all individual static meshes
# note you can get all the file paths easily in UE4 (with the folderless+filtered mode) right-click -> copy file paths
cp ${OUT_DIR}/Vh_Car_ToyotaPrius_Rig.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/Toyota_Prius/
cp ${OUT_DIR}/SK_AudiA2.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/AudiA2_/
cp ${OUT_DIR}/SK_BMWGranTourer.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/BmwGranTourer/
cp ${OUT_DIR}/SK_BMWIsetta.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/BmwIsetta/
cp ${OUT_DIR}/SK_CarlaCola.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/CarlaCola/
cp ${OUT_DIR}/SK_Charger2020.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/DodgeCharger2020/
cp ${OUT_DIR}/SK_ChargerCop.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/DodgeCharger2020/ChargerCop/
cp ${OUT_DIR}/SK_ChevroletImpala.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/Chevrolet/
cp ${OUT_DIR}/SK_Citroen_C3.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/Citroen/
cp ${OUT_DIR}/SK_JeepWranglerRubicon.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/Jeep/
cp ${OUT_DIR}/SK_lincolnv5.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/LincolnMKZ2020/
cp ${OUT_DIR}/SK_MercedesBenzCoupeC.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/Mercedes/
cp ${OUT_DIR}/SK_MercedesCCC.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/MercedesCCC/
cp ${OUT_DIR}/SK_MiniCooperS.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/MIni/
cp ${OUT_DIR}/SK_Mustang_OLD.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/Mustang/
cp ${OUT_DIR}/SK_NissanMicra.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/Nissan_Micra/
cp ${OUT_DIR}/SK_NissanPatrolST.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/Nissan_Patrol/
cp ${OUT_DIR}/SK_SeatLeon.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/Leon/
cp ${OUT_DIR}/SM_AudiTT_v1.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/AudiTT/
cp ${OUT_DIR}/SM_Charger_v2.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/DodgeCharge/
cp ${OUT_DIR}/SM_Chevrolet_v2.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/Chevrolet/
cp ${OUT_DIR}/SM_Cybertruck_v2.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/Cybertruck/
cp ${OUT_DIR}/SM_Etron.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/AudiETron/
cp ${OUT_DIR}/SM_Lincoln_v2.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/LincolnMKZ2017/
cp ${OUT_DIR}/SM_Mustang_v2.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/Mustang/
cp ${OUT_DIR}/SM_TeslaM3_v2.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/Tesla/
cp ${OUT_DIR}/SM_TestTruck.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/Truck/
cp ${OUT_DIR}/SM_Van_v2.uasset $CARLA_ROOT/Unreal/CarlaUE4/Content/Carla/Static/Vehicles/4Wheeled/VolkswagenT2/

echo -e "\n${G}Installation successful!${NC}"