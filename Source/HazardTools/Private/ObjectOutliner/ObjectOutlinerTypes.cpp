// Copyright Neyl Sullivan 2022

#include "ObjectOutlinerTypes.h"
#include "SourceCodeNavigation.h"
#include "ToolMenu.h"
#include "HazardTools.h"
#include "HazardToolsUtils.h"

namespace HazardTools
{
void FObjectOutlinerItemActions::GenerateContextMenu(UToolMenu* Menu, const FObjectOutlinerItemPtr Item)
{
	check(Item.IsValid());
	const UObject* Object = Item->ObjectPtr.Get();

	if (Object == nullptr)
	{
		return;
	}

	FToolMenuSection& Section = Menu->AddSection("ObjectOutlinerContextActions", INVTEXT("Common"));

	Section.AddMenuEntry("OpenSourceFile",
		INVTEXT("Open Source File"),
		FText::GetEmpty(),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "SystemWideCommands.FindInContentBrowser"),
		FUIAction(
			FExecuteAction::CreateStatic(&FObjectOutlinerItemActions::OpenHeaderFile, Object),
			FCanExecuteAction::CreateStatic(&FObjectOutlinerItemActions::CanOpenHeaderFile, Object)
			)
		);

	Section.AddMenuEntry("CopyObjectPath",
		INVTEXT("Copy Object Path"),
		FText::GetEmpty(),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "SystemWideCommands.FindInContentBrowser"),
		FUIAction(
			FExecuteAction::CreateStatic(&FObjectOutlinerItemActions::CopyObjectPath, Object)
			)
		);
}

void FObjectOutlinerItemActions::OpenHeaderFile(const UObject* Object)
{
	if (IsValid(Object) == false)
	{
		return;
	}
	if (FSourceCodeNavigation::CanNavigateToClass(Object->GetClass()))
	{
		FSourceCodeNavigation::NavigateToClass(Object->GetClass());
		return;
	}

	if (FString ClassHeaderPath; FSourceCodeNavigation::FindClassHeaderPath(Object->GetClass(), ClassHeaderPath))
	{
		ClassHeaderPath = FPaths::ConvertRelativePathToFull(ClassHeaderPath);
		UE_LOG(LogHazardTools, Log, TEXT("Trying to open: %s"), *ClassHeaderPath);
		FSourceCodeNavigation::OpenSourceFile(ClassHeaderPath);
	}
}

bool FObjectOutlinerItemActions::CanOpenHeaderFile(const UObject* Object)
{
	return true;
}

void FObjectOutlinerItemActions::CopyObjectPath(const UObject* Object)
{
	if (IsValid(Object))
	{
		const FString ClipboardText = Object->GetPathName();
		FHazardToolsUtils::SetClipboardText(ClipboardText);
	}
}
}
