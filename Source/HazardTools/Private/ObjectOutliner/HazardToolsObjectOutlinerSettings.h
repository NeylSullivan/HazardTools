// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"
#include "HazardToolsSettingsUtils.h"
#include "HazardToolsObjectOutlinerSettings.generated.h"
/**
 * 
 */
UCLASS(config=HazardTools, DefaultConfig)
class HAZARDTOOLS_API UHazardToolsObjectOutlinerSettings : public UObject
{
	GENERATED_BODY()

public:
	SETTING_CDO_GET(ThisClass)

	UPROPERTY(config)
	TSet<FName> ActiveImperativeFilterNames;

	UPROPERTY(config)
	TSet<FName> ActiveDropDownFilterNames;

	UPROPERTY(config)
	bool bShowOnlyCheckedObjects = false;

	// List or Tree
	UPROPERTY(config)
	uint8 DisplayMode = 0; // Default is List

	// Specify which column to sort with
	UPROPERTY(config)
	FName SortByColumn = NAME_Name;

	// Currently selected sorting mode
	UPROPERTY(config)
	uint8 SortMode;
};
