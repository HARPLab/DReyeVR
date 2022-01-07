//========= Copyright 2019-2020, HTC Corporation. All rights reserved. ===========

#pragma once

#include "CoreMinimal.h"
#include "SRanipal_Eyes_Enums.h"

/**
 * The internal static class to handle eye framework
 */
class SRANIPALEYE_API SRanipalEye_Framework
{
public:
	SRanipalEye_Framework();
	~SRanipalEye_Framework();

	static SRanipalEye_Framework* Instance();
	static void DestroyEyeFramework();

	int GetStatus();
	// This will be used if no eye tracking support.
	FVector GetCameraPosition();

	enum FrameworkStatus {
		STOP,
		START,
		WORKING,
		ERROR_SRANIPAL, // DReyeVR fix: updated to not conflict with Carla
		NOT_SUPPORT,
	};

	int StartFramework(SupportedEyeVersion version);
	int StopFramework();

	SupportedEyeVersion GetEyeVersion()
	{
		return EyeVersion;
	}

private:
	int Status;
	SupportedEyeVersion EyeVersion;
	static SRanipalEye_Framework* Framework;
	const FString AnipalTypeName = "Eye";
	const FString AnipalTypeName_v2 = "Eye_v2";
};
