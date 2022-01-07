//========= Copyright 2019-2020, HTC Corporation. All rights reserved. ===========


#include "SRanipalEye_Framework.h"
#include "SRanipal_API.h"
#include "Eye/SRanipal_API_Eye.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "SRanipalEye.h"


SRanipalEye_Framework* SRanipalEye_Framework::Framework = nullptr;

SRanipalEye_Framework::SRanipalEye_Framework()
{
	Status = FrameworkStatus::STOP;
}

SRanipalEye_Framework::~SRanipalEye_Framework()
{
}

SRanipalEye_Framework* SRanipalEye_Framework::Instance()
{
	if (Framework == nullptr) {
		Framework = new SRanipalEye_Framework();
	}
	return Framework;
}

void SRanipalEye_Framework::DestroyEyeFramework()
{
	if (Framework != nullptr) {
		delete Framework;
		Framework = nullptr;
	}
}

int SRanipalEye_Framework::StartFramework(SupportedEyeVersion version) {
	if (Status == FrameworkStatus::WORKING) {
		UE_LOG(LogSRanipalEye, Log, TEXT("[SRanipal] Framework is already running."));
	}

	Status = FrameworkStatus::START;
	EyeVersion = version;
	if (EyeVersion == SupportedEyeVersion::version1) {
		int* config = 0;
		int result = ViveSR::anipal::Initial(ViveSR::anipal::Eye::ANIPAL_TYPE_EYE, config);
		if (result != ViveSR::Error::WORK) {
			Status = FrameworkStatus::ERROR_SRANIPAL; // DReyeVR fix to not conflict with Carla
			UE_LOG(LogSRanipalEye, Error, TEXT("[SRanipal] Start %s Framework failed: %d"), *AnipalTypeName, result);
		}
		else {
			Status = FrameworkStatus::WORKING;
			UE_LOG(LogSRanipalEye, Log, TEXT("[SRanipal] Start %s Framework success."), *AnipalTypeName);
		}
	}
	else {
		int* config = 0;
		int result = ViveSR::anipal::Initial(ViveSR::anipal::Eye::ANIPAL_TYPE_EYE_V2, config);
		if (result != ViveSR::Error::WORK) {
			Status = FrameworkStatus::ERROR_SRANIPAL; // DReyeVR fix to not conflict with Carla
			UE_LOG(LogSRanipalEye, Error, TEXT("[SRanipal] Start %s Framework failed: %d"), *AnipalTypeName_v2, result);
		}
		else {
			Status = FrameworkStatus::WORKING;
			UE_LOG(LogSRanipalEye, Log, TEXT("[SRanipal] Start %s Framework success."), *AnipalTypeName_v2);
		}
	}

	return Status;
}

int SRanipalEye_Framework::StopFramework() {
	if (Status != FrameworkStatus::NOT_SUPPORT) {
		if (Status == FrameworkStatus::STOP) {
			UE_LOG(LogSRanipalEye, Log, TEXT("[SRanipal] Stop Framework: Module not on"));
		}
		else {
			if (EyeVersion == SupportedEyeVersion::version1) {
				int result = ViveSR::anipal::Release(ViveSR::anipal::Eye::ANIPAL_TYPE_EYE);
				if (result != ViveSR::Error::WORK) {
					UE_LOG(LogSRanipalEye, Error, TEXT("[SRanipal] Stop %s Framework failed: %d"), *AnipalTypeName, result);
				}
				else {
					UE_LOG(LogSRanipalEye, Log, TEXT("[SRanipal] Stop %s Framework success"), *AnipalTypeName);
				}
			}
			else {
				int result = ViveSR::anipal::Release(ViveSR::anipal::Eye::ANIPAL_TYPE_EYE_V2);
				if (result != ViveSR::Error::WORK) {
					UE_LOG(LogSRanipalEye, Error, TEXT("[SRanipal] Stop %s Framework failed: %d"), *AnipalTypeName_v2, result);
				}
				else {
					UE_LOG(LogSRanipalEye, Log, TEXT("[SRanipal] Stop %s Framework success"), *AnipalTypeName_v2);
				}
			}
		}
	}
	Status = FrameworkStatus::STOP;
	return Status;
}

int SRanipalEye_Framework::GetStatus() {
	return Status;
}

FVector SRanipalEye_Framework::GetCameraPosition()
{
	return UGameplayStatics::GetPlayerController(GWorld, 0)->PlayerCameraManager->GetCameraLocation();
}
