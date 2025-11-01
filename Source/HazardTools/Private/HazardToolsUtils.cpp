// Copyright Neyl Sullivan 2022


#include "HazardToolsUtils.h"
#include "HazardTools.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Windows/WindowsPlatformApplicationMisc.h"

const FString FHazardToolsUtils::Tag_Prefix_Type = TEXT("<SyntaxHighlight.Node>");
const FString FHazardToolsUtils::Tag_Prefix_Literal = TEXT("<SyntaxHighlight.NodeAttributeValue>");

const FString FHazardToolsUtils::Tag_Suffix = TEXT("</>");

bool FHazardToolsUtils::HasSelectedAssets()
{
	if (!GEditor)
	{
		return false;
	}

	TArray<FAssetData> SelectedAssets;
	GEditor->GetContentBrowserSelections(SelectedAssets);

	return SelectedAssets.Num() > 0;
}

void FHazardToolsUtils::SetClipboardText(const FString& ClipboardText, const bool bLog, const bool bShowNotification, const bool bSuccess)
{
	FPlatformApplicationMisc::ClipboardCopy(*ClipboardText);

	if (bShowNotification)
	{
		const FText NotificationText = FText::Format(INVTEXT("Copied to clipboard:\n{0}"), FText::FromString(ClipboardText));
		FNotificationInfo Info(NotificationText);
		Info.ExpireDuration = 5.0f;

		if (const TSharedPtr<SNotificationItem> InfoItem = FSlateNotificationManager::Get().AddNotification(Info))
		{
			const SNotificationItem::ECompletionState State = bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail;
			InfoItem->SetCompletionState(State);
		}
	}

	if (bLog)
	{
		if (bSuccess)
		{
			UE_LOG(LogHazardTools, Log, TEXT("Copied to clipboard: %s"), *ClipboardText);
		}
		else
		{
			UE_LOG(LogHazardTools, Warning, TEXT("Copied to clipboard: %s"), *ClipboardText);
		}
	}
}

bool IsPropertyBuiltinStruct(const FStructProperty* StructProperty)
{
	static TSet<FName> BuiltinStructNames =
	{
		NAME_Color, NAME_LinearColor,
		NAME_Vector2D, NAME_Vector2d, NAME_Vector2f,
		NAME_Vector, NAME_Vector3d, NAME_Vector3f,
		NAME_Vector4, NAME_Vector4d, NAME_Vector4f,
		NAME_Box2d, NAME_Box2f,
		FMargin::StaticStruct()->GetFName(),
		FSlateColor::StaticStruct()->GetFName()
	};

	return StructProperty != nullptr ? BuiltinStructNames.Contains(StructProperty->Struct->GetFName()) : false;
}

[[nodiscard]] bool WrapPathWithHyperlinkIfPossible(FString& InOutString)
{
	if (FPaths::FileExists(InOutString))
	{
		const FString& Prefix = FString::Printf(TEXT("<a id=\"FileHyperLink\" href=\"%s\" style=\"DetailsView.BPMessageHyperlinkStyle\">"), *InOutString);
		InOutString = Prefix + InOutString + FHazardToolsUtils::Tag_Suffix;
		return true;
	}
	return false;
}


FString BuiltinStructPropertyToCompactString_Internal(const UStruct* StructDefinition, const void* Struct)
{
	FString Result;
	Result += TEXT("{");

	for (TFieldIterator<FProperty> It(StructDefinition); It; ++It)
	{
		const FProperty* Property = *It;
		FString VariableName = Property->GetNameCPP();
		const void* Value = Property->ContainerPtrToValuePtr<uint8>(Struct);
		FString ValueString;

		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			const UEnum* EnumDef = EnumProperty->GetEnum();
			ValueString = EnumDef->GetAuthoredNameStringByValue(EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(Value));
		}
		else if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
		{
			if (const UEnum* EnumDef = NumericProperty->GetIntPropertyEnum(); EnumDef != nullptr)
			{
				ValueString = EnumDef->GetAuthoredNameStringByValue(NumericProperty->GetSignedIntPropertyValue(Value));
			}
			else if (NumericProperty->IsFloatingPoint())
			{
				const double Val = NumericProperty->GetFloatingPointPropertyValue(Value);
				ValueString = FString::SanitizeFloat(Val, 0);
			}
			else if (NumericProperty->IsInteger())
			{
				const int64 Val = NumericProperty->GetSignedIntPropertyValue(Value);
				ValueString = FString::FromInt(Val);
			}
		}
		else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property); IsPropertyBuiltinStruct(StructProperty))
		{
			ValueString = BuiltinStructPropertyToCompactString_Internal(StructProperty->Struct, Value);
		}
		else
		{
			ValueString = TEXT("<Log.Error>UNRECOGNIZED</>");
		}

		Result += FString::Printf(TEXT("%s=%s,"), *VariableName, *ValueString);
	}
	Result.RemoveFromEnd(TEXT(","));
	Result += TEXT("}");
	return Result;
}

void UStructToText_Internal(const UStruct* StructDefinition, const void* Struct, FTextBuilder& Builder)
{
	Builder.AppendLine(INVTEXT("{"));
	Builder.Indent();

	for (TFieldIterator<FProperty> It(StructDefinition); It; ++It)
	{
		const FProperty* Property = *It;
		FString VariableName = Property->GetNameCPP();
		FString ClassName = Property->GetCPPType();
		const void* Value = Property->ContainerPtrToValuePtr<uint8>(Struct);

		// Try to clean up namespaced enum class name
		if (ClassName.StartsWith(TEXT("T"), ESearchCase::CaseSensitive))
		{
			if (ClassName.RemoveFromStart(TEXT("TEnumAsByte<")))
			{
				ClassName.RemoveFromEnd(TEXT("::Type>"));
			}
		}

		const FString PrefixString = FString::Printf(TEXT("%s%s</> %s ="), *FHazardToolsUtils::Tag_Prefix_Type, *ClassName, *VariableName);

		if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			if (IsPropertyBuiltinStruct(StructProperty))
			{
				FString ValueString = BuiltinStructPropertyToCompactString_Internal(StructProperty->Struct, Value);
				Builder.AppendLineFormat(INVTEXT("{0} <SyntaxHighlight.NodeAttributeValue>{1}</>"), FText::FromString(PrefixString), FText::FromString(ValueString));
			}
			/*else 
			{
				FString ValueString;
				Property->ExportTextItem(ValueString, Value, nullptr, nullptr, 0);
				Builder.AppendLineFormat(INVTEXT("{0} <Log.Command>{1}</>"), FText::FromString(PrefixString), FText::FromString(ValueString));
			}*/
			else
			{
				Builder.AppendLine(FText::FromString(PrefixString));
				UStructToText_Internal(StructProperty->Struct, Value, Builder);
			}
		}
		else
		{
			FString ValueString;
			// Property->ExportTextItem(ValueString, Value, nullptr, nullptr, 0);
			// UE 5.1 update
			Property->ExportTextItem_Direct(ValueString, Value, nullptr, nullptr, 0);

			if (WrapPathWithHyperlinkIfPossible(ValueString))
			{
				// Value already formatted
				Builder.AppendLineFormat(INVTEXT("{0} {1}"), FText::FromString(PrefixString), FText::FromString(ValueString));
			}
			else
			{
				//Builder.AppendLineFormat(INVTEXT("{0} <Log.Command>{1}</>"), FText::FromString(PrefixString), FText::FromString(ValueString));
				// UE 5.5 update. <Log.Command> (green text) not exist anymore
				Builder.AppendLineFormat(INVTEXT("{0} <SyntaxHighlight.SourceCode.Number>{1}</>"), FText::FromString(PrefixString), FText::FromString(ValueString));
			}
		}
	}
	Builder.Unindent();
	Builder.AppendLine(INVTEXT("}"));
	Builder.AppendLine();
}

bool FHazardToolsUtils::UStructToText(const UStruct* StructDefinition, const void* Struct, FText& OutText)
{
	FTextBuilder Builder;

	Builder.AppendLineFormat(INVTEXT("{0}{1}:"), FText::FromString(StructDefinition->GetPrefixCPP()), FText::FromString(StructDefinition->GetName()));
	UStructToText_Internal(StructDefinition, Struct, Builder);

	OutText = Builder.ToText();

	return true;
}
