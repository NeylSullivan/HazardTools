// Copyright Neyl Sullivan 2022

#include "SObjectOutlinerTableRow.h"

#include "AssetViewUtils.h"
#include "ObjectOutlinerTypes.h"
#include "SourceCodeNavigation.h"
#include "Styling/SlateIconFinder.h"

namespace HazardTools
{
void SObjectOutlinerTableRow::Construct(const FArguments& /*InArgs*/,
	const TSharedRef<STableViewBase>& InOwnerTableView,
	const FObjectOutlinerItemRef InItem,
	const TSharedRef<SObjectOutliner> InOwnerObjectOutliner)
{
	SceneOutlinerWeakPtr = InOwnerObjectOutliner;
	Item = InItem;

	if (const UObject* Obj = Item.IsValid() ? Item->ObjectPtr.Get() : nullptr)
	{
		Name = FText::FromString(Obj->GetName());
		ClassName = Obj->GetClass()->GetDisplayNameText();

		Package = FText::FromString(Obj->GetPathName());
		// Get selection icon based on actor(s) classes and add before the selection label
		ClassIcon = FSlateIconFinder::FindIconForClass(Obj->GetClass()).GetIcon();
		ContentColor = FSlateColor::UseForeground();
		if (Obj->IsTemplate(RF_ClassDefaultObject))
		{
			static FSlateColor ColorAccentOrange = FAppStyle::Get().GetSlateColor("Colors.AccentOrange");
			ContentColor = ColorAccentOrange;
		}
		else if (Obj->IsA(UPackage::StaticClass()))
		{
			static FSlateColor ColorAccentPink = FAppStyle::Get().GetSlateColor("Colors.AccentPink");
			ContentColor = ColorAccentPink;
		}

		if (Item->bIsExplicitlyAdded == false)
		{
			ContentColor = ContentColor.UseSubduedForeground();
		}

		FTextBuilder TooltipBuilder;

		TooltipBuilder.AppendLineFormat(INVTEXT("Class: {0}"), ClassName);
		TooltipBuilder.AppendLineFormat(INVTEXT("Package: {0}"), Package);

		TooltipBuilder.AppendLine();
		TooltipBuilder.AppendLine(Obj->GetClass()->GetToolTipText());

		if (const UObject* ObjOuter = Obj->GetOuter())
		{
			TooltipBuilder.AppendLine();
			TooltipBuilder.AppendLineFormat(INVTEXT("Outer Class: {0} Name: {1}"), ObjOuter->GetClass()->GetDisplayNameText(), FText::FromString(ObjOuter->GetName()));

			if (const UWorld* ObjWorld = ObjOuter->GetWorld())
			{
				TooltipBuilder.AppendLineFormat(INVTEXT("World: {0}"), FText::FromString(ObjWorld->GetName()));
			}
		}

		if (FString ClassModuleName; FSourceCodeNavigation::FindClassModuleName(Obj->GetClass(), ClassModuleName))
		{
			TooltipBuilder.AppendLine();
			TooltipBuilder.AppendLineFormat(INVTEXT("Module: {0}"), FText::FromString(ClassModuleName));
		}

		if (FString ClassHeaderPath; FSourceCodeNavigation::FindClassHeaderPath(Obj->GetClass(), ClassHeaderPath))
		{
			TooltipBuilder.AppendLine();
			TooltipBuilder.AppendLineFormat(INVTEXT("Header: {0}"), FText::FromString(ClassHeaderPath));
		}

		if (FString ClassSourcePath; FSourceCodeNavigation::FindClassSourcePath(Obj->GetClass(), ClassSourcePath))
		{
			TooltipBuilder.AppendLineFormat(INVTEXT("Source: {0}"), FText::FromString(ClassSourcePath));
		}

		SetToolTipText(TooltipBuilder.ToText());
	}

	SMultiColumnTableRow<FObjectOutlinerItemRef>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
}

TSharedRef<SWidget> SObjectOutlinerTableRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (const auto ObjectBrowser = SceneOutlinerWeakPtr.Pin())
	{
		HighlightText = ObjectBrowser->GetTextFilterHighlightText();
	}

	if (ColumnName == SObjectOutliner::Column_ID_Name)
	{
		return SNew(SBox)
		       .MinDesiredHeight(20.f)
		       [
			       SNew(SHorizontalBox)

			       + SHorizontalBox::Slot()
			         .AutoWidth()
			         .Padding(6, 0, 0, 0)
			       [
				       SNew(SExpanderArrow, SharedThis(this))
						.IndentAmount(12)
						.Visibility_Lambda([&]()
				                                             {
					                                             if (const auto ObjectBrowser = SceneOutlinerWeakPtr.Pin())
					                                             {
						                                             if (ObjectBrowser->IsTreeViewMode())
						                                             {
							                                             return EVisibility::Visible;
						                                             }
					                                             }
					                                             return EVisibility::Collapsed;
				                                             })
			       ]

			       + SHorizontalBox::Slot()
			         .AutoWidth()
			         .VAlign(VAlign_Center)
			         .Padding(FMargin(0.f, 1.f, 6.f, 1.f))
			       [
				       SNew(SBox)
						.WidthOverride(16.f)
						.HeightOverride(16.f)
				       [
					       SNew(SImage)
							.Image(ClassIcon)
							.ColorAndOpacity(ContentColor)
				       ]
			       ]

			       + SHorizontalBox::Slot()
			         .FillWidth(1.0f)
			         .VAlign(VAlign_Center)
			         .Padding(0.0f, 0.0f)
			       [
				       SNew(STextBlock)
						.Text(Name)
						.HighlightText(HighlightText)
						.ColorAndOpacity(ContentColor)
			       ]
		       ];
	}
	if (ColumnName == SObjectOutliner::Column_ID_Class)
	{
		return
			SNew(STextBlock)
			.Text(ClassName)
			.HighlightText(HighlightText)
			.ColorAndOpacity(ContentColor);
	}

	if (ColumnName == SObjectOutliner::Column_ID_Memory)
	{
		return
			SNew(STextBlock)
			.Text_Lambda([&]() { return GetMemoryText(); })
			.ColorAndOpacity(ContentColor);
	}

	return SNullWidget::NullWidget;
}

FReply SObjectOutlinerTableRow::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (UObject* LiveObject = Item->ObjectPtr.Get())
	{
		AssetViewUtils::OpenEditorForAsset(LiveObject);
	}

	return FReply::Handled();
}

FText SObjectOutlinerTableRow::GetMemoryText() const
{
	if (UObject* Obj = Item->ObjectPtr.Get())
	{
		const SIZE_T Size = Obj->GetResourceSizeBytes(EResourceSizeMode::Exclusive);

		return FText::AsMemory(Size, IEC);
	}
	return INVTEXT("-");
}
}
