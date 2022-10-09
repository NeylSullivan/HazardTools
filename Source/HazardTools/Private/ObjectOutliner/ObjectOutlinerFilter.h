#pragma once

namespace HazardTools
{
struct FObjectOutlinerFilter
{
	FName FilterName;
	FText Title;
	FText Tooltip;

	bool bEnabled = false;

	TArray<UClass*> Classes;
	TArray<FName> ClassNames;
	TFunction<bool (const UObject* TestObject)> CustomFunction;

	//private:
	enum class EMode
	{
		None,
		Classes,
		ClassNames,
		Custom
	};

	const EMode Mode;

	FObjectOutlinerFilter(const FName FilterName, const FText& Title, const EMode Mode)
		: FilterName(FilterName)
		, Title(Title)
		, Mode(Mode)
	{
	}

public:
	static TSharedPtr<FObjectOutlinerFilter> MakeForClasses(FName FilterName, const FText& Title, const TArray<UClass*> InClasses, FText InCustomTooltip = FText());
	static TSharedPtr<FObjectOutlinerFilter> MakeForClassNames(FName FilterName, const FText& Title, const TArray<FName> InClassNames, FText InCustomTooltip = FText());
	static TSharedPtr<FObjectOutlinerFilter> MakeCustom(FName FilterName, const FText& Title, const FText& InTooltip, const TFunction<bool (const UObject* TestObject)> InFunction);

	bool IsPass(const UObject* Object);
};

void GenerateImperativeFilters(TArray<TSharedPtr<FObjectOutlinerFilter>>& OutFilters);
void GenerateDropDownFilters(TArray<TSharedPtr<FObjectOutlinerFilter>>& OutFilters);

bool ShouldSkipObject(const UObject* Object, const TArray<TSharedPtr<FObjectOutlinerFilter>>& Filters);

bool IsMatchAnyDisabledFilter(const UObject* Object, const TArray<TSharedPtr<FObjectOutlinerFilter>>& Filters);
bool IsMatchAnyEnabledFilter(const UObject* Object, const TArray<TSharedPtr<FObjectOutlinerFilter>>& Filters);
}
