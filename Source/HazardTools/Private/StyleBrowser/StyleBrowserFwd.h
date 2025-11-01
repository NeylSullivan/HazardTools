// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"

namespace HazardTools
{
	class FStyleBrowserModel;
	//enum class EDisplayMode : uint8;

	//struct FObjectOutlinerFilter;
	struct FStyleBrowserItem;
	class IDetailsView;

	typedef TSharedPtr<FStyleBrowserItem> FStyleBrowserItemPtr;
	typedef TSharedRef<FStyleBrowserItem> FStyleBrowserItemRef;
}