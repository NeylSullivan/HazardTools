// Copyright Neyl Sullivan 2026

#include "FontGlyphBrowser/SFontGlyphBrowser.h"

#include "AssetRegistry/AssetData.h"
#include "Containers/StringConv.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Text/STextBlock.h"

#if WITH_FREETYPE
THIRD_PARTY_INCLUDES_START
#include "ft2build.h"
#include FT_FREETYPE_H
THIRD_PARTY_INCLUDES_END
#endif

namespace HazardTools
{
	namespace
	{
		constexpr const TCHAR* SettingsSection = TEXT("HazardTools.FontGlyphBrowser");
	}

	void SFontGlyphBrowser::Construct(const FArguments& InArgs)
	{
		LoadNames();
		SearchFilter = MakeShared<TTextFilter<const FFontGlyphItem&>>(
			TTextFilter<const FFontGlyphItem&>::FItemToStringArray::CreateLambda([](const FFontGlyphItem& Item, TArray<FString>& Strings) {
				Strings = { Item.Names, Item.Names.Replace(TEXT("_"), TEXT(" ")).Replace(TEXT("-"), TEXT(" ")),
				            Item.Hex, TEXT("U+") + Item.Hex, TEXT("0x") + Item.Hex, TEXT("\\u") + Item.Hex,
				            FString::Printf(TEXT("\\U%08X"), Item.Codepoint), FString::FromInt(Item.Codepoint), Item.Glyph };
			}));

		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
				[
					SNew(STextBlock).Text(INVTEXT("Font / Font Face"))
				]
				+ SHorizontalBox::Slot().FillWidth(1)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(UObject::StaticClass())
					.ObjectPath_Lambda([this]() { return SelectedFont.IsValid() ? SelectedFont->GetPathName() : FString(); })
					.OnShouldFilterAsset_Lambda([](const FAssetData& Asset) {
						return Asset.AssetClassPath != UFont::StaticClass()->GetClassPathName()
							&& Asset.AssetClassPath != UFontFace::StaticClass()->GetClassPathName();
					})
					.OnObjectChanged_Lambda([this](const FAssetData& Asset) { SelectFont(Asset.GetAsset()); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0, 0, 0)
				[
					SNew(SButton)
					.Text(INVTEXT("Refresh"))
					.OnClicked_Lambda([this]() {
						SelectFont(SelectedFont.Get());
						return FReply::Handled();
					})
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1)
				[
					SAssignNew(FacePicker, SComboBox<TSharedPtr<FFontBrowserFace>>)
					.OptionsSource(&Faces)
					.OnGenerateWidget_Lambda([](TSharedPtr<FFontBrowserFace> Face) {
						return SNew(STextBlock).Text(FText::FromString(Face->Label));
					})
					.OnSelectionChanged(this, &SFontGlyphBrowser::SelectFace)
					.ToolTipText(INVTEXT("Browse the Unicode glyphs of one face, including faces in composite fonts."))
					[
						SNew(STextBlock).Text_Lambda([this]() {
							return SelectedFace ? FText::FromString(SelectedFace->Label) : INVTEXT("Select a font face");
						})
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(12, 0, 0, 0)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return bUseMaterialNames ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State) {
						bUseMaterialNames = State == ECheckBoxState::Checked;
						ApplyNames();
					})
					.ToolTipText(INVTEXT("Use bundled Material Symbols names and aliases. Automatically enabled for Material Symbols faces."))
					[
						SNew(STextBlock).Text(INVTEXT("Material Symbols names"))
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
			[
				SAssignNew(SearchBox, SSearchBox)
				.HintText(INVTEXT("Search name, Unicode or glyph: arrow back, E5C4, U+E5C4, 0xE5C4, \\uE5C4"))
				.ToolTipText(INVTEXT("Case-insensitive partial matches; spaces mean AND. Also supports OR, NOT and quoted phrases."))
				.OnTextChanged_Lambda([this](const FText& Text) {
					SearchFilter->SetRawFilterText(Text);
					SearchBox->SetError(SearchFilter->GetFilterErrorText());
					ApplyFilter();
				})
			]
			+ SVerticalBox::Slot().FillHeight(1).Padding(8, 0)
			[
				SNew(SSplitter)
				+ SSplitter::Slot().Value(0.75f)
				[
					SAssignNew(TileView, STileView<TSharedPtr<FFontGlyphItem>>)
					.ListItemsSource(&FilteredGlyphs)
					.ItemWidth(132)
					.ItemHeight(116)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateTile(this, &SFontGlyphBrowser::GenerateTile)
					.OnSelectionChanged_Lambda([this](TSharedPtr<FFontGlyphItem> Item, ESelectInfo::Type) {
						SelectedGlyph = Item;
						CopyStatus = FText::GetEmpty();
					})
					.OnMouseButtonDoubleClick_Lambda([this](TSharedPtr<FFontGlyphItem> Item) {
						SelectedGlyph = Item;
						CopySelection(TEXT("U+"));
					})
				]
				+ SSplitter::Slot().Value(0.25f)
				[
					MakePreview()
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(8)
			[
				SNew(STextBlock).AutoWrapText(true).Text(this, &SFontGlyphBrowser::GetStatusText)
			]
		];

		FString FontPath;
		GConfig->GetString(SettingsSection, TEXT("LastFont"), FontPath, GEditorPerProjectIni);
		SelectFont(FSoftObjectPath(FontPath).TryLoad());
	}

	void SFontGlyphBrowser::SelectFont(UObject* Asset)
	{
		if (Asset && !Asset->IsA<UFont>() && !Asset->IsA<UFontFace>())
		{
			return;
		}

		// A strong reference keeps the selected asset and its owned faces alive while the window is open.
		SelectedFont.Reset(Asset);
		Faces.Reset();
		SelectedFace.Reset();
		LoadError = FText::GetEmpty();
		if (const UFont* Font = Cast<UFont>(Asset))
		{
			if (Font->FontCacheType != EFontCacheType::Runtime)
			{
				LoadError = INVTEXT("Offline bitmap fonts are not supported. Select a runtime Font or Font Face asset.");
			}
			else if (const FCompositeFont* Composite = Font->GetCompositeFont())
			{
				const auto AddFaces = [this](const FTypeface& Typeface, const FString& Group) {
					for (const FTypefaceEntry& Entry : Typeface.Fonts)
					{
						TSharedPtr<FFontBrowserFace> Face = MakeShared<FFontBrowserFace>();
						Face->Label = Group + TEXT(" / ") + Entry.Name.ToString();
						Face->Font = Entry.Font;
						Faces.Add(Face);
					}
				};
				AddFaces(Composite->DefaultTypeface, TEXT("Default"));
				for (int32 Index = 0; Index < Composite->SubTypefaces.Num(); ++Index)
				{
					const FCompositeSubFont& SubFont = Composite->SubTypefaces[Index];
					AddFaces(SubFont.Typeface, FString::Printf(TEXT("Sub-font %d (%s)"), Index + 1, *SubFont.EditorName.ToString()));
				}
				AddFaces(Composite->FallbackTypeface.Typeface, TEXT("Fallback"));
			}
		}
		else if (const UFontFace* FontFace = Cast<UFontFace>(Asset))
		{
			TSharedPtr<FFontBrowserFace> Face = MakeShared<FFontBrowserFace>();
			Face->Label = FontFace->GetName();
			Face->Font = FFontData(FontFace);
			Faces.Add(Face);
		}

		FString LastFace;
		GConfig->GetString(SettingsSection, TEXT("LastFace"), LastFace, GEditorPerProjectIni);
		TSharedPtr<FFontBrowserFace> FaceToSelect = Faces.IsEmpty() ? nullptr : Faces[0];
		for (const TSharedPtr<FFontBrowserFace>& Face : Faces)
		{
			if (Face->Label == LastFace)
			{
				FaceToSelect = Face;
				break;
			}
		}
		FacePicker->RefreshOptions();
		FacePicker->SetSelectedItem(FaceToSelect);
		if (!FaceToSelect)
		{
			SelectFace(nullptr, ESelectInfo::Direct);
		}
	}

	void SFontGlyphBrowser::SelectFace(TSharedPtr<FFontBrowserFace> Face, ESelectInfo::Type SelectInfo)
	{
		SelectedFace = Face;
		SaveSelection();
		RebuildGlyphs();
	}

	void SFontGlyphBrowser::SaveSelection() const
	{
		GConfig->SetString(SettingsSection, TEXT("LastFont"), SelectedFont.IsValid() ? *SelectedFont->GetPathName() : TEXT(""), GEditorPerProjectIni);
		GConfig->SetString(SettingsSection, TEXT("LastFace"), SelectedFace ? *SelectedFace->Label : TEXT(""), GEditorPerProjectIni);
		GConfig->Flush(false, GEditorPerProjectIni);
	}

	void SFontGlyphBrowser::LoadNames()
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("HazardTools"));
		TArray<FString> Lines;
		if (!Plugin || !FFileHelper::LoadFileToStringArray(Lines, *FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources/FontGlyphBrowser/MaterialSymbolsOutlined.codepoints"))))
		{
			return;
		}
		for (const FString& Line : Lines)
		{
			TArray<FString> Parts;
			Line.ParseIntoArrayWS(Parts);
			if (Parts.Num() != 2 || Parts[1].IsEmpty() || Parts[1].Len() > 6)
			{
				continue;
			}
			bool bValidHex = true;
			for (const TCHAR Digit : Parts[1])
			{
				bValidHex &= FChar::IsHexDigit(Digit);
			}
			if (bValidHex)
			{
				FString& Names = MaterialNames.FindOrAdd(FParse::HexNumber(Parts[1]));
				if (!Names.IsEmpty())
				{
					Names += TEXT(", ");
				}
				Names += Parts[0];
			}
		}
	}

	void SFontGlyphBrowser::RebuildGlyphs()
	{
		Glyphs.Reset();
		SelectedGlyph.Reset();
		PreviewFont.Reset();
		CopyStatus = FText::GetEmpty();
		bUseMaterialNames = false;
		ON_SCOPE_EXIT { ApplyNames(); };
		if (!SelectedFace)
		{
			return;
		}
		LoadError = FText::GetEmpty();
		PreviewFont = MakeShared<FStandaloneCompositeFont>();
		FTypefaceEntry Entry(FName(TEXT("Preview")));
		Entry.Font = SelectedFace->Font;
		PreviewFont->DefaultTypeface.Fonts.Add(Entry);

#if WITH_FREETYPE
		const FFontFaceDataConstPtr FaceData = SelectedFace->Font.GetFontFaceData();
		TArray<uint8> FileData;
		if ((!FaceData || !FaceData->HasData()) && !FFileHelper::LoadFileToArray(FileData, *SelectedFace->Font.GetFontFilename()))
		{
			LoadError = INVTEXT("Unable to read the font face data. Reimport the asset and refresh.");
			return;
		}
		const TArray<uint8>& Bytes = FaceData && FaceData->HasData() ? FaceData->GetData() : FileData;
		FT_Library Library = nullptr;
		if (FT_Init_FreeType(&Library) != 0)
		{
			LoadError = INVTEXT("Unable to initialize FreeType.");
			return;
		}
		ON_SCOPE_EXIT { FT_Done_FreeType(Library); };
		FT_Face Face = nullptr;
		if (FT_New_Memory_Face(Library, Bytes.GetData(), Bytes.Num(), SelectedFace->Font.GetSubFaceIndex(), &Face) != 0)
		{
			LoadError = INVTEXT("Unable to open this font face.");
			return;
		}
		ON_SCOPE_EXIT { FT_Done_Face(Face); };
		if (FT_Select_Charmap(Face, FT_ENCODING_UNICODE) != 0)
		{
			LoadError = INVTEXT("This face has no Unicode character map.");
			return;
		}

		const FString Family = Face->family_name ? UTF8_TO_TCHAR(Face->family_name) : FString();
		bUseMaterialNames = Family.Contains(TEXT("Material Symbols")) || Family.Contains(TEXT("MaterialSymbols"))
			|| SelectedFont->GetName().Contains(TEXT("MaterialSymbols"));
		FT_UInt GlyphIndex = 0;
		for (FT_ULong Codepoint = FT_Get_First_Char(Face, &GlyphIndex); GlyphIndex != 0; Codepoint = FT_Get_Next_Char(Face, Codepoint, &GlyphIndex))
		{
			if (Codepoint == 0 || Codepoint > 0x10FFFF || (Codepoint >= 0xD800 && Codepoint <= 0xDFFF))
			{
				continue;
			}
			TSharedPtr<FFontGlyphItem> Item = MakeShared<FFontGlyphItem>();
			Item->Codepoint = static_cast<uint32>(Codepoint);
			Item->Hex = FString::Printf(TEXT("%04X"), Item->Codepoint);
			const UTF32CHAR Character = static_cast<UTF32CHAR>(Codepoint);
			const FUTF32ToTCHAR Converted(&Character, 1);
			Item->Glyph = FString(Converted.Length(), Converted.Get());
			ANSICHAR GlyphName[256] = {};
			if (FT_HAS_GLYPH_NAMES(Face) && FT_Get_Glyph_Name(Face, GlyphIndex, GlyphName, sizeof(GlyphName)) == 0)
			{
				Item->FontName = UTF8_TO_TCHAR(GlyphName);
			}
			Glyphs.Add(Item);
		}
#else
		LoadError = INVTEXT("This engine build does not include FreeType support.");
#endif
	}

	void SFontGlyphBrowser::ApplyNames()
	{
		for (const TSharedPtr<FFontGlyphItem>& Item : Glyphs)
		{
			const FString* Names = bUseMaterialNames ? MaterialNames.Find(Item->Codepoint) : nullptr;
			Item->Names = Names ? *Names : Item->FontName;
		}
		ApplyFilter();
		TileView->RebuildList();
	}

	void SFontGlyphBrowser::ApplyFilter()
	{
		FilteredGlyphs.Reset();
		for (const TSharedPtr<FFontGlyphItem>& Item : Glyphs)
		{
			if (SearchFilter->PassesFilter(*Item))
			{
				FilteredGlyphs.Add(Item);
			}
		}
		if (SelectedGlyph && !FilteredGlyphs.Contains(SelectedGlyph))
		{
			TileView->ClearSelection();
			SelectedGlyph.Reset();
		}
		TileView->RequestListRefresh();
	}

	FSlateFontInfo SFontGlyphBrowser::GetPreviewFont(float Size) const
	{
		FSlateFontInfo Font(PreviewFont, Size);
		Font.FontFallback = EFontFallback::FF_NoFallback;
		return Font;
	}

	TSharedRef<ITableRow> SFontGlyphBrowser::GenerateTile(TSharedPtr<FFontGlyphItem> Item, const TSharedRef<STableViewBase>& Owner)
	{
		return SNew(STableRow<TSharedPtr<FFontGlyphItem>>, Owner)
			.Padding(4)
			.ToolTipText(FText::FromString(TEXT("U+") + Item->Hex + TEXT("\n") + Item->Names + TEXT("\nDouble-click to copy Unicode")))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text(FText::AsCultureInvariant(Item->Glyph)).Font(GetPreviewFont(32))
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("U+") + Item->Hex))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 3)
				[
					SNew(STextBlock).Text(FText::FromString(Item->Names)).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
				]
			];
	}

	TSharedRef<SWidget> SFontGlyphBrowser::MakePreview()
	{
		TSharedRef<SVerticalBox> Panel = SNew(SVerticalBox);
		Panel->AddSlot().AutoHeight().Padding(8)
		[
			SNew(STextBlock).Text(INVTEXT("Glyph preview"))
		];
		Panel->AddSlot().AutoHeight().Padding(8)
		[
			SNew(SBox).HeightOverride(200).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return SelectedGlyph ? FText::AsCultureInvariant(SelectedGlyph->Glyph) : FText::GetEmpty(); })
				.Font_Lambda([this]() { return GetPreviewFont(PreviewSize); })
			]
		];
		Panel->AddSlot().AutoHeight().Padding(8)
		[
			SNew(SSpinBox<float>)
			.MinValue(8)
			.MaxValue(160)
			.Value_Lambda([this]() { return PreviewSize; })
			.OnValueChanged_Lambda([this](float Value) { PreviewSize = Value; })
			.ToolTipText(INVTEXT("Preview font size"))
		];
		Panel->AddSlot().AutoHeight().Padding(8)
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text_Lambda([this]() {
				return SelectedGlyph
					? FText::FromString(TEXT("U+") + SelectedGlyph->Hex + TEXT("\n") + SelectedGlyph->Names)
					: INVTEXT("Select a glyph. Double-click a tile to copy its Unicode code.");
			})
		];
		for (const FString& Format : { FString(TEXT("U+")), FString(TEXT("0x")), FString(TEXT("\\u")), FString(TEXT("Glyph")) })
		{
			Panel->AddSlot().AutoHeight().Padding(8, 3)
			[
				SNew(SButton)
				.IsEnabled_Lambda([this]() { return SelectedGlyph.IsValid(); })
				.Text(FText::FromString(Format == TEXT("Glyph") ? TEXT("Copy glyph") : TEXT("Copy ") + Format + TEXT(" code")))
				.OnClicked_Lambda([this, Format]() { return CopySelection(Format); })
			];
		}
		Panel->AddSlot().AutoHeight().Padding(8)
		[
			SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]() { return CopyStatus; })
		];
		return SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				Panel
			];
	}

	FReply SFontGlyphBrowser::CopySelection(const FString& Format)
	{
		if (SelectedGlyph)
		{
			FString Value = Format + SelectedGlyph->Hex;
			if (Format == TEXT("Glyph"))
			{
				Value = SelectedGlyph->Glyph;
			}
			else if (Format == TEXT("\\u") && SelectedGlyph->Codepoint > 0xFFFF)
			{
				Value = FString::Printf(TEXT("\\U%08X"), SelectedGlyph->Codepoint);
			}
			FPlatformApplicationMisc::ClipboardCopy(*Value);
			CopyStatus = FText::FromString(Format == TEXT("Glyph") ? TEXT("Glyph copied") : TEXT("Copied ") + Value);
		}
		return FReply::Handled();
	}

	FText SFontGlyphBrowser::GetStatusText() const
	{
		if (!LoadError.IsEmpty())
		{
			return LoadError;
		}
		if (!SelectedFont.IsValid())
		{
			return INVTEXT("Select a Font or Font Face asset to browse its Unicode glyphs.");
		}
		FString Status = FString::Printf(TEXT("%d / %d Unicode glyphs"), FilteredGlyphs.Num(), Glyphs.Num());
		if (FilteredGlyphs.IsEmpty())
		{
			Status += Glyphs.IsEmpty() ? TEXT(" - no Unicode glyphs available in this face.") : TEXT(" - no matches. Try another name or code.");
		}
		if (bUseMaterialNames && MaterialNames.IsEmpty())
		{
			Status += TEXT(" - Material Symbols name file is missing or empty.");
		}
		return FText::FromString(Status);
	}
}