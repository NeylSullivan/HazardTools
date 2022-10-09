// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"
#include "HazardToolsSettings.generated.h"

/**
 * Global plugin settings repository
 *
 * With "DefaultConfig" specifier save setting to "Project\Config\DefaultHazardTools.ini" 
 */
UCLASS(config=HazardTools, DefaultConfig, Abstract)
class HAZARDTOOLS_API UHazardToolsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// UDeveloperSettings overrides
	virtual FName GetContainerName() const override { return FName("Editor"); }
	virtual FName GetCategoryName() const override { return FName("Plugins"); }
	virtual FText GetSectionText() const override { return INVTEXT("Hazard Tools"); }
	virtual FText GetSectionDescription() const override { return INVTEXT("Configure the Hazard Tools plugin"); }
};
