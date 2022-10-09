// Copyright Neyl Sullivan 2022

#pragma once
#include "ObjectOutlinerFwd.h"

namespace HazardTools
{
class FObjectOutlinerModel : public TSharedFromThis<FObjectOutlinerModel>
{
public:
	FObjectOutlinerModel();

	DECLARE_DELEGATE_RetVal_OneParam(bool, FShouldPassItem, const UObject* ObjectPtr);

	TSharedRef<FObjectOutlinerModel> SetShouldPassFilter(const FShouldPassItem& InShouldItemPassFilterDelegate)
	{
		ShouldItemPassFilterDelegate = InShouldItemPassFilterDelegate;
		return this->AsShared();
	}

	TSharedRef<FObjectOutlinerModel> SetShouldPassTextFilter(const FShouldPassItem& InShouldItemPassTextFilterDelegate)
	{
		ShouldItemPassTextFilterDelegate = InShouldItemPassTextFilterDelegate;
		return this->AsShared();
	}

	const TArray<FObjectOutlinerItemPtr>& GetRootContent() const
	{
		return RootContent;
	}

	TArray<FObjectOutlinerItemPtr>& GetMutableRootContent()
	{
		return RootContent;
	}

	[[nodiscard]] int32 GetDiscoveredNum() const { return DiscoveredNum; }
	[[nodiscard]] int32 GetFilteredNum() const { return FilteredNum; }
	[[nodiscard]] int32 GetDisplayedNum() const { return DisplayedNum; }

	void UpdateContent(const bool bHierarchical, TMap<UObject*, FObjectOutlinerItemPtr>* OutProcessedObjectsMapPtr = nullptr);

private:
	FObjectOutlinerItemPtr AddItemToTreeView(UObject* NewItemObjectPtr, TMap<UObject*, FObjectOutlinerItemPtr>& ProcessedObjectsMap, const bool bExplicitlyAdded);

	TArray<FObjectOutlinerItemPtr> RootContent;

	FShouldPassItem ShouldItemPassFilterDelegate;
	FShouldPassItem ShouldItemPassTextFilterDelegate;

	int32 DiscoveredNum = 0;
	int32 FilteredNum = 0;
	int32 DisplayedNum = 0;
};
}
