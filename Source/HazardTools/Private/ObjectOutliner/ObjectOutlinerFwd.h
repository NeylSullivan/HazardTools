// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"

class UHazardToolsObjectOutlinerSettings;

namespace HazardTools
{
class FObjectOutlinerModel;
enum class EDisplayMode : uint8;

struct FObjectOutlinerFilter;
struct FObjectOutlinerItem;
class IDetailsView;

typedef TSharedPtr<FObjectOutlinerItem> FObjectOutlinerItemPtr;
typedef TSharedRef<FObjectOutlinerItem> FObjectOutlinerItemRef;
}
