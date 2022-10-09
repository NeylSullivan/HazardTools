// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"

#define SETTING_CDO_GET(ThisClassName) \
static const ThisClassName& Get()\
{ \
	return *GetDefault<ThisClassName>(); \
} \
static ThisClassName& GetMutable() \
{ \
	return *GetMutableDefault<ThisClassName>(); \
}
