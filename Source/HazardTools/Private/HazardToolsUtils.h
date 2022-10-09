// Copyright Neyl Sullivan 2022

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class FHazardToolsUtils
{
public:
	// Does content browser has assets selected?
	static bool HasSelectedAssets();

	static void SetClipboardText(const FString& ClipboardText, const bool bLog = true, const bool bShowNotification = true, const bool bSuccess = true);

	// Does not work with inheritance !
	template <typename InStructType>
	static bool UStructToText(const InStructType& InStruct, FText& OutText)
	{
		return UStructToText(InStructType::StaticStruct(), &InStruct, OutText);
	}

	static bool UStructToText(const UStruct* StructDefinition, const void* Struct, FText& OutText);

	static const FString Tag_Prefix_Type;
	static const FString Tag_Prefix_Literal;
	static const FString Tag_Suffix;
};
