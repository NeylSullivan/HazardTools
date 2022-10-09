// Copyright Neyl Sullivan 2022

#pragma once
#include "ObjectOutlinerFwd.h"

namespace HazardTools
{
enum class EDisplayMode : uint8
{
	List,
	Tree
};

struct FObjectOutlinerItem : TSharedFromThis<FObjectOutlinerItem>
{
	explicit FObjectOutlinerItem(UObject* InPtr)
		: ObjectPtr(InPtr)
	{
	}

	TWeakObjectPtr<UObject> ObjectPtr;

	/** This item's parent, if any. */
	TWeakPtr<FObjectOutlinerItem> Parent;

	/** Array of children contained underneath this item */
	mutable TSet<FObjectOutlinerItemPtr> Children;

	// Set it to false for tree items displayed only cause on their child pass filter, but not this item.
	// False will add opacity to tree view item;
	bool bIsExplicitlyAdded = true;

	bool bChildrenRequireSort = true;

	void AddChild(const TSharedRef<FObjectOutlinerItem> Child)
	{
		check(!Children.Contains(Child));
		check(Child->Parent.Pin() == nullptr)
		Child->Parent = AsShared();
		Children.Add(Child);
		bChildrenRequireSort = true; // TODO ?
	}

	FObjectOutlinerItemPtr GetParent() const
	{
		return Parent.Pin();
	}

	const TSet<FObjectOutlinerItemPtr>& GetChildren() const
	{
		return Children;
	}
};

class FObjectOutlinerItemActions
{
public:
	static void GenerateContextMenu(UToolMenu* Menu, FObjectOutlinerItemPtr Item);

private:
	static void OpenHeaderFile(const UObject* Object);
	static bool CanOpenHeaderFile(const UObject* Object);

	static void CopyObjectPath(const UObject* Object);
};
}
