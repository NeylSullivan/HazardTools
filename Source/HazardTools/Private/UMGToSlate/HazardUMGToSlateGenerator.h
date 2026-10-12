// Copyright Neyl Sullivan 2026

#pragma once

#include "CoreMinimal.h"

class UWidgetBlueprint;

namespace HazardTools
{
	struct FUMGToSlateOptions
	{
		/** Keep only values required to reproduce the UMG design with Slate defaults. */
		bool bOnlyNonDefaultValues = true;
	};

	struct FUMGToSlateResult
	{
		FString Code;
		TArray<FString> Warnings;
		int32 WidgetCount = 0;
	};

	/** Exports designer templates only. Does not instantiate widgets or execute Blueprint code. */
	class FUMGToSlateGenerator
	{
	public:
		static FUMGToSlateResult Generate(const UWidgetBlueprint* Blueprint);
		static FUMGToSlateResult Generate(const UWidgetBlueprint* Blueprint, const FUMGToSlateOptions& Options);
	};
}