#include "ObjectOutlinerFilter.h"

#include "MeshDescriptionBase.h"
#include "EditorFramework/AssetImportData.h"
#include "EditorFramework/ThumbnailInfo.h"

namespace HazardTools
{
TSharedPtr<FObjectOutlinerFilter> FObjectOutlinerFilter::MakeForClasses(FName FilterName, const FText& Title, const TArray<UClass*> InClasses, FText InCustomTooltip /*= FText()*/)
{
	const TSharedPtr<FObjectOutlinerFilter> Result = MakeShared<FObjectOutlinerFilter>(FilterName, Title, EMode::Classes);
	Result->Classes = InClasses;

	if (InCustomTooltip.IsEmptyOrWhitespace())
	{
		FTextBuilder TooltipTextBuilder;
		TooltipTextBuilder.AppendLine(INVTEXT("Filter for classes derived from:"));
		TooltipTextBuilder.Indent();
		for (const UClass* Class : Result->Classes)
		{
			if (Class != nullptr)
			{
				TooltipTextBuilder.AppendLine(FText::FromString(Class->GetName()));
			}
		}
		Result->Tooltip = TooltipTextBuilder.ToText();
	}
	else
	{
		Result->Tooltip = InCustomTooltip;
	}
	return Result;
}

TSharedPtr<FObjectOutlinerFilter> FObjectOutlinerFilter::MakeForClassNames(FName FilterName, const FText& Title, const TArray<FName> InClassNames, FText InCustomTooltip /*= FText()*/)
{
	const TSharedPtr<FObjectOutlinerFilter> Result = MakeShared<FObjectOutlinerFilter>(FilterName, Title, EMode::ClassNames);
	Result->ClassNames = InClassNames;

	if (InCustomTooltip.IsEmptyOrWhitespace())
	{
		FTextBuilder TooltipTextBuilder;
		TooltipTextBuilder.AppendLine(INVTEXT("Filter for exact classes"));
		TooltipTextBuilder.Indent();
		for (const FName& ClassName : Result->ClassNames)
		{
			if (ClassName.IsNone() == false)
			{
				TooltipTextBuilder.AppendLine(FText::FromName(ClassName));
			}
		}
		Result->Tooltip = TooltipTextBuilder.ToText();
	}
	else
	{
		Result->Tooltip = InCustomTooltip;
	}
	return Result;
}

TSharedPtr<FObjectOutlinerFilter> FObjectOutlinerFilter::MakeCustom(FName FilterName, const FText& Title, const FText& InTooltip, const TFunction<bool(const UObject* TestObject)> InFunction)
{
	const TSharedPtr<FObjectOutlinerFilter> Result = MakeShared<FObjectOutlinerFilter>(FilterName, Title, EMode::Custom);
	Result->CustomFunction = InFunction;
	Result->Tooltip = InTooltip;
	return Result;
}

bool FObjectOutlinerFilter::IsPass(const UObject* Object)
{
	if (Object == nullptr)
	{
		return false;
	}

	switch (Mode)
	{
		case EMode::None:
			checkNoEntry();
			break;
		case EMode::Classes:
			for (const UClass* Class : Classes)
			{
				if (ensure(Class != nullptr) && Object->IsA(Class))
				{
					return true;
				}
			}
			break;
		case EMode::ClassNames:
			for (const FName& ClassName : ClassNames)
			{
				if (ensure(ClassName.IsNone() == false) && Object->GetClass()->GetFName() == ClassName)
				{
					return true;
				}
			}
			break;
		case EMode::Custom:
			if (CustomFunction(Object))
			{
				return true;
			}
			break;
		default:
			checkNoEntry();
	}

	return false;
}

void GenerateImperativeFilters(TArray<TSharedPtr<FObjectOutlinerFilter>>& OutFilters)
{
	OutFilters.Reset();

	OutFilters.Add(FObjectOutlinerFilter::MakeCustom("Filter_CDO",
		INVTEXT("CDO"),
		INVTEXT("Class Default Objects"),
		[](const UObject* TestObject)
		{
			if (TestObject->IsTemplate(RF_ClassDefaultObject))
			{
				return true;
			}
			return false;
		}));
}

void GenerateDropDownFilters(TArray<TSharedPtr<FObjectOutlinerFilter>>& OutFilters)
{
	OutFilters.Reset();

	OutFilters.Add(FObjectOutlinerFilter::MakeForClasses("Filter_Package", INVTEXT("Package"), {UPackage::StaticClass()}));
	OutFilters.Add(FObjectOutlinerFilter::MakeForClasses("Filter_Fields", INVTEXT("Fields"), {UField::StaticClass()}, INVTEXT("Filter for structs, classes, functions, delegates etc")));
	OutFilters.Add(FObjectOutlinerFilter::MakeForClasses("Filter_Meta",
		INVTEXT("Meta"),
		{
			UMetaData::StaticClass(),
			UAssetImportData::StaticClass(),
			UMeshDescriptionBaseBulkData::StaticClass(),
			UThumbnailInfo::StaticClass(),
			UMeshDescriptionBase::StaticClass()
		}));

	OutFilters.Add(FObjectOutlinerFilter::MakeCustom("Filter_Transient",
		INVTEXT("Transient"),
		INVTEXT("Objects contained in transient package"),
		[](const UObject* TestObject)
		{
			if (const UPackage* ContainerPackage = TestObject->GetOutermost(); ContainerPackage == GetTransientPackage() || ContainerPackage->HasAnyFlags(RF_Transient))
			{
				return true;
			}
			return false;
		}));

	OutFilters.Add(FObjectOutlinerFilter::MakeForClasses("Filter_Materials",
		INVTEXT("Materials"),
		{
			UMaterialExpression::StaticClass(),
			UMaterialInterface::StaticClass(),
			UMaterialFunctionInterface::StaticClass()
		}));

	OutFilters.Add(FObjectOutlinerFilter::MakeForClasses("Filter_Textures", INVTEXT("Textures"), {UTexture::StaticClass()}));

	OutFilters.Add(FObjectOutlinerFilter::MakeForClassNames("Filter_Niagara",
		INVTEXT("Niagara"),
		{
			FName("NiagaraScriptVariable"),
			FName("NiagaraParameterDefinitions")
		}));

	OutFilters.Add(FObjectOutlinerFilter::MakeForClasses("Filter_Subsystem", INVTEXT("Subsystem"), {USubsystem::StaticClass()}));
	OutFilters.Add(FObjectOutlinerFilter::MakeForClasses("Filter_Actor", INVTEXT("Actors"), {AActor::StaticClass()}));
	OutFilters.Last()->bEnabled = true;
}

bool ShouldSkipObject(const UObject* Object, const TArray<TSharedPtr<FObjectOutlinerFilter>>& Filters)
{
	if (Object)
	{
		for (const TSharedPtr<FObjectOutlinerFilter> Filter : Filters)
		{
			if (Filter.IsValid())
			{
				if (Filter->IsPass(Object))
				{
					return Filter->bEnabled == false;
				}
			}
		}
	}
	return false;
}

bool IsMatchAnyDisabledFilter(const UObject* Object, const TArray<TSharedPtr<FObjectOutlinerFilter>>& Filters)
{
	if (Object)
	{
		for (const TSharedPtr<FObjectOutlinerFilter> Filter : Filters)
		{
			if (Filter.IsValid() && Filter->bEnabled == false && Filter->IsPass(Object))
			{
				return true;
			}
		}
	}
	return false;
}

bool IsMatchAnyEnabledFilter(const UObject* Object, const TArray<TSharedPtr<FObjectOutlinerFilter>>& Filters)
{
	if (Object)
	{
		for (const TSharedPtr<FObjectOutlinerFilter> Filter : Filters)
		{
			if (Filter.IsValid() && Filter->bEnabled && Filter->IsPass(Object))
			{
				return true;
			}
		}
	}
	return false;
}
}
