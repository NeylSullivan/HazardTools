// Copyright Neyl Sullivan 2022

#include "ObjectOutlinerModel.h"
#include "ObjectOutlinerTypes.h"

namespace HazardTools
{
FObjectOutlinerModel::FObjectOutlinerModel()
{
}

void FObjectOutlinerModel::UpdateContent(const bool bHierarchical, TMap<UObject*, FObjectOutlinerItemPtr>* OutProcessedObjectsMapPtr /*= nullptr*/)
{
	RootContent.Reset();
	DiscoveredNum = 0;
	FilteredNum = 0;
	DisplayedNum = 0;

	if (OutProcessedObjectsMapPtr != nullptr)
	{
		OutProcessedObjectsMapPtr->Reset();
	}

	{
		TSet<UObject*> FilteredObjectsSet;

		for (FThreadSafeObjectIterator It; It; ++It)
		{
			DiscoveredNum++;

			if (ShouldItemPassFilterDelegate.IsBound() && ShouldItemPassFilterDelegate.Execute(*It) == false)
			{
				continue;;
			}

			FilteredNum++;

			if (ShouldItemPassTextFilterDelegate.IsBound() && ShouldItemPassTextFilterDelegate.Execute(*It) == false)
			{
				continue;;
			}

			if (bHierarchical)
			{
				FilteredObjectsSet.Add(*It); // Process it later	
			}
			else
			{
				RootContent.Add(MakeShared<FObjectOutlinerItem>(*It));
				if (OutProcessedObjectsMapPtr != nullptr)
				{
					OutProcessedObjectsMapPtr->Add(*It, RootContent.Last());
				}
			}
		}

		if (bHierarchical)
		{
			TMap<UObject*, FObjectOutlinerItemPtr> LocalProcessedObjectsMap;
			TMap<UObject*, FObjectOutlinerItemPtr>& ProcessedObjectsMapRef = OutProcessedObjectsMapPtr ? *OutProcessedObjectsMapPtr : LocalProcessedObjectsMap;

			for (auto It = FilteredObjectsSet.CreateIterator(); It; ++It)
			{
				AddItemToTreeView(*It, ProcessedObjectsMapRef, true);
			}
		}

		DisplayedNum = RootContent.Num();
	}
}

FObjectOutlinerItemPtr FObjectOutlinerModel::AddItemToTreeView(UObject* NewItemObjectPtr, TMap<UObject*, FObjectOutlinerItemPtr>& ProcessedObjectsMap, const bool bExplicitlyAdded)
{
	check(NewItemObjectPtr != nullptr)

	// Ensure parent exist
	UObject* OuterPtr = NewItemObjectPtr->GetOuter();
	const FObjectOutlinerItemPtr ParentItem = OuterPtr ? AddItemToTreeView(OuterPtr, ProcessedObjectsMap, false) : nullptr;

	// Can already be processed as other item parent/outer, do nothing then
	if (FObjectOutlinerItemPtr* AlreadyProcessedItem = ProcessedObjectsMap.Find(NewItemObjectPtr))
	{
		if (bExplicitlyAdded)
		{
			AlreadyProcessedItem->Get()->bIsExplicitlyAdded = true;
		}
		return *AlreadyProcessedItem;
	}

	FObjectOutlinerItemPtr NewItem = MakeShared<FObjectOutlinerItem>(NewItemObjectPtr);
	NewItem->bIsExplicitlyAdded = bExplicitlyAdded;

	ProcessedObjectsMap.Add(NewItemObjectPtr, NewItem);

	if (ParentItem.IsValid())
	{
		ParentItem->AddChild(NewItem->AsShared());
	}
	else
	{
		RootContent.Add(NewItem);
	}

	return NewItem;
}
}
