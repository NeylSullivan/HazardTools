// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"
#include "HazardToolsSettingsUtils.h"
#include "HazardToolsStyleBrowserSettings.generated.h"
/**
 * 
 */
UCLASS(config=HazardTools, DefaultConfig)
class HAZARDTOOLS_API UHazardToolsStyleBrowserSettings : public UObject
{
	GENERATED_BODY()

public:
	SETTING_CDO_GET(ThisClass)

	UPROPERTY(config)
	bool bShouldAddFromParentStyle = true;

	UPROPERTY(config)
	FName StyleTypeNameToDisplay = NAME_None;
};
