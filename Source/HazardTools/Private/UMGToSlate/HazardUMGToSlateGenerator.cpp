// Copyright Neyl Sullivan 2026

#include "UMGToSlate/HazardUMGToSlateGenerator.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Slate/WidgetTransform.h"
#include "Styling/SlateTypes.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"
#include "WidgetBlueprint.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SSafeZone.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SInvalidationPanel.h"

#include <charconv>

namespace HazardTools
{
	namespace
	{
		FString Number(double Value)
		{
			const float Float = static_cast<float>(Value);
			if (!FMath::IsFinite(Float) || Float == 0.0f) { return TEXT("0.0f"); }
			// Shortest decimal that round-trips to the same float, including small/large values.
			char Buffer[64];
			const auto Converted = std::to_chars(Buffer, Buffer + UE_ARRAY_COUNT(Buffer) - 1, Float);
			*Converted.ptr = '\0';
			FString Text(ANSI_TO_TCHAR(Buffer));
			if (!Text.Contains(TEXT(".")) && !Text.Contains(TEXT("e"))) { Text += TEXT(".0"); }
			return Text + TEXT("f");
		}

		FString Quote(const FString& Value)
		{
			FString Result = TEXT("TEXT(\"");
			for (TCHAR C : Value)
			{
				switch (C)
				{
					case '\\':
						Result += TEXT("\\\\");
						break;
					case '"':
						Result += TEXT("\\\"");
						break;
					case '\n':
						Result += TEXT("\\n");
						break;
					case '\r':
						Result += TEXT("\\r");
						break;
					case '\t':
						Result += TEXT("\\t");
						break;
					default:
						if (C < 32)
						{
							Result += TEXT("\\0");
							Result += TCHAR('0' + C / 8);
							Result += TCHAR('0' + C % 8);
						}
						else { Result += C; }
				}
			}
			return Result + TEXT("\")");
		}

		FString Comment(FString Value)
		{
			Value.ReplaceInline(TEXT("\r"), TEXT(" "));
			Value.ReplaceInline(TEXT("\n"), TEXT(" "));
			// A trailing backslash must never splice the following C++ line into a comment.
			Value.ReplaceInline(TEXT("\\"), TEXT("/"));
			return Value;
		}

		FString Vector(FVector2D V) { return FString::Printf(TEXT("FVector2D(%s, %s)"), *Number(V.X), *Number(V.Y)); }
		FString Margin(FMargin V) { return FString::Printf(TEXT("FMargin(%s, %s, %s, %s)"), *Number(V.Left), *Number(V.Top), *Number(V.Right), *Number(V.Bottom)); }
		FString Color(FLinearColor V) { return FString::Printf(TEXT("FLinearColor(%s, %s, %s, %s)"), *Number(V.R), *Number(V.G), *Number(V.B), *Number(V.A)); }

		struct FWidgetType
		{
			const TCHAR* UMG;
			const TCHAR* Slate;
			const TCHAR* Header;
			bool bPanel = false;
		};

		const FWidgetType WidgetTypes[] = {
			{ TEXT("CanvasPanel"), TEXT("SConstraintCanvas"), TEXT("Widgets/Layout/SConstraintCanvas.h"), true },
			{ TEXT("HorizontalBox"), TEXT("SHorizontalBox"), TEXT("Widgets/SBoxPanel.h"), true },
			{ TEXT("VerticalBox"), TEXT("SVerticalBox"), TEXT("Widgets/SBoxPanel.h"), true },
			{ TEXT("Overlay"), TEXT("SOverlay"), TEXT("Widgets/SOverlay.h"), true },
			{ TEXT("GridPanel"), TEXT("SGridPanel"), TEXT("Widgets/Layout/SGridPanel.h"), true },
			{ TEXT("UniformGridPanel"), TEXT("SUniformGridPanel"), TEXT("Widgets/Layout/SUniformGridPanel.h"), true },
			{ TEXT("WrapBox"), TEXT("SWrapBox"), TEXT("Widgets/Layout/SWrapBox.h"), true },
			{ TEXT("ScrollBox"), TEXT("SScrollBox"), TEXT("Widgets/Layout/SScrollBox.h"), true },
			{ TEXT("WidgetSwitcher"), TEXT("SWidgetSwitcher"), TEXT("Widgets/Layout/SWidgetSwitcher.h"), true },
			{ TEXT("SizeBox"), TEXT("SBox"), TEXT("Widgets/Layout/SBox.h") },
			{ TEXT("ScaleBox"), TEXT("SScaleBox"), TEXT("Widgets/Layout/SScaleBox.h") },
			{ TEXT("Border"), TEXT("SBorder"), TEXT("Widgets/Layout/SBorder.h") },
			{ TEXT("Button"), TEXT("SButton"), TEXT("Widgets/Input/SButton.h") },
			{ TEXT("TextBlock"), TEXT("STextBlock"), TEXT("Widgets/Text/STextBlock.h") },
			{ TEXT("Image"), TEXT("SImage"), TEXT("Widgets/Images/SImage.h") },
			{ TEXT("Spacer"), TEXT("SSpacer"), TEXT("Widgets/Layout/SSpacer.h") },
			{ TEXT("ProgressBar"), TEXT("SProgressBar"), TEXT("Widgets/Notifications/SProgressBar.h") },
			{ TEXT("CheckBox"), TEXT("SCheckBox"), TEXT("Widgets/Input/SCheckBox.h") },
			{ TEXT("Slider"), TEXT("SSlider"), TEXT("Widgets/Input/SSlider.h") },
			{ TEXT("BackgroundBlur"), TEXT("SBackgroundBlur"), TEXT("Widgets/Layout/SBackgroundBlur.h") },
			{ TEXT("SafeZone"), TEXT("SSafeZone"), TEXT("Widgets/Layout/SSafeZone.h") },
			{ TEXT("NamedSlot"), TEXT("SBox"), TEXT("Widgets/Layout/SBox.h") },
			{ TEXT("InvalidationBox"), TEXT("SInvalidationPanel"), TEXT("Widgets/SInvalidationPanel.h") },
			{ TEXT("RetainerBox"), TEXT("SBox"), TEXT("Widgets/Layout/SBox.h") },
			{ TEXT("EditableText"), TEXT("SEditableText"), TEXT("Widgets/Input/SEditableText.h") },
			{ TEXT("EditableTextBox"), TEXT("SEditableTextBox"), TEXT("Widgets/Input/SEditableTextBox.h") }
		};

		class FGenerator
		{
		public:
			FUMGToSlateOptions Options;
			TMap<FString, FString> AttributeDefaults;
			FUMGToSlateResult Result;
			TSet<FString> Includes;
			FString Members;
			FString Initialization;
			FString PostInitialization;
			TSet<const UWidget*> ActiveWidgets;
			TSet<const UWidgetTree*> ActiveTrees;
			TMap<const UWidgetTree*, TMap<FName, const UWidget*>> SlotOverrides;
			int32 NextResource = 0;

			void Warn(const FString& Message) { Result.Warnings.AddUnique(Comment(Message)); }

			void AssetComments(const FProperty* P, const void* Data, const FString& Label, FString& Out, const FString& I, int32 Depth = 0)
			{
				if (Depth > 16 || P->HasAnyPropertyFlags(CPF_Deprecated | CPF_Transient)) { return; }
				FString Path;
				if (const FSoftObjectProperty* Soft = CastField<FSoftObjectProperty>(P)) { Path = Soft->GetPropertyValue(Data).ToSoftObjectPath().ToString(); }
				else if (const FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(P))
				{
					const UObject* Reference = Object->GetObjectPropertyValue(Data);
					if (Reference && Reference->IsAsset()) { Path = Reference->GetPathName(); }
				}
				else if (const FStructProperty* Struct = CastField<FStructProperty>(P))
				{
					for (TFieldIterator<FProperty> It(Struct->Struct); It; ++It)
					{
						AssetComments(*It, It->ContainerPtrToValuePtr<void>(Data), Label + TEXT(".") + It->GetName(), Out, I, Depth + 1);
					}
				}
				else if (const FArrayProperty* Array = CastField<FArrayProperty>(P))
				{
					FScriptArrayHelper Helper(Array, Data);
					for (int32 Index = 0; Index < Helper.Num(); ++Index) { AssetComments(Array->Inner, Helper.GetRawPtr(Index), Label + FString::Printf(TEXT("[%d]"), Index), Out, I, Depth + 1); }
				}
				if (!Path.IsEmpty()) { Out += I + TEXT("// Asset ") + Comment(Label) + TEXT(": ") + Comment(Path) + TEXT("\n"); }
			}

			void AddSlotOverrides(const UWidgetTree* Tree, const INamedSlotInterface* Source)
			{
				TArray<FName> Names;
				Source->GetSlotNames(Names);
				for (const FName Name : Names)
				{
					if (const UWidget* Content = Source->GetContentForSlot(Name)) { SlotOverrides.FindOrAdd(Tree).Add(Name, Content); }
				}
			}

			FString EnumValue(const UEnum* Enum, int64 Value)
			{
				if (Enum->GetName() == TEXT("ESlateVisibility"))
				{
					return TEXT("EVisibility::") + Enum->GetNameStringByValue(Value);
				}
				return Enum->GetNameByValue(Value).ToString();
			}

			FString StructValue(const UScriptStruct* Struct, const void* Data)
			{
				const FName Name = Struct->GetFName();
				if (Name == TEXT("Vector2D")) { return Vector(*static_cast<const FVector2D*>(Data)); }
				if (Name == TEXT("Vector2f")) { return Vector(FVector2D(*static_cast<const FVector2f*>(Data))); }
				if (Name == TEXT("Margin")) { return Margin(*static_cast<const FMargin*>(Data)); }
				if (Name == TEXT("LinearColor")) { return Color(*static_cast<const FLinearColor*>(Data)); }
				if (Name == TEXT("Vector4"))
				{
					const FVector4& V = *static_cast<const FVector4*>(Data);
					return FString::Printf(TEXT("FVector4(%s, %s, %s, %s)"), *Number(V.X), *Number(V.Y), *Number(V.Z), *Number(V.W));
				}
				if (Name == TEXT("SlateColor"))
				{
					const FSlateColor& V = *static_cast<const FSlateColor*>(Data);
					if (V.IsColorSpecified()) { return TEXT("FSlateColor(") + Color(V.GetSpecifiedColor()) + TEXT(")"); }
					const FProperty* Rule = Struct->FindPropertyByName(TEXT("ColorUseRule"));
					const FString Mode = Rule ? PropertyValue(Rule, Rule->ContainerPtrToValuePtr<void>(Data)) : FString();
					if (Mode.EndsWith(TEXT("UseColor_Foreground_Subdued"))) { return TEXT("FSlateColor::UseSubduedForeground()"); }
					if (Mode.EndsWith(TEXT("UseColor_UseStyle"))) { return TEXT("FSlateColor::UseStyle()"); }
					return TEXT("FSlateColor::UseForeground()");
				}
				return FString();
			}

			FString PropertyValue(const FProperty* Property, const void* Data)
			{
				if (const FEnumProperty* P = CastField<FEnumProperty>(Property)) { return EnumValue(P->GetEnum(), P->GetUnderlyingProperty()->GetSignedIntPropertyValue(Data)); }
				if (const FByteProperty* P = CastField<FByteProperty>(Property); P && P->Enum) { return EnumValue(P->Enum, P->GetPropertyValue(Data)); }
				if (const FBoolProperty* P = CastField<FBoolProperty>(Property)) { return P->GetPropertyValue(Data) ? TEXT("true") : TEXT("false"); }
				if (const FNumericProperty* P = CastField<FNumericProperty>(Property))
				{
					return P->IsFloatingPoint() ? Number(P->GetFloatingPointPropertyValue(Data)) : P->GetNumericPropertyValueToString(Data);
				}
				if (const FTextProperty* P = CastField<FTextProperty>(Property)) { return TEXT("FText::FromString(") + Quote(P->GetPropertyValue(Data).ToString()) + TEXT(")"); }
				if (const FStrProperty* P = CastField<FStrProperty>(Property)) { return Quote(P->GetPropertyValue(Data)); }
				if (const FNameProperty* P = CastField<FNameProperty>(Property)) { return TEXT("FName(") + Quote(P->GetPropertyValue(Data).ToString()) + TEXT(")"); }
				if (const FStructProperty* P = CastField<FStructProperty>(Property)) { return StructValue(P->Struct, Data); }
				return FString();
			}

			FString Value(const UObject* Object, const TCHAR* Name)
			{
				const FProperty* P = Object ? Object->GetClass()->FindPropertyByName(Name) : nullptr;
				return P ? PropertyValue(P, P->ContainerPtrToValuePtr<void>(Object)) : FString();
			}

			bool Flag(const UObject* Object, const TCHAR* Name) { return Value(Object, Name) == TEXT("true"); }

			// Only reflected Slate style fields are serialized. UObject resources never become executable code.
			void WriteStruct(const UScriptStruct* Struct, const void* Data, const FString& Target, const void* Defaults)
			{
				const bool bBrush = Struct == FSlateBrush::StaticStruct();
				if (bBrush)
				{
					const FSlateBrush& Brush = *static_cast<const FSlateBrush*>(Data);
					const FString Path = Brush.GetResourceObject() ? Brush.GetResourceObject()->GetPathName() : Brush.GetResourceName().ToString();
					if (!Path.IsEmpty() && Path != TEXT("None"))
					{
						Initialization += TEXT("\t\t// TODO resource for ") + Target + TEXT(": ") + Comment(Path) + TEXT("\n");
						Warn(TEXT("Asset resources are omitted; assign brushes/fonts manually to reproduce their appearance."));
					}
					const FBox2f UV = Brush.GetUVRegion();
					if (UV.bIsValid)
					{
						Initialization += TEXT("\t\t") + Target + TEXT(".SetUVRegion(FBox2D(") + Vector(FVector2D(UV.Min)) + TEXT(", ") + Vector(FVector2D(UV.Max)) + TEXT("));\n");
					}
				}
				for (TFieldIterator<FProperty> It(Struct); It; ++It)
				{
					const FProperty* P = *It;
					const FString Name = P->GetName();
					if (P->HasAnyPropertyFlags(CPF_Deprecated | CPF_Transient) || Name.EndsWith(TEXT("_DEPRECATED"))) { continue; }
					const void* Field = P->ContainerPtrToValuePtr<void>(Data);
					const void* DefaultField = P->ContainerPtrToValuePtr<void>(Defaults);
					const FString FieldTarget = Target + TEXT(".") + Name;
					if (bBrush && (Name == TEXT("ResourceObject") || Name == TEXT("ResourceName") || Name == TEXT("UVRegion") || Name.StartsWith(TEXT("b")))) { continue; }
					if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(P))
					{
						if (const UObject* Resource = ObjectProperty->GetObjectPropertyValue(Field))
						{
							Initialization += TEXT("\t\t// TODO resource for ") + FieldTarget + TEXT(": ") + Comment(Resource->GetPathName()) + TEXT("\n");
							Warn(TEXT("Asset resources are omitted; assign brushes/fonts manually to reproduce their appearance."));
						}
						continue;
					}
					if (Struct == FSlateFontInfo::StaticStruct() && Name == TEXT("TypefaceFontName"))
					{
						Initialization += TEXT("\t\t// Original typeface: ") + Comment(PropertyValue(P, Field)) + TEXT("; fallback uses Regular.\n");
						continue;
					}
					const FString Literal = PropertyValue(P, Field);
					if (!Literal.IsEmpty())
					{
						if (!Options.bOnlyNonDefaultValues || !P->Identical(Field, DefaultField)) { Initialization += TEXT("\t\t") + FieldTarget + TEXT(" = ") + Literal + TEXT(";\n"); }
					}
					else if (const FStructProperty* SP = CastField<FStructProperty>(P))
					{
						if (SP->Struct == FSlateSound::StaticStruct())
						{
							if (const UObject* Resource = static_cast<const FSlateSound*>(Field)->GetResourceObject())
							{
								Initialization += TEXT("\t\t// Omitted sound ") + FieldTarget + TEXT(": ") + Comment(Resource->GetPathName()) + TEXT("\n");
							}
							continue;
						}
						FSlateFontInfo FallbackFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
						if (SP->Struct == FSlateFontInfo::StaticStruct())
						{
							Initialization += TEXT("\t\t") + FieldTarget + TEXT(" = FCoreStyle::GetDefaultFontStyle(\"Regular\", 12);\n");
							DefaultField = &FallbackFont;
						}
						WriteStruct(SP->Struct, Field, FieldTarget, DefaultField);
					}
					else { Warn(TEXT("Style property requires manual conversion: ") + FieldTarget); }
				}
			}

			FString Resource(const UObject* Object, const TCHAR* PropertyName, bool bPointer)
			{
				const FStructProperty* P = FindFProperty<FStructProperty>(Object->GetClass(), PropertyName);
				if (!P) { return FString(); }
				const FString Name = FString::Printf(TEXT("Style%d"), ++NextResource);
				const FString Type = P->Struct->GetStructCPPName();
				Members += TEXT("\t") + Type + TEXT(" ") + Name + TEXT(";\n");
				Initialization += TEXT("\t\t// ") + Comment(Object->GetName()) + TEXT(".") + PropertyName + TEXT("\n");
				if (P->Struct == FSlateFontInfo::StaticStruct()) { Initialization += TEXT("\t\t") + Name + TEXT(" = FCoreStyle::GetDefaultFontStyle(\"Regular\", 12);\n"); }
				FStructOnScope Defaults(P->Struct);
				if (P->Struct == FSlateFontInfo::StaticStruct()) { *reinterpret_cast<FSlateFontInfo*>(Defaults.GetStructMemory()) = FCoreStyle::GetDefaultFontStyle("Regular", 12); }
				WriteStruct(P->Struct, P->ContainerPtrToValuePtr<void>(Object), Name, Defaults.GetStructMemory());
				return (bPointer ? TEXT("&") : TEXT("")) + Name;
			}

			template <typename T> FString DefaultLiteral(const T& V)
			{
				if constexpr (std::is_enum_v<T>) { return EnumValue(StaticEnum<T>(), static_cast<int64>(V)); }
				else if constexpr (std::is_same_v<T, bool>) { return V ? TEXT("true") : TEXT("false"); }
				else if constexpr (std::is_floating_point_v<T>) { return Number(V); }
				else if constexpr (std::is_integral_v<T>) { return FString::Printf(TEXT("%lld"), static_cast<int64>(V)); }
				else { return FString(); }
			}

			template <typename T> FString DefaultLiteral(const TAttribute<T>& V) { return V.IsSet() && !V.IsBound() ? DefaultLiteral(V.Get()) : FString(); }
			template <typename T> FString DefaultLiteral(const TOptional<T>& V) { return V.IsSet() ? DefaultLiteral(V.GetValue()) : FString(); }
			FString DefaultLiteral(const FVector2D& V) { return Vector(V); }
			FString DefaultLiteral(const FVector4& V) { return FString::Printf(TEXT("FVector4(%s, %s, %s, %s)"), *Number(V.X), *Number(V.Y), *Number(V.Z), *Number(V.W)); }
			FString DefaultLiteral(const FMargin& V) { return Margin(V); }
			FString DefaultLiteral(const FLinearColor& V) { return Color(V); }
			FString DefaultLiteral(const FSlateColor& V) { return StructValue(FSlateColor::StaticStruct(), &V); }
			FString DefaultLiteral(const FText& V) { return TEXT("FText::FromString(") + Quote(V.ToString()) + TEXT(")"); }

			FString DefaultLiteral(const EVisibility& V)
			{
				if (V == EVisibility::Visible) { return TEXT("EVisibility::Visible"); }
				if (V == EVisibility::Collapsed) { return TEXT("EVisibility::Collapsed"); }
				if (V == EVisibility::Hidden) { return TEXT("EVisibility::Hidden"); }
				return V == EVisibility::HitTestInvisible ? TEXT("EVisibility::HitTestInvisible") : TEXT("EVisibility::SelfHitTestInvisible");
			}

			template <typename T> void Default(const TCHAR* Name, const T& V)
			{
				const FString Literal = DefaultLiteral(V);
				if (!Literal.IsEmpty()) { AttributeDefaults.Add(Name, Literal); }
			}

			template <typename T> void CommonDefaults(const T& Args)
			{
				Default(TEXT("Visibility"), Args._Visibility);
				Default(TEXT("IsEnabled"), Args._IsEnabled);
				Default(TEXT("RenderOpacity"), Args._RenderOpacity);
				Default(TEXT("Clipping"), Args._Clipping);
				Default(TEXT("PixelSnappingMethod"), Args._PixelSnappingMethod);
				Default(TEXT("FlowDirectionPreference"), Args._FlowDirectionPreference);
				Default(TEXT("RenderTransformPivot"), Args._RenderTransformPivot);
			}

			// Read the actual Slate constructors, not UMG CDOs: their defaults often differ.
			void SetDefaults(const FString& Kind)
			{
				AttributeDefaults.Reset();
				if (!Options.bOnlyNonDefaultValues) { return; }
#define SLATE_DEFAULT(Name) Default(TEXT(#Name), Args._##Name)
#define SLATE_DEFAULT_BEGIN(Name, Type) if (Kind == TEXT(Name)) { const Type::FArguments Args; CommonDefaults(Args);
#define SLATE_DEFAULT_END return; }
				SLATE_DEFAULT_BEGIN("TextBlock", STextBlock)
					SLATE_DEFAULT(WrapTextAt);
					SLATE_DEFAULT(AutoWrapText);
					SLATE_DEFAULT(WrappingPolicy);
					SLATE_DEFAULT(LineHeightPercentage);
					SLATE_DEFAULT(ApplyLineHeightToBottomLine);
					SLATE_DEFAULT(Justification);
					SLATE_DEFAULT(MinDesiredWidth);
					Default(TEXT("Text"), FText::GetEmpty());
					Default(TEXT("ColorAndOpacity"), Args._TextStyle->ColorAndOpacity);
					Default(TEXT("ShadowOffset"), Args._TextStyle->ShadowOffset);
					Default(TEXT("ShadowColorAndOpacity"), Args._TextStyle->ShadowColorAndOpacity);
					Default(TEXT("Margin"), FMargin(0));
					Default(TEXT("TransformPolicy"), ETextTransformPolicy::None);
					Default(TEXT("OverflowPolicy"), ETextOverflowPolicy::Clip);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("Border", SBorder)
					SLATE_DEFAULT(Padding);
					SLATE_DEFAULT(HAlign);
					SLATE_DEFAULT(VAlign);
					SLATE_DEFAULT(ColorAndOpacity);
					SLATE_DEFAULT(BorderBackgroundColor);
					SLATE_DEFAULT(DesiredSizeScale);
					SLATE_DEFAULT(ShowEffectWhenDisabled);
					SLATE_DEFAULT(FlipForRightToLeftFlowDirection);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("UserWidget", SBorder)
					SLATE_DEFAULT(Padding);
					SLATE_DEFAULT(ColorAndOpacity);
					SLATE_DEFAULT(ForegroundColor);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("Button", SButton)
					SLATE_DEFAULT(ContentPadding);
					SLATE_DEFAULT(HAlign);
					SLATE_DEFAULT(VAlign);
					SLATE_DEFAULT(ButtonColorAndOpacity);
					SLATE_DEFAULT(IsFocusable);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("Image", SImage)
					SLATE_DEFAULT(ColorAndOpacity);
					SLATE_DEFAULT(FlipForRightToLeftFlowDirection);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("SizeBox", SBox)
					SLATE_DEFAULT(Padding);
					SLATE_DEFAULT(HAlign);
					SLATE_DEFAULT(VAlign);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("ScaleBox", SScaleBox)
					SLATE_DEFAULT(HAlign);
					SLATE_DEFAULT(VAlign);
					SLATE_DEFAULT(Stretch);
					SLATE_DEFAULT(StretchDirection);
					SLATE_DEFAULT(UserSpecifiedScale);
					SLATE_DEFAULT(IgnoreInheritedScale);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("CanvasPanel", SConstraintCanvas)
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("HorizontalBox", SHorizontalBox)
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("VerticalBox", SVerticalBox)
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("GridPanel", SGridPanel)
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("UniformGridPanel", SUniformGridPanel)
					SLATE_DEFAULT(SlotPadding);
					SLATE_DEFAULT(MinDesiredSlotWidth);
					SLATE_DEFAULT(MinDesiredSlotHeight);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("WrapBox", SWrapBox)
					SLATE_DEFAULT(InnerSlotPadding);
					SLATE_DEFAULT(Orientation);
					SLATE_DEFAULT(HAlign);
					SLATE_DEFAULT(PreferredSize);
					SLATE_DEFAULT(UseAllottedSize);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("ScrollBox", SScrollBox)
					SLATE_DEFAULT(Orientation);
					SLATE_DEFAULT(ScrollBarVisibility);
					SLATE_DEFAULT(ScrollBarPadding);
					SLATE_DEFAULT(ScrollBarAlwaysVisible);
					Default(TEXT("ScrollBarThickness"), FVector2D(Args._ScrollBarThickness));
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("WidgetSwitcher", SWidgetSwitcher)
					SLATE_DEFAULT(WidgetIndex);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("Spacer", SSpacer)
					SLATE_DEFAULT(Size);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("ProgressBar", SProgressBar)
					SLATE_DEFAULT(Percent);
					SLATE_DEFAULT(BarFillType);
					SLATE_DEFAULT(BarFillStyle);
					SLATE_DEFAULT(FillColorAndOpacity);
					SLATE_DEFAULT(BorderPadding);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("CheckBox", SCheckBox)
					SLATE_DEFAULT(IsChecked);
					SLATE_DEFAULT(HAlign);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("Slider", SSlider)
					SLATE_DEFAULT(Orientation);
					SLATE_DEFAULT(Value);
					SLATE_DEFAULT(MinValue);
					SLATE_DEFAULT(MaxValue);
					SLATE_DEFAULT(SliderBarColor);
					SLATE_DEFAULT(SliderHandleColor);
					SLATE_DEFAULT(IndentHandle);
					SLATE_DEFAULT(Locked);
					SLATE_DEFAULT(StepSize);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("BackgroundBlur", SBackgroundBlur)
					SLATE_DEFAULT(HAlign);
					SLATE_DEFAULT(VAlign);
					SLATE_DEFAULT(Padding);
					SLATE_DEFAULT(BlurStrength);
					SLATE_DEFAULT(bApplyAlphaToBlur);
					SLATE_DEFAULT(CornerRadius);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("SafeZone", SSafeZone)
					SLATE_DEFAULT(HAlign);
					SLATE_DEFAULT(VAlign);
					SLATE_DEFAULT(Padding);
					SLATE_DEFAULT(IsTitleSafe);
					SLATE_DEFAULT(SafeAreaScale);
					SLATE_DEFAULT(PadLeft);
					SLATE_DEFAULT(PadRight);
					SLATE_DEFAULT(PadTop);
					SLATE_DEFAULT(PadBottom);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("EditableText", SEditableText)
					SLATE_DEFAULT(Text);
					SLATE_DEFAULT(HintText);
					SLATE_DEFAULT(IsReadOnly);
					SLATE_DEFAULT(IsPassword);
					SLATE_DEFAULT(Justification);
					SLATE_DEFAULT(MinDesiredWidth);
					SLATE_DEFAULT(OverflowPolicy);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("EditableTextBox", SEditableTextBox)
					SLATE_DEFAULT(Text);
					SLATE_DEFAULT(HintText);
					SLATE_DEFAULT(IsReadOnly);
					SLATE_DEFAULT(IsPassword);
					SLATE_DEFAULT(Justification);
					SLATE_DEFAULT(MinDesiredWidth);
					SLATE_DEFAULT(OverflowPolicy);
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("InvalidationBox", SInvalidationPanel)
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("RetainerBox", SBox)
				SLATE_DEFAULT_END
				SLATE_DEFAULT_BEGIN("NamedSlot", SBox)
				SLATE_DEFAULT_END
				CommonDefaults(SOverlay::FArguments());
#undef SLATE_DEFAULT_END
#undef SLATE_DEFAULT_BEGIN
#undef SLATE_DEFAULT
			}

			void SetSlotDefaults(const FString& Kind)
			{
				AttributeDefaults.Reset();
				if (!Options.bOnlyNonDefaultValues) { return; }
				Default(TEXT("HAlign"), HAlign_Fill);
				Default(TEXT("VAlign"), VAlign_Fill);
				Default(TEXT("Padding"), FMargin(0));
				if (Kind == TEXT("CanvasPanel"))
				{
					const SConstraintCanvas::FSlot Slot;
					Default(TEXT("Offset"), Slot.GetOffset());
					Default(TEXT("Alignment"), Slot.GetAlignment());
					Default(TEXT("AutoSize"), Slot.GetAutoSize());
					AttributeDefaults.Add(TEXT("Anchors"), TEXT("FAnchors(0.0f, 0.0f, 0.0f, 0.0f)"));
					Default(TEXT("ZOrder"), 0);
				}
				if (Kind == TEXT("GridPanel"))
				{
					Default(TEXT("RowSpan"), 1);
					Default(TEXT("ColumnSpan"), 1);
					Default(TEXT("Nudge"), FVector2D::ZeroVector);
				}
				if (Kind == TEXT("HorizontalBox") || Kind == TEXT("VerticalBox")) { AttributeDefaults.Add(TEXT("SizeParam"), TEXT("FStretch(1.0f)")); }
				if (Kind == TEXT("ScrollBox")) { AttributeDefaults.Add(TEXT("SizeParam"), TEXT("FAuto()")); }
				if (Kind == TEXT("WrapBox"))
				{
					Default(TEXT("FillEmptySpace"), false);
					Default(TEXT("ForceNewLine"), false);
				}
			}

			void Attr(FString& Out, const FString& Indent, const TCHAR* Name, const FString& Expression)
			{
				if (Options.bOnlyNonDefaultValues)
				{
					if (const FString* Default = AttributeDefaults.Find(Name); Default && (*Default == Expression || *Default == TEXT("FSlateColor(") + Expression + TEXT(")") || Expression == TEXT("FSlateColor(") + *Default + TEXT(")"))) { return; }
				}
				if (!Expression.IsEmpty()) { Out += Indent + TEXT(".") + Name + TEXT("(") + Expression + TEXT(")\n"); }
			}

			void Copy(FString& Out, const FString& Indent, const UObject* Object, const TCHAR* Property, const TCHAR* Attribute = nullptr)
			{
				Attr(Out, Indent, Attribute ? Attribute : Property, Value(Object, Property));
			}

			void Style(FString& Out, const FString& Indent, const UObject* Object, const TCHAR* Property, const TCHAR* Attribute, bool bPointer = true)
			{
				Attr(Out, Indent, Attribute, Resource(Object, Property, bPointer));
			}

			const UWidgetTree* FindTree(const UWidgetBlueprint* Blueprint)
			{
				if (Blueprint->WidgetTree && Blueprint->WidgetTree->RootWidget) { return Blueprint->WidgetTree; }
				for (UClass* Class = Blueprint->ParentClass; Class; Class = Class->GetSuperClass())
				{
					if (const UWidgetBlueprint* Parent = Cast<UWidgetBlueprint>(Class->ClassGeneratedBy))
					{
						if (Parent->WidgetTree && Parent->WidgetTree->RootWidget) { return Parent->WidgetTree; }
					}
				}
				return nullptr;
			}

			void Slot(FString& Out, const FString& I, const FWidgetType& Type, const UWidget* Child)
			{
				const UObject* S = Child->Slot;
				const FString Kind(Type.UMG);
				SetSlotDefaults(Kind);
				FString Args;
				if (Kind == TEXT("GridPanel") || Kind == TEXT("UniformGridPanel"))
				{
					Args = Value(S, TEXT("Column")) + TEXT(", ") + Value(S, TEXT("Row"));
					if (Kind == TEXT("GridPanel")) { Args += TEXT(", SGridPanel::Layer(") + Value(S, TEXT("Layer")) + TEXT(")"); }
				}
				Out += I + TEXT("+ ") + Type.Slate + TEXT("::Slot(") + Args + TEXT(")\n");
				if (const UCanvasPanelSlot* Canvas = Cast<UCanvasPanelSlot>(S))
				{
					const FAnchors Anchors = Canvas->GetAnchors();
					Attr(Out, I, TEXT("Anchors"), TEXT("FAnchors(") + Number(Anchors.Minimum.X) + TEXT(", ") + Number(Anchors.Minimum.Y) + TEXT(", ") + Number(Anchors.Maximum.X) + TEXT(", ") + Number(Anchors.Maximum.Y) + TEXT(")"));
					Attr(Out, I, TEXT("Offset"), Margin(Canvas->GetOffsets()));
					Attr(Out, I, TEXT("Alignment"), Vector(Canvas->GetAlignment()));
					Attr(Out, I, TEXT("AutoSize"), Canvas->GetAutoSize() ? TEXT("true") : TEXT("false"));
					Attr(Out, I, TEXT("ZOrder"), FString::FromInt(Canvas->GetZOrder()));
					return;
				}
				Copy(Out, I, S, TEXT("HorizontalAlignment"), TEXT("HAlign"));
				Copy(Out, I, S, TEXT("VerticalAlignment"), TEXT("VAlign"));
				Copy(Out, I, S, TEXT("Padding"));
				if (Kind == TEXT("HorizontalBox") || Kind == TEXT("VerticalBox") || Kind == TEXT("ScrollBox"))
				{
					if (const FStructProperty* P = S ? FindFProperty<FStructProperty>(S->GetClass(), TEXT("Size")) : nullptr)
					{
						const FSlateChildSize& Size = *P->ContainerPtrToValuePtr<FSlateChildSize>(S);
						Attr(Out, I, TEXT("SizeParam"), Size.SizeRule == ESlateSizeRule::Fill ? TEXT("FStretch(") + Number(Size.Value) + TEXT(")") : TEXT("FAuto()"));
					}
				}
				if (Kind == TEXT("GridPanel"))
				{
					Copy(Out, I, S, TEXT("RowSpan"));
					Copy(Out, I, S, TEXT("ColumnSpan"));
					Copy(Out, I, S, TEXT("Nudge"));
				}
				if (Kind == TEXT("WrapBox"))
				{
					Copy(Out, I, S, TEXT("bFillEmptySpace"), TEXT("FillEmptySpace"));
					Copy(Out, I, S, TEXT("bForceNewLine"), TEXT("ForceNewLine"));
					const FString Fill = Value(S, TEXT("FillSpanWhenLessThan"));
					if (!Fill.IsEmpty() && FCString::Atof(*Fill) != 0) { Attr(Out, I, TEXT("FillLineWhenSizeLessThan"), Fill); }
				}
			}

			void Properties(FString& Out, const FString& I, const UWidget* W, const FString& Kind)
			{
				SetDefaults(Kind);
				Copy(Out, I, W, TEXT("Visibility"));
				Copy(Out, I, W, TEXT("bIsEnabled"), TEXT("IsEnabled"));
				Copy(Out, I, W, TEXT("RenderOpacity"));
				Copy(Out, I, W, TEXT("Clipping"));
				Copy(Out, I, W, TEXT("PixelSnapping"), TEXT("PixelSnappingMethod"));
				Copy(Out, I, W, TEXT("RenderTransformPivot"));
				Copy(Out, I, W, TEXT("FlowDirectionPreference"));
				const FWidgetTransform Transform = W->GetRenderTransform();
				if (!Transform.IsIdentity())
				{
					Attr(Out, I, TEXT("RenderTransform"), TEXT("Concatenate(FScale2D(") + Vector(Transform.Scale) + TEXT("), FShear2D::FromShearAngles(") + Vector(Transform.Shear) + TEXT("), FQuat2D(FMath::DegreesToRadians(") + Number(Transform.Angle) + TEXT(")), ") + Vector(Transform.Translation) + TEXT(")"));
				}
				if (Kind == TEXT("TextBlock"))
				{
					for (const TCHAR* P : { TEXT("Text"), TEXT("ColorAndOpacity"), TEXT("ShadowOffset"), TEXT("ShadowColorAndOpacity"), TEXT("MinDesiredWidth"), TEXT("Justification"), TEXT("WrappingPolicy"), TEXT("WrapTextAt"), TEXT("LineHeightPercentage"), TEXT("Margin") }) { Copy(Out, I, W, P); }
					Copy(Out, I, W, TEXT("bAutoWrapText"), TEXT("AutoWrapText"));
					Copy(Out, I, W, TEXT("bApplyLineHeightToBottomLine"), TEXT("ApplyLineHeightToBottomLine"));
					Copy(Out, I, W, TEXT("TextTransformPolicy"), TEXT("TransformPolicy"));
					Copy(Out, I, W, TEXT("TextOverflowPolicy"), TEXT("OverflowPolicy"));
					Style(Out, I, W, TEXT("Font"), TEXT("Font"), false);
					Style(Out, I, W, TEXT("StrikeBrush"), TEXT("StrikeBrush"));
				}
				else if (Kind == TEXT("Image"))
				{
					Style(Out, I, W, TEXT("Brush"), TEXT("Image"));
					Copy(Out, I, W, TEXT("ColorAndOpacity"));
					Copy(Out, I, W, TEXT("bFlipForRightToLeftFlowDirection"), TEXT("FlipForRightToLeftFlowDirection"));
				}
				else if (Kind == TEXT("Border"))
				{
					Style(Out, I, W, TEXT("Background"), TEXT("BorderImage"));
					Copy(Out, I, W, TEXT("BrushColor"), TEXT("BorderBackgroundColor"));
					Copy(Out, I, W, TEXT("ContentColorAndOpacity"), TEXT("ColorAndOpacity"));
					Copy(Out, I, W, TEXT("DesiredSizeScale"));
					Copy(Out, I, W, TEXT("bShowEffectWhenDisabled"), TEXT("ShowEffectWhenDisabled"));
					Copy(Out, I, W, TEXT("bFlipForRightToLeftFlowDirection"), TEXT("FlipForRightToLeftFlowDirection"));
				}
				else if (Kind == TEXT("Button"))
				{
					Style(Out, I, W, TEXT("WidgetStyle"), TEXT("ButtonStyle"));
					Copy(Out, I, W, TEXT("BackgroundColor"), TEXT("ButtonColorAndOpacity"));
					Copy(Out, I, W, TEXT("IsFocusable"));
				}
				else if (Kind == TEXT("SizeBox"))
				{
					for (const TCHAR* P : { TEXT("WidthOverride"), TEXT("HeightOverride"), TEXT("MinDesiredWidth"), TEXT("MinDesiredHeight"), TEXT("MaxDesiredWidth"), TEXT("MaxDesiredHeight"), TEXT("MinAspectRatio"), TEXT("MaxAspectRatio") })
					{
						if (Flag(W, *(TEXT("bOverride_") + FString(P)))) { Copy(Out, I, W, P); }
					}
				}
				else if (Kind == TEXT("ScaleBox"))
				{
					for (const TCHAR* P : { TEXT("Stretch"), TEXT("StretchDirection"), TEXT("UserSpecifiedScale"), TEXT("IgnoreInheritedScale") }) { Copy(Out, I, W, P); }
				}
				else if (Kind == TEXT("Spacer")) { Copy(Out, I, W, TEXT("Size")); }
				else if (Kind == TEXT("UniformGridPanel"))
				{
					for (const TCHAR* P : { TEXT("SlotPadding"), TEXT("MinDesiredSlotWidth"), TEXT("MinDesiredSlotHeight") }) { Copy(Out, I, W, P); }
				}
				else if (Kind == TEXT("GridPanel"))
				{
					for (const TCHAR* P : { TEXT("ColumnFill"), TEXT("RowFill") })
					{
						if (const FArrayProperty* A = FindFProperty<FArrayProperty>(W->GetClass(), P))
						{
							FScriptArrayHelper Array(A, A->ContainerPtrToValuePtr<void>(W));
							for (int32 Index = 0; Index < Array.Num(); ++Index) { Attr(Out, I, P, FString::FromInt(Index) + TEXT(", ") + PropertyValue(A->Inner, Array.GetRawPtr(Index))); }
						}
					}
				}
				else if (Kind == TEXT("WrapBox"))
				{
					Copy(Out, I, W, TEXT("InnerSlotPadding"));
					Copy(Out, I, W, TEXT("Orientation"));
					Copy(Out, I, W, TEXT("HorizontalAlignment"), TEXT("HAlign"));
					Copy(Out, I, W, TEXT("WrapSize"), TEXT("PreferredSize"));
					Attr(Out, I, TEXT("UseAllottedSize"), Flag(W, TEXT("bExplicitWrapSize")) ? TEXT("false") : TEXT("true"));
				}
				else if (Kind == TEXT("WidgetSwitcher")) { Copy(Out, I, W, TEXT("ActiveWidgetIndex"), TEXT("WidgetIndex")); }
				else if (Kind == TEXT("ProgressBar"))
				{
					Style(Out, I, W, TEXT("WidgetStyle"), TEXT("Style"));
					for (const TCHAR* P : { TEXT("BarFillType"), TEXT("BarFillStyle"), TEXT("FillColorAndOpacity"), TEXT("BorderPadding") }) { Copy(Out, I, W, P); }
					Attr(Out, I, TEXT("Percent"), Flag(W, TEXT("bIsMarquee")) ? TEXT("TOptional<float>()") : Value(W, TEXT("Percent")));
				}
				else if (Kind == TEXT("CheckBox"))
				{
					Style(Out, I, W, TEXT("WidgetStyle"), TEXT("Style"));
					Copy(Out, I, W, TEXT("CheckedState"), TEXT("IsChecked"));
					Copy(Out, I, W, TEXT("HorizontalAlignment"), TEXT("HAlign"));
				}
				else if (Kind == TEXT("Slider"))
				{
					Style(Out, I, W, TEXT("WidgetStyle"), TEXT("Style"));
					for (const TCHAR* P : { TEXT("Orientation"), TEXT("Value"), TEXT("MinValue"), TEXT("MaxValue"), TEXT("SliderBarColor"), TEXT("SliderHandleColor"), TEXT("IndentHandle"), TEXT("Locked"), TEXT("StepSize") }) { Copy(Out, I, W, P); }
				}
				else if (Kind == TEXT("ScrollBox"))
				{
					Style(Out, I, W, TEXT("WidgetStyle"), TEXT("Style"));
					Style(Out, I, W, TEXT("WidgetBarStyle"), TEXT("ScrollBarStyle"));
					Copy(Out, I, W, TEXT("Orientation"));
					Copy(Out, I, W, TEXT("ScrollBarVisibility"));
					Copy(Out, I, W, TEXT("ScrollbarThickness"), TEXT("ScrollBarThickness"));
					Copy(Out, I, W, TEXT("ScrollbarPadding"), TEXT("ScrollBarPadding"));
					Copy(Out, I, W, TEXT("AlwaysShowScrollbar"), TEXT("ScrollBarAlwaysVisible"));
					if (Flag(W, TEXT("AlwaysShowScrollbarTrack"))) { Warn(TEXT("ScrollBox: call SetScrollBarTrackAlwaysVisible(true) after construction.")); }
				}
				else if (Kind == TEXT("BackgroundBlur"))
				{
					Copy(Out, I, W, TEXT("BlurStrength"));
					Copy(Out, I, W, TEXT("CornerRadius"));
					Copy(Out, I, W, TEXT("bApplyAlphaToBlur"));
					if (Flag(W, TEXT("bOverrideAutoRadiusCalculation"))) { Copy(Out, I, W, TEXT("BlurRadius")); }
					Style(Out, I, W, TEXT("LowQualityFallbackBrush"), TEXT("LowQualityFallbackBrush"));
				}
				else if (Kind == TEXT("SafeZone"))
				{
					for (const TCHAR* P : { TEXT("PadLeft"), TEXT("PadRight"), TEXT("PadTop"), TEXT("PadBottom") }) { Copy(Out, I, W, P); }
				}
				else if (Kind == TEXT("EditableText") || Kind == TEXT("EditableTextBox"))
				{
					Style(Out, I, W, TEXT("WidgetStyle"), TEXT("Style"));
					for (const TCHAR* P : { TEXT("Text"), TEXT("HintText"), TEXT("IsReadOnly"), TEXT("IsPassword"), TEXT("Justification"), TEXT("MinDesiredWidth") }) { Copy(Out, I, W, P); }
					Copy(Out, I, W, TEXT("MinimumDesiredWidth"), TEXT("MinDesiredWidth"));
					Copy(Out, I, W, TEXT("OverflowPolicy"));
				}
			}

			FString Widget(const UWidget* W, int32 Depth)
			{
				const FString I = FString::ChrN(Depth, '\t');
				if (!W) { return I + TEXT("SNullWidget::NullWidget\n"); }
				if (Depth > 128 || ActiveWidgets.Contains(W) || Result.WidgetCount >= 10000)
				{
					Warn(TEXT("Recursive or oversized widget hierarchy truncated."));
					return I + TEXT("SNullWidget::NullWidget // TODO: hierarchy limit\n");
				}
				ActiveWidgets.Add(W);
				++Result.WidgetCount;
				FString Out = I + TEXT("// ") + Comment(W->GetName()) + TEXT(" (") + Comment(W->GetClass()->GetName()) + TEXT(")\n");
				for (TFieldIterator<FProperty> It(W->GetClass()); It; ++It) { AssetComments(*It, It->ContainerPtrToValuePtr<void>(W), It->GetName(), Out, I); }
				const FWidgetType* Type = nullptr;
				for (const UClass* Class = W->GetClass(); Class && !Type; Class = Class->GetSuperClass())
				{
					for (const FWidgetType& Candidate : WidgetTypes)
					{
						if (Class->GetFName() == Candidate.UMG)
						{
							Type = &Candidate;
							break;
						}
					}
				}
				if (const UUserWidget* User = Cast<UUserWidget>(W))
				{
					Includes.Add(TEXT("Widgets/Layout/SBorder.h"));
					Out += I + TEXT("SNew(SBorder)\n") + I + TEXT("\t.BorderImage(nullptr)\n");
					Properties(Out, I + TEXT("\t"), W, TEXT("UserWidget"));
					Copy(Out, I + TEXT("\t"), W, TEXT("ColorAndOpacity"));
					Copy(Out, I + TEXT("\t"), W, TEXT("ForegroundColor"));
					Copy(Out, I + TEXT("\t"), W, TEXT("Padding"));
					const UWidgetBlueprint* BP = Cast<UWidgetBlueprint>(User->GetClass()->ClassGeneratedBy);
					const UWidgetTree* Tree = BP ? FindTree(BP) : User->WidgetTree;
					Out += I + TEXT("[\n");
					if (Tree && Tree->RootWidget && !ActiveTrees.Contains(Tree))
					{
						ActiveTrees.Add(Tree);
						AddSlotOverrides(Tree, Tree);
						AddSlotOverrides(Tree, User);
						Out += Widget(Tree->RootWidget, Depth + 1);
						SlotOverrides.Remove(Tree);
						ActiveTrees.Remove(Tree);
						Warn(TEXT("Nested UserWidget uses its designer template; PreConstruct and instance bindings are not executed."));
					}
					else
					{
						Out += I + TEXT("\tSNullWidget::NullWidget // TODO: unavailable or recursive UserWidget template\n");
						Warn(TEXT("Unavailable or recursive UserWidget: ") + W->GetName());
					}
					Out += I + TEXT("]\n");
				}
				else
				{
					const FWidgetType Fallback = { TEXT("Unsupported"), TEXT("SOverlay"), TEXT("Widgets/SOverlay.h"), true };
					if (!Type)
					{
						Type = &Fallback;
						Warn(TEXT("Unsupported widget replaced with an overlay preserving children: ") + W->GetName() + TEXT(" (") + W->GetClass()->GetName() + TEXT(")"));
						Out += I + TEXT("// TODO: implement custom visual for this widget.\n");
					}
					else
						if (W->GetClass()->GetFName() != Type->UMG) { Warn(TEXT("Custom subclass exported as its native base: ") + W->GetName()); }
					Includes.Add(Type->Header);
					const FString ButtonColor = FString(Type->UMG) == TEXT("Button") ? Value(W, TEXT("ColorAndOpacity")) : FString();
					if (!ButtonColor.IsEmpty() && (!Options.bOnlyNonDefaultValues || ButtonColor != Color(FLinearColor::White)))
					{
						const FString ButtonName = FString::Printf(TEXT("Button%d"), Result.WidgetCount);
						Members += TEXT("\tTSharedPtr<SButton> ") + ButtonName + TEXT(";\n");
						Out += I + TEXT("SAssignNew(") + ButtonName + TEXT(", SButton)\n");
						PostInitialization += TEXT("\t\t") + ButtonName + TEXT("->SetColorAndOpacity(") + ButtonColor + TEXT(");\n");
					}
					else { Out += I + TEXT("SNew(") + Type->Slate + TEXT(")\n"); }
					Properties(Out, I + TEXT("\t"), W, Type->UMG);
					if (FString(Type->UMG) == TEXT("RetainerBox")) { Warn(TEXT("RetainerBox exported as SBox; effect material and render phases omitted.")); }
					if (const UPanelWidget* Panel = Cast<UPanelWidget>(W))
					{
						if (!Type->bPanel)
						{
							const UObject* S = Panel->GetChildrenCount() ? Panel->GetChildAt(0)->Slot : nullptr;
							const FString Kind(Type->UMG);
							if (Kind != TEXT("CheckBox") && Kind != TEXT("InvalidationBox"))
							{
								const UObject* AlignSource = (Kind == TEXT("Border") || Kind == TEXT("BackgroundBlur")) ? W : S;
								Copy(Out, I + TEXT("\t"), AlignSource, TEXT("HorizontalAlignment"), TEXT("HAlign"));
								Copy(Out, I + TEXT("\t"), AlignSource, TEXT("VerticalAlignment"), TEXT("VAlign"));
								if (Kind != TEXT("ScaleBox")) { Copy(Out, I + TEXT("\t"), AlignSource, TEXT("Padding"), Kind == TEXT("Button") ? TEXT("ContentPadding") : TEXT("Padding")); }
							}
							if (Kind == TEXT("SafeZone"))
							{
								Copy(Out, I + TEXT("\t"), S, TEXT("HAlign"));
								Copy(Out, I + TEXT("\t"), S, TEXT("VAlign"));
								Copy(Out, I + TEXT("\t"), S, TEXT("SafeAreaScale"));
								Copy(Out, I + TEXT("\t"), S, TEXT("bIsTitleSafe"), TEXT("IsTitleSafe"));
							}
						}
						const UWidget* NamedContent = nullptr;
						if (FString(Type->UMG) == TEXT("NamedSlot"))
						{
							if (const TMap<FName, const UWidget*>* Overrides = SlotOverrides.Find(W->GetTypedOuter<UWidgetTree>())) { NamedContent = Overrides->FindRef(W->GetFName()); }
						}
						const int32 ChildCount = NamedContent ? 1 : Panel->GetChildrenCount();
						for (int32 Index = 0; Index < ChildCount; ++Index)
						{
							const UWidget* Child = NamedContent ? NamedContent : Panel->GetChildAt(Index);
							if (!Child) { continue; }
							if (Type->bPanel)
							{
								if (Type == &Fallback) { Out += I + TEXT("+ SOverlay::Slot()\n"); }
								else { Slot(Out, I, *Type, Child); }
							}
							Out += I + TEXT("[\n") + Widget(Child, Depth + 1) + I + TEXT("]\n");
						}
					}
				}
				ActiveWidgets.Remove(W);
				return Out;
			}
		};
	}

	FUMGToSlateResult FUMGToSlateGenerator::Generate(const UWidgetBlueprint* Blueprint)
	{
		return Generate(Blueprint, FUMGToSlateOptions());
	}

	FUMGToSlateResult FUMGToSlateGenerator::Generate(const UWidgetBlueprint* Blueprint, const FUMGToSlateOptions& Options)
	{
		FGenerator G;
		G.Options = Options;
		if (!Blueprint)
		{
			G.Result.Warnings.Add(TEXT("Select a Widget Blueprint to generate Slate code."));
			return G.Result;
		}
		const UWidgetTree* Tree = G.FindTree(Blueprint);
		if (!Tree)
		{
			G.Result.Warnings.Add(TEXT("The Widget Blueprint and its parents have no designer root widget."));
			return G.Result;
		}
		G.ActiveTrees.Add(Tree);
		G.AddSlotOverrides(Tree, Tree);
		if (Blueprint->WidgetTree && Blueprint->WidgetTree != Tree) { G.AddSlotOverrides(Tree, Blueprint->WidgetTree); }
		const FString Hierarchy = G.Widget(Tree->RootWidget, 3);
		FString RootPadding;
		if (const UUserWidget* Defaults = Blueprint->GeneratedClass ? Cast<UUserWidget>(Blueprint->GeneratedClass->GetDefaultObject()) : nullptr)
		{
			if (!Options.bOnlyNonDefaultValues || Defaults->GetColorAndOpacity() != FLinearColor::White) { G.Initialization += TEXT("\t\tSetColorAndOpacity(") + Color(Defaults->GetColorAndOpacity()) + TEXT(");\n"); }
			if (!Options.bOnlyNonDefaultValues || Defaults->GetForegroundColor() != FSlateColor::UseForeground()) { G.Initialization += TEXT("\t\tSetForegroundColor(") + G.Value(Defaults, TEXT("ForegroundColor")) + TEXT(");\n"); }
			if (!Options.bOnlyNonDefaultValues || Defaults->GetPadding() != FMargin(0)) { RootPadding = TEXT("\t\t.Padding(") + Margin(Defaults->GetPadding()) + TEXT(")\n"); }
		}
		FString ClassName = TEXT("SGenerated_");
		for (const TCHAR C : Blueprint->GetName()) { ClassName += ((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9')) ? C : TCHAR('_'); }
		G.Includes.Append({ TEXT("CoreMinimal.h"), TEXT("Widgets/SCompoundWidget.h"), TEXT("Widgets/SNullWidget.h"), TEXT("Styling/CoreStyle.h"), TEXT("Styling/SlateTypes.h"), TEXT("Math/TransformCalculus2D.h") });
		TArray<FString> Headers = G.Includes.Array();
		Headers.Sort();
		FString Code = TEXT("// Generated from: ") + Comment(Blueprint->GetPathName()) + TEXT("\n// Designer snapshot only: no Blueprint logic, bindings, animations or PreConstruct.\n// Requires Core, Slate and SlateCore. Resource paths appear only in comments.\n// Host this widget at the original viewport size / DPI scale for matching layout.\n");
		for (const FString& Warning : G.Result.Warnings) { Code += TEXT("// WARNING: ") + Warning + TEXT("\n"); }
		Code += TEXT("#pragma once\n\n");
		for (const FString& Header : Headers) { Code += TEXT("#include \"") + Header + TEXT("\"\n"); }
		Code += TEXT("\nclass ") + ClassName + TEXT(" : public SCompoundWidget\n{\npublic:\n\tSLATE_BEGIN_ARGS(") + ClassName + TEXT(") {}\n\tSLATE_END_ARGS()\n\n\tvoid Construct(const FArguments& InArgs)\n\t{\n") + G.Initialization + TEXT("\n\t\tChildSlot\n") + RootPadding + TEXT("\t\t[\n") + Hierarchy + TEXT("\t\t];\n") + G.PostInitialization + TEXT("\t}\n");
		if (!G.Members.IsEmpty()) { Code += TEXT("\nprivate:\n\t// Keep styles alive for as long as the Slate widgets reference them.\n") + G.Members; }
		G.Result.Code = Code + TEXT("};\n");
		return MoveTemp(G.Result);
	}
}