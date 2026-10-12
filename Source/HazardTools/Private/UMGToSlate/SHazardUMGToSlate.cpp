// Copyright Neyl Sullivan 2026

#include "UMGToSlate/SHazardUMGToSlate.h"
#include "UMGToSlate/HazardUMGToSlateGenerator.h"

#include "Framework/Docking/TabManager.h"
#include "Framework/Text/PlainTextLayoutMarshaller.h"
#include "Framework/Text/SlateTextRun.h"
#include "Framework/Text/TextLayout.h"
#include "HAL/PlatformApplicationMisc.h"
#include "PropertyCustomizationHelpers.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditor.h"
#include "WidgetBlueprintToolMenuContext.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace HazardTools
{
	namespace
	{
		/** C++ lexer for the generated source. The marshaller keeps the clipboard text unmodified. */
		class FSlateCodeMarshaller : public FPlainTextLayoutMarshaller
		{
		public:
			FSlateCodeMarshaller()
			{
				const FTextBlockStyle Base = FTextBlockStyle().SetFont(FCoreStyle::GetDefaultFontStyle("Mono", 10));
				Normal = Base;
				Normal.SetColorAndOpacity(FLinearColor(0.82f, 0.82f, 0.82f));
				Keyword = Base;
				Keyword.SetColorAndOpacity(FLinearColor(0.35f, 0.65f, 1.0f));
				String = Base;
				String.SetColorAndOpacity(FLinearColor(0.9f, 0.65f, 0.4f));
				CommentStyle = Base;
				CommentStyle.SetColorAndOpacity(FLinearColor(0.4f, 0.7f, 0.4f));
				Numeric = Base;
				Numeric.SetColorAndOpacity(FLinearColor(0.7f, 0.85f, 0.6f));
				Type = Base;
				Type.SetColorAndOpacity(FLinearColor(0.3f, 0.8f, 0.8f));
			}

			virtual void SetText(const FString& SourceString, FTextLayout& Layout) override
			{
				static const TSet<FString> Keywords = { TEXT("class"), TEXT("struct"), TEXT("public"), TEXT("private"), TEXT("protected"), TEXT("const"), TEXT("void"), TEXT("return"), TEXT("true"), TEXT("false"), TEXT("nullptr"), TEXT("float"), TEXT("int32"), TEXT("bool"), TEXT("auto"), TEXT("virtual"), TEXT("override") };
				TArray<FTextRange> Ranges;
				FTextRange::CalculateLineRangesFromString(SourceString, Ranges);
				TArray<FTextLayout::FNewLineData> Lines;
				bool bBlockComment = false;
				for (const FTextRange& Range : Ranges)
				{
					TSharedRef<FString> Line = MakeShared<FString>(SourceString.Mid(Range.BeginIndex, Range.Len()));
					TArray<TSharedRef<IRun>> Runs;
					int32 Pos = 0;
					while (Pos < Line->Len())
					{
						const int32 Start = Pos;
						const TCHAR C = (*Line)[Pos];
						const FTextBlockStyle* Style = &Normal;
						if (bBlockComment || Line->Mid(Pos, 2) == TEXT("/*"))
						{
							Style = &CommentStyle;
							if (!bBlockComment) { Pos += 2; }
							const int32 End = Line->Find(TEXT("*/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Pos);
							bBlockComment = End == INDEX_NONE;
							Pos = bBlockComment ? Line->Len() : End + 2;
						}
						else if (Line->Mid(Pos, 2) == TEXT("//"))
						{
							Style = &CommentStyle;
							Pos = Line->Len();
						}
						else if (C == '"' || C == '\'')
						{
							Style = &String;
							++Pos;
							while (Pos < Line->Len())
							{
								if ((*Line)[Pos] == '\\') { Pos = FMath::Min(Pos + 2, Line->Len()); }
								else
									if ((*Line)[Pos++] == C) { break; }
							}
						}
						else if (C == '#')
						{
							Style = &Keyword;
							++Pos;
							while (Pos < Line->Len() && FChar::IsAlpha((*Line)[Pos])) { ++Pos; }
						}
						else if (FChar::IsDigit(C))
						{
							Style = &Numeric;
							++Pos;
							while (Pos < Line->Len() && (FChar::IsAlnum((*Line)[Pos]) || (*Line)[Pos] == '.')) { ++Pos; }
						}
						else if (FChar::IsAlpha(C) || C == '_')
						{
							++Pos;
							while (Pos < Line->Len() && (FChar::IsAlnum((*Line)[Pos]) || (*Line)[Pos] == '_')) { ++Pos; }
							const FString Word = Line->Mid(Start, Pos - Start);
							if (Keywords.Contains(Word) || Word == TEXT("SNew") || Word == TEXT("TEXT") || Word.StartsWith(TEXT("SLATE_"))) { Style = &Keyword; }
							else
								if (FChar::IsUpper(C)) { Style = &Type; }
						}
						else { ++Pos; }
						Runs.Add(FSlateTextRun::Create(FRunInfo(), Line, *Style, FTextRange(Start, Pos)));
					}
					if (Runs.IsEmpty()) { Runs.Add(FSlateTextRun::Create(FRunInfo(), Line, Normal)); }
					Lines.Emplace(Line, MoveTemp(Runs));
				}
				Layout.AddLines(Lines);
			}

		private:
			FTextBlockStyle Normal, Keyword, String, CommentStyle, Numeric, Type;
		};
	}

	const FName SHazardUMGToSlate::TabName(TEXT("HazardToolsUMGToSlate"));

	void SHazardUMGToSlate::Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UWidgetBlueprint::StaticClass())
					.ObjectPath_Lambda([this] { return Source.IsValid() ? Source->GetPathName() : FString(); })
					.OnObjectChanged_Lambda([this](const FAssetData& Asset) { SetBlueprint(Cast<UWidgetBlueprint>(Asset.GetAsset())); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
				[
					SNew(SButton)
					.Text(INVTEXT("Generate Slate"))
					.IsEnabled_Lambda([this] { return Source.IsValid(); })
					.OnClicked(this, &SHazardUMGToSlate::Generate)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.Text(INVTEXT("Copy C++"))
					.IsEnabled_Lambda([this] { return !Code.IsEmpty(); })
					.OnClicked(this, &SHazardUMGToSlate::CopyCode)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 6)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this] { return bOnlyNonDefaultValues ? ECheckBoxState::Unchecked : ECheckBoxState::Checked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State) {
					bOnlyNonDefaultValues = State != ECheckBoxState::Checked;
					Generate();
				})
				.ToolTipText(INVTEXT("Include values that match Slate defaults. Disabled by default to keep the generated code compact."))
				[
					SNew(STextBlock).Text(INVTEXT("Write all values (including defaults)"))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 6)
			[
				SNew(STextBlock)
				.Text(INVTEXT("Designer layout and appearance only. Asset references are comments; Blueprint logic and animations are not exported."))
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot().FillHeight(1).Padding(8, 0)
			[
				SAssignNew(CodeView, SMultiLineEditableTextBox)
				.IsReadOnly(true)
				.AutoWrapText(false)
				.Font(FCoreStyle::GetDefaultFontStyle("Mono", 10))
				.Marshaller(MakeShared<FSlateCodeMarshaller>())
			]
			+ SVerticalBox::Slot().AutoHeight().MaxHeight(120).Padding(8, 4)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(STextBlock)
					.Text_Lambda([this] { return WarningText; })
					.AutoWrapText(true)
					.ColorAndOpacity(FLinearColor(1.0f, 0.7f, 0.3f))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(8, 4)
			[
				SNew(STextBlock).Text_Lambda([this] { return Status; })
			]
		];
		SetBlueprint(InArgs._Blueprint);
	}

	void SHazardUMGToSlate::SetBlueprint(UWidgetBlueprint* Blueprint)
	{
		Source = Blueprint;
		Generate();
	}

	FReply SHazardUMGToSlate::Generate()
	{
		FUMGToSlateOptions Options;
		Options.bOnlyNonDefaultValues = bOnlyNonDefaultValues;
		const FUMGToSlateResult Result = FUMGToSlateGenerator::Generate(Source.Get(), Options);
		Code = Result.Code;
		CodeView->SetText(FText::FromString(Code));
		WarningText = FText::FromString(FString::Join(Result.Warnings, TEXT("\n")));
		Status = FText::Format(INVTEXT("{0} widgets exported. {1} conversion notes. Regenerate after changing the UMG design."), Result.WidgetCount, Result.Warnings.Num());
		return FReply::Handled();
	}

	FReply SHazardUMGToSlate::CopyCode() const
	{
		FPlatformApplicationMisc::ClipboardCopy(*Code);
		return FReply::Handled();
	}

	void SHazardUMGToSlate::Open(UWidgetBlueprint* Blueprint)
	{
		if (const TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->TryInvokeTab(TabName))
		{
			StaticCastSharedRef<SHazardUMGToSlate>(Tab->GetContent())->SetBlueprint(Blueprint);
		}
	}

	void SHazardUMGToSlate::RegisterMenus()
	{
		FToolMenuOwnerScoped Owner(TabName);
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("AssetEditor.WidgetBlueprintEditor.ToolBar");
		Menu->AddDynamicSection("HazardUMGToSlate", FNewToolMenuDelegate::CreateLambda([](UToolMenu* InMenu) {
			const UWidgetBlueprintToolMenuContext* Context = InMenu->FindContext<UWidgetBlueprintToolMenuContext>();
			const TSharedPtr<FWidgetBlueprintEditor> Editor = Context ? Context->WidgetBlueprintEditor.Pin() : nullptr;
			if (!Editor) { return; }
			const TWeakObjectPtr<UWidgetBlueprint> Blueprint = Editor->GetWidgetBlueprintObj();
			InMenu->FindOrAddSection("WidgetTools").AddEntry(FToolMenuEntry::InitToolBarButton(
				"HazardExportSlate", FUIAction(FExecuteAction::CreateLambda([Blueprint] { if (Blueprint.IsValid()) { Open(Blueprint.Get()); } })),
				INVTEXT("Export Slate"), INVTEXT("Generate pure Slate C++ from this Widget Blueprint's designer hierarchy."),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy")));
		}));
	}
}