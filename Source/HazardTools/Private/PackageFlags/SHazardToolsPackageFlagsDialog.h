// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"
#include "HazardToolsPackageFlagsTypes.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"

namespace HazardToolsPackagesFlagsColumns
{
static const FName Flag("Flag");
static const FName State("State");
static const FName Info("Info");
};

class SHazardToolsPackagesFlagsRow : public SMultiColumnTableRow<TSharedRef<FHazardToolsPackageFlagsDescription>>
{
	SLATE_BEGIN_ARGS(SHazardToolsPackagesFlagsRow)
		{
		}

	SLATE_END_ARGS()

	TSharedPtr<FHazardToolsPackageFlagsDescription> Item;
	TSharedPtr<STableViewBase> OwnerTable;

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedRef<FHazardToolsPackageFlagsDescription> InNode)
	{
		Item = InNode;
		OwnerTable = InOwnerTable;

		SMultiColumnTableRow<TSharedRef<FHazardToolsPackageFlagsDescription>>::Construct(
			FSuperRowType::FArguments()
			.Padding(1.0f),
			InOwnerTable
			);
	}

	void OnStateChanged(const ECheckBoxState NewState) const
	{
		if (NewState == ECheckBoxState::Checked)
		{
			Item.Get()->OutState = FHazardToolsPackageFlagsDescription::EState::Checked;
		}
		else if (NewState == ECheckBoxState::Unchecked)
		{
			Item.Get()->OutState = FHazardToolsPackageFlagsDescription::EState::Unchecked;
		}

		if (Item.Get()->OutState == Item.Get()->InState)
		{
			Item.Get()->OutState = FHazardToolsPackageFlagsDescription::EState::NotInitialized; //reset to default and use InState in ListView instead
		}

		if (OwnerTable.IsValid())
		{
			OwnerTable.Get()->RebuildList();
		}
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		if (InColumnName == HazardToolsPackagesFlagsColumns::Flag)
		{
			return
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
				[
					SNew(STextBlock)
					.Text(Item.Get()->DisplayName)
					.ToolTipText(Item.Get()->Tooltip)
				];
		}
		if (InColumnName == HazardToolsPackagesFlagsColumns::State)
		{
			ECheckBoxState CheckBoxState = ECheckBoxState::Unchecked;
			if (Item.Get()->OutState == FHazardToolsPackageFlagsDescription::EState::NotInitialized) //get initial state
			{
				if (Item.Get()->InState == FHazardToolsPackageFlagsDescription::EState::Checked)
				{
					CheckBoxState = ECheckBoxState::Checked;
				}
				else if (Item.Get()->InState == FHazardToolsPackageFlagsDescription::EState::Mixed)
				{
					CheckBoxState = ECheckBoxState::Undetermined;
				}
			}
			else //get new updated state
			{
				if (Item.Get()->OutState == FHazardToolsPackageFlagsDescription::EState::Checked)
				{
					CheckBoxState = ECheckBoxState::Checked;
				}
				//cannot be ECheckBoxState::Undetermined
			}

			return
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
				[
					SNew(SCheckBox)
					.IsChecked(CheckBoxState)
					.OnCheckStateChanged(this, &SHazardToolsPackagesFlagsRow::OnStateChanged)
				];
		}

		if (InColumnName == HazardToolsPackagesFlagsColumns::Info)
		{
			return
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
				[
					SNew(STextBlock)
					.Text(Item.Get()->Info)
					.ToolTipText(Item.Get()->InfoTooltip)
				];
		}

		return SNullWidget::NullWidget;
	}
};


class SHazardToolsPackagesFlagsDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHazardToolsPackagesFlagsDialog)
		{
		}

		SLATE_ATTRIBUTE(TSharedPtr<SWindow>, ParentWindow)
		SLATE_ATTRIBUTE(TArray<FHazardToolsPackageFlagsDescription>, FlagsDescriptions)
		SLATE_ATTRIBUTE(uint32, TotalSelectedItemsNum)
		SLATE_ATTRIBUTE(FText, SelectedItemsInfo)


	SLATE_END_ARGS()

	/** A default window size for the dialog */
	static const FVector2D Default_Window_Size;
private:
	/** Pointer to the parent window, so we know to destroy it when done */
	TArray<TSharedPtr<FHazardToolsPackageFlagsDescription>> FlagsDescriptions;
	TWeakPtr<SWindow> ParentWindowPtr;
	TSharedPtr<SButton> OkayButton;
	TSharedPtr<SListView<TSharedPtr<FHazardToolsPackageFlagsDescription>>> ListView;
	int32 TotalSelectedItemsNum = 0;
	FText SelectedItemsInfo;

	bool bUserClickedOkay = false;
public:
	void Construct(const FArguments& InArgs)
	{
		ParentWindowPtr = InArgs._ParentWindow.Get();
		FlagsDescriptions.Empty(InArgs._FlagsDescriptions.Get().Num());
		TotalSelectedItemsNum = InArgs._TotalSelectedItemsNum.Get();
		SelectedItemsInfo = InArgs._SelectedItemsInfo.Get();

		for (FHazardToolsPackageFlagsDescription Description : InArgs._FlagsDescriptions.Get())
		{
			FlagsDescriptions.Add(MakeShared<FHazardToolsPackageFlagsDescription>(Description));
		}
		bUserClickedOkay = false;

		ListView = SNew(SListView< TSharedPtr<FHazardToolsPackageFlagsDescription> >)
		// UE_5.5 - deprecated slate attribute
		//.ItemHeight(24)
		.SelectionMode(ESelectionMode::None)
		.ListItemsSource(&FlagsDescriptions)
		.OnGenerateRow(this, &SHazardToolsPackagesFlagsDialog::OnGenerateRowForList)
		.HeaderRow(SNew(SHeaderRow)

		           + SHeaderRow::Column(HazardToolsPackagesFlagsColumns::State)
		             .DefaultLabel(FText::FromName(HazardToolsPackagesFlagsColumns::State))
		             .FixedWidth(70)
		             .HAlignHeader(HAlign_Center)
		             .HAlignCell(HAlign_Center)

		           + SHeaderRow::Column(HazardToolsPackagesFlagsColumns::Flag)
		             .DefaultLabel(FText::FromName(HazardToolsPackagesFlagsColumns::Flag))
		             .HAlignHeader(HAlign_Left)
		             .HAlignCell(HAlign_Left)
		             .ManualWidth(200)

		           + SHeaderRow::Column(HazardToolsPackagesFlagsColumns::Info)
		             .DefaultLabel(FText::FromName(HazardToolsPackagesFlagsColumns::Info))
		             .HAlignHeader(HAlign_Left)
		             .HAlignCell(HAlign_Left)
			                                                                             );

		this->ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)

				// Title
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .Padding(2)
				  .HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(FAppStyle::Get(), "NewClassDialog.PageTitle")
					.Text(FText::FromString(FString::Printf(TEXT("%i Assets Selected"), TotalSelectedItemsNum)))
					.ToolTipText(SelectedItemsInfo)
				]

				// Title spacer
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .Padding(0, 2, 0, 8)
				[
					SNew(SSeparator)
				]

				+ SVerticalBox::Slot()
				.FillHeight(1)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					.Padding(4)
					[
						ListView.ToSharedRef()
					]
				]
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .HAlign(HAlign_Right)
				  .Padding(6, 2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					  .AutoWidth()
					  .HAlign(HAlign_Left)
					  .VAlign(VAlign_Bottom)
					  .Padding(12.0f)
					[
						SAssignNew(OkayButton, SButton)
						.ContentPadding(FMargin(10, 3))
						.Text(FText::FromString("  Ok  "))
						.OnClicked(this, &SHazardToolsPackagesFlagsDialog::OnOkayClicked)
						.IsEnabled(this, &SHazardToolsPackagesFlagsDialog::GetIsOkayButtonEnabled)
					]
					+ SHorizontalBox::Slot()
					  .AutoWidth()
					  .HAlign(HAlign_Left)
					  .VAlign(VAlign_Bottom)
					  .Padding(12.0f)
					[
						SNew(SButton)
						.ContentPadding(FMargin(10, 3))
						.Text(FText::FromString("Cancel"))
						.OnClicked(this, &SHazardToolsPackagesFlagsDialog::OnCancelClicked)
					]
				]
			]
		];

		ParentWindowPtr.Pin().Get()->SetWidgetToFocusOnActivate(ListView);
	}

	bool IsOkayClicked() const { return bUserClickedOkay; }

	void FillFlagsDescriptions(TArray<FHazardToolsPackageFlagsDescription>& InOutFlagsDescriptions) const
	{
		for (int i = 0; i < FlagsDescriptions.Num(); ++i)
		{
			TSharedPtr<FHazardToolsPackageFlagsDescription> Data = FlagsDescriptions[i];
			if (Data.IsValid())
			{
				InOutFlagsDescriptions[i].OutState = Data.Get()->OutState;
			}
		}
	}

private:
	TSharedRef<ITableRow> OnGenerateRowForList(TSharedPtr<FHazardToolsPackageFlagsDescription> Item, const TSharedRef<STableViewBase>& OwnerTable) const
	{
		ListView->SetItemHighlighted(Item, Item.Get()->OutState != FHazardToolsPackageFlagsDescription::EState::NotInitialized);

		return
			SNew(SHazardToolsPackagesFlagsRow, OwnerTable, Item.ToSharedRef());
	}

	bool GetIsOkayButtonEnabled() const
	{
		for (TSharedPtr<FHazardToolsPackageFlagsDescription> Description : FlagsDescriptions)
		{
			if (Description.IsValid() && Description.Get()->OutState != FHazardToolsPackageFlagsDescription::EState::NotInitialized)
			{
				return true;
			}
		}
		return false;
	}

	FReply OnOkayClicked()
	{
		bUserClickedOkay = true;
		ParentWindowPtr.Pin()->RequestDestroyWindow();
		return FReply::Handled();
	}

	FReply OnCancelClicked()
	{
		bUserClickedOkay = false;
		ParentWindowPtr.Pin()->RequestDestroyWindow();
		return FReply::Handled();
	}

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		if (InKeyEvent.GetKey() == EKeys::Escape)
		{
			return OnCancelClicked();
		}
		if (OkayButton.Get()->IsEnabled() && InKeyEvent.GetKey() == EKeys::Enter)
		{
			return OnOkayClicked();
		}

		return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
	}
};

const FVector2D SHazardToolsPackagesFlagsDialog::Default_Window_Size = FVector2D(640, 560);
