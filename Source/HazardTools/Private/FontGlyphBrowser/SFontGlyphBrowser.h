// Copyright Neyl Sullivan 2026

#pragma once

#include "CoreMinimal.h"
#include "Fonts/CompositeFont.h"
#include "Misc/TextFilter.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STileView.h"

class SSearchBox;
template <typename OptionType> class SComboBox;

namespace HazardTools
{
	struct FFontGlyphItem
	{
		uint32 Codepoint = 0;
		FString Glyph;
		FString Hex;
		FString FontName;
		FString Names;
	};

	struct FFontBrowserFace
	{
		FString Label;
		FFontData Font;
	};

	class SFontGlyphBrowser : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFontGlyphBrowser) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

	private:
		void SelectFont(UObject* Asset);
		void SelectFace(TSharedPtr<FFontBrowserFace> Face, ESelectInfo::Type SelectInfo);
		void LoadNames();
		void RebuildGlyphs();
		void ApplyNames();
		void ApplyFilter();
		void SaveSelection() const;
		TSharedRef<ITableRow> GenerateTile(TSharedPtr<FFontGlyphItem> Item, const TSharedRef<STableViewBase>& Owner);
		TSharedRef<SWidget> MakePreview();
		FSlateFontInfo GetPreviewFont(float Size) const;
		FReply CopySelection(const FString& Format);
		FText GetStatusText() const;

		TStrongObjectPtr<UObject> SelectedFont;
		TSharedPtr<FStandaloneCompositeFont> PreviewFont;
		TArray<TSharedPtr<FFontBrowserFace>> Faces;
		TSharedPtr<FFontBrowserFace> SelectedFace;
		TSharedPtr<SComboBox<TSharedPtr<FFontBrowserFace>>> FacePicker;
		TMap<uint32, FString> MaterialNames;
		TArray<TSharedPtr<FFontGlyphItem>> Glyphs;
		TArray<TSharedPtr<FFontGlyphItem>> FilteredGlyphs;
		TSharedPtr<FFontGlyphItem> SelectedGlyph;
		TSharedPtr<STileView<TSharedPtr<FFontGlyphItem>>> TileView;
		TSharedPtr<SSearchBox> SearchBox;
		TSharedPtr<TTextFilter<const FFontGlyphItem&>> SearchFilter;
		FText LoadError;
		FText CopyStatus;
		float PreviewSize = 96.0f;
		bool bUseMaterialNames = false;
	};
}