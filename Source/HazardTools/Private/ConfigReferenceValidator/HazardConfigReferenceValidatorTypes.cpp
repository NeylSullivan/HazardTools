// Copyright Neyl Sullivan 2026

#include "ConfigReferenceValidator/HazardConfigReferenceValidatorTypes.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/Blueprint.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/PackageName.h"
#include "Misc/ScopedSlowTask.h"
#include "UObject/CoreRedirects.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/UObjectIterator.h"

namespace HazardTools::ConfigReferenceValidator
{
static FString StatusToString(const EHazardConfigReferenceStatus Status)
{
	switch (Status)
	{
	case EHazardConfigReferenceStatus::Valid:
		return TEXT("Valid");
	case EHazardConfigReferenceStatus::Invalid:
		return TEXT("Invalid");
	case EHazardConfigReferenceStatus::Empty:
		return TEXT("Empty");
	case EHazardConfigReferenceStatus::Redirected:
		return TEXT("Redirected");
	case EHazardConfigReferenceStatus::Unsupported:
		return TEXT("Unsupported");
	case EHazardConfigReferenceStatus::PendingFix:
		return TEXT("Pending Fix");
	case EHazardConfigReferenceStatus::SaveFailed:
		return TEXT("Save Failed");
	default:
		return TEXT("Unknown");
	}
}

static FString ReferenceKindToString(const EHazardConfigReferenceKind Kind)
{
	switch (Kind)
	{
	case EHazardConfigReferenceKind::SoftObject:
		return TEXT("Soft Object");
	case EHazardConfigReferenceKind::SoftClass:
		return TEXT("Soft Class");
	case EHazardConfigReferenceKind::SoftObjectPath:
		return TEXT("FSoftObjectPath");
	case EHazardConfigReferenceKind::SoftClassPath:
		return TEXT("FSoftClassPath");
	case EHazardConfigReferenceKind::HardObject:
		return TEXT("Hard Object");
	case EHazardConfigReferenceKind::HardClass:
		return TEXT("Hard Class");
	case EHazardConfigReferenceKind::Unsupported:
	default:
		return TEXT("Unsupported");
	}
}

static bool IsClassReferenceKind(const EHazardConfigReferenceKind Kind)
{
	return Kind == EHazardConfigReferenceKind::SoftClass
		|| Kind == EHazardConfigReferenceKind::SoftClassPath
		|| Kind == EHazardConfigReferenceKind::HardClass;
}

static bool IsSoftReferenceKind(const EHazardConfigReferenceKind Kind)
{
	return Kind == EHazardConfigReferenceKind::SoftObject
		|| Kind == EHazardConfigReferenceKind::SoftClass
		|| Kind == EHazardConfigReferenceKind::SoftObjectPath
		|| Kind == EHazardConfigReferenceKind::SoftClassPath;
}

static FString SanitizeExportedReference(FString ExportedValue)
{
	ExportedValue.TrimStartAndEndInline();
	if (ExportedValue.Equals(TEXT("None"), ESearchCase::IgnoreCase) || ExportedValue.Equals(TEXT("Null"), ESearchCase::IgnoreCase))
	{
		return FString();
	}

	const int32 FirstQuote = ExportedValue.Find(TEXT("'"));
	const int32 LastQuote = ExportedValue.Find(TEXT("'"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	if (FirstQuote != INDEX_NONE && LastQuote != INDEX_NONE && LastQuote > FirstQuote)
	{
		return ExportedValue.Mid(FirstQuote + 1, LastQuote - FirstQuote - 1);
	}

	return ExportedValue;
}

static FString MakeConfigSectionName(const UClass* SettingsClass)
{
	if (!SettingsClass)
	{
		return FString();
	}

	return SettingsClass->GetPathName();
}

static FTopLevelAssetPath RedirectorClassPath()
{
	return UObjectRedirector::StaticClass()->GetClassPathName();
}

static FString GetObjectPathFromAssetData(const FAssetData& AssetData)
{
	return AssetData.GetSoftObjectPath().ToString();
}

static FString GetFieldDisplayName(const FField* Field)
{
	return Field ? Field->GetFName().ToString() : TEXT("<invalid>");
}

static UClass* GetDeclaringClassForProperty(const FProperty* Property, UClass* FallbackClass)
{
	if (!Property)
	{
		return FallbackClass;
	}

	if (UClass* OwnerClass = Cast<UClass>(Property->GetOwnerUObject()))
	{
		return OwnerClass;
	}

	return FallbackClass;
}
}

FString FHazardConfigReferencePath::ToString() const
{
	FString Result;
	for (const FHazardConfigReferencePathSegment& Segment : Segments)
	{
		switch (Segment.Type)
		{
		case EHazardConfigReferencePathSegmentType::Property:
			if (!Result.IsEmpty())
			{
				Result += TEXT(".");
			}
			Result += Segment.DisplayText;
			break;
		case EHazardConfigReferencePathSegmentType::ArrayIndex:
			Result += FString::Printf(TEXT("[%d]"), Segment.Index);
			break;
		case EHazardConfigReferencePathSegmentType::SetIndex:
			Result += FString::Printf(TEXT("[%d]"), Segment.Index);
			break;
		case EHazardConfigReferencePathSegmentType::MapKey:
			Result += FString::Printf(TEXT("[%s]"), *Segment.DisplayText);
			break;
		case EHazardConfigReferencePathSegmentType::MapValue:
			Result += FString::Printf(TEXT("[%s]"), *Segment.DisplayText);
			break;
		default:
			break;
		}
	}
	return Result;
}

bool FHazardConfigReferencePath::Resolve(UObject* Owner, void*& OutValuePtr, FProperty*& OutLeafProperty, FText& OutError) const
{
	OutValuePtr = nullptr;
	OutLeafProperty = nullptr;

	if (!Owner)
	{
		OutError = INVTEXT("Settings object is no longer valid.");
		return false;
	}

	void* CurrentContainer = Owner;
	FProperty* CurrentProperty = nullptr;

	for (const FHazardConfigReferencePathSegment& Segment : Segments)
	{
		switch (Segment.Type)
		{
		case EHazardConfigReferencePathSegmentType::Property:
			if (!Segment.Property || !CurrentContainer)
			{
				OutError = INVTEXT("Property path contains an invalid property segment.");
				return false;
			}
			CurrentContainer = Segment.Property->ContainerPtrToValuePtr<void>(CurrentContainer);
			CurrentProperty = Segment.Property;
			break;

		case EHazardConfigReferencePathSegmentType::ArrayIndex:
			{
				FArrayProperty* ArrayProperty = CastField<FArrayProperty>(CurrentProperty);
				if (!ArrayProperty || !CurrentContainer)
				{
					OutError = INVTEXT("Property path expected an array container.");
					return false;
				}
				FScriptArrayHelper ArrayHelper(ArrayProperty, CurrentContainer);
				if (!ArrayHelper.IsValidIndex(Segment.Index))
				{
					OutError = INVTEXT("Array index is no longer valid.");
					return false;
				}
				CurrentContainer = ArrayHelper.GetRawPtr(Segment.Index);
				CurrentProperty = ArrayProperty->Inner;
				break;
			}

		case EHazardConfigReferencePathSegmentType::SetIndex:
			{
				FSetProperty* SetProperty = CastField<FSetProperty>(CurrentProperty);
				if (!SetProperty || !CurrentContainer)
				{
					OutError = INVTEXT("Property path expected a set container.");
					return false;
				}
				FScriptSetHelper SetHelper(SetProperty, CurrentContainer);
				if (!SetHelper.IsValidIndex(Segment.Index))
				{
					OutError = INVTEXT("Set index is no longer valid.");
					return false;
				}
				CurrentContainer = SetHelper.GetElementPtr(Segment.Index);
				CurrentProperty = SetProperty->ElementProp;
				break;
			}

		case EHazardConfigReferencePathSegmentType::MapKey:
		case EHazardConfigReferencePathSegmentType::MapValue:
			{
				FMapProperty* MapProperty = CastField<FMapProperty>(CurrentProperty);
				if (!MapProperty || !CurrentContainer)
				{
					OutError = INVTEXT("Property path expected a map container.");
					return false;
				}
				FScriptMapHelper MapHelper(MapProperty, CurrentContainer);
				if (!MapHelper.IsValidIndex(Segment.Index))
				{
					OutError = INVTEXT("Map entry is no longer valid.");
					return false;
				}
				if (Segment.Type == EHazardConfigReferencePathSegmentType::MapKey)
				{
					CurrentContainer = MapHelper.GetKeyPtr(Segment.Index);
					CurrentProperty = MapProperty->KeyProp;
				}
				else
				{
					CurrentContainer = MapHelper.GetValuePtr(Segment.Index);
					CurrentProperty = MapProperty->ValueProp;
				}
				break;
			}
		default:
			OutError = INVTEXT("Property path contains an unsupported segment.");
			return false;
		}
	}

	if (!CurrentContainer || !CurrentProperty)
	{
		OutError = INVTEXT("Property path did not resolve to a value.");
		return false;
	}

	OutValuePtr = CurrentContainer;
	OutLeafProperty = CurrentProperty;
	return true;
}

bool FHazardConfigReferenceRow::IsClassReference() const
{
	return HazardTools::ConfigReferenceValidator::IsClassReferenceKind(ReferenceKind);
}

bool FHazardConfigReferenceRow::IsWritable() const
{
	return ReferencePath.bWritable && SettingsObject.IsValid();
}

bool FHazardConfigReferenceRow::HasPendingReplacement() const
{
	return !PendingReplacementPath.IsEmpty();
}

bool FHazardConfigReferenceRow::IsFixable() const
{
	return IsWritable() && (!PendingReplacementPath.IsEmpty() || !SuggestedReplacementPath.IsEmpty());
}

FString FHazardConfigReferenceRow::GetStatusString() const
{
	return HazardTools::ConfigReferenceValidator::StatusToString(Status);
}

FString FHazardConfigReferenceRow::GetReferenceKindString() const
{
	return HazardTools::ConfigReferenceValidator::ReferenceKindToString(ReferenceKind);
}

FString FHazardConfigReferenceRow::GetSettingsClassName() const
{
	const UClass* Class = SettingsClass.Get();
	return Class ? Class->GetName() : TEXT("<invalid>");
}

FString FHazardConfigReferenceRow::GetDeclaringClassName() const
{
	const UClass* Class = DeclaringClass.Get();
	return Class ? Class->GetName() : GetSettingsClassName();
}

FString FHazardConfigReferenceRow::GetReplacementPathForApply() const
{
	return !PendingReplacementPath.IsEmpty() ? PendingReplacementPath : SuggestedReplacementPath;
}

FText FHazardConfigReferenceRow::GetToolTipText() const
{
	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("Status: %s"), *GetStatusString()));
	Lines.Add(FString::Printf(TEXT("Settings Class: %s"), *GetSettingsClassName()));
	Lines.Add(FString::Printf(TEXT("Declaring Class: %s"), *GetDeclaringClassName()));
	Lines.Add(FString::Printf(TEXT("Config File: %s"), *ConfigFile));
	Lines.Add(FString::Printf(TEXT("Config Section: %s"), *ConfigSection));
	Lines.Add(FString::Printf(TEXT("Property Path: %s"), *ReferencePath.ToString()));
	Lines.Add(FString::Printf(TEXT("Reference Type: %s"), *GetReferenceKindString()));
	if (const UClass* ExpectedClass = ExpectedBaseClass.Get())
	{
		Lines.Add(FString::Printf(TEXT("Expected Type: %s"), *ExpectedClass->GetName()));
	}
	if (!CurrentPath.IsEmpty())
	{
		Lines.Add(FString::Printf(TEXT("Current Path: %s"), *CurrentPath));
	}
	if (!SuggestedReplacementPath.IsEmpty())
	{
		Lines.Add(FString::Printf(TEXT("Suggested Replacement: %s"), *SuggestedReplacementPath));
	}
	if (!PendingReplacementPath.IsEmpty())
	{
		Lines.Add(FString::Printf(TEXT("Pending Replacement: %s"), *PendingReplacementPath));
	}
	if (!ReferencePath.bWritable && !ReferencePath.ReadOnlyReason.IsEmpty())
	{
		Lines.Add(FString::Printf(TEXT("Read Only: %s"), *ReferencePath.ReadOnlyReason.ToString()));
	}
	if (!Message.IsEmpty())
	{
		Lines.Add(FString::Printf(TEXT("Message: %s"), *Message.ToString()));
	}
	return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

void FHazardConfigReferenceValidator::Validate(const FHazardConfigReferenceRowPtr& Row) const
{
	if (!Row.IsValid())
	{
		return;
	}

	Row->ResolvedObjectPath.Reset();
	if (Row->HasPendingReplacement())
	{
		Row->Status = EHazardConfigReferenceStatus::PendingFix;
		Row->Message = INVTEXT("A replacement is pending and has not been saved.");
		return;
	}

	if (Row->ReferenceKind == EHazardConfigReferenceKind::Unsupported)
	{
		Row->Status = EHazardConfigReferenceStatus::Unsupported;
		if (Row->Message.IsEmpty())
		{
			Row->Message = INVTEXT("This reflected property shape is not supported for editing.");
		}
		return;
	}

	if (Row->CurrentPath.IsEmpty())
	{
		Row->Status = EHazardConfigReferenceStatus::Empty;
		Row->Message = INVTEXT("Reference is empty.");
		return;
	}

	FString ResolvedPath;
	FString Message;
	if (Row->IsClassReference() && DoesNativeClassExist(Row->CurrentPath, Row->ExpectedBaseClass.Get(), ResolvedPath, Message))
	{
		Row->Status = EHazardConfigReferenceStatus::Valid;
		Row->ResolvedObjectPath = ResolvedPath;
		Row->Message = FText::FromString(Message);
		return;
	}

	if (DoesAssetExist(Row->CurrentPath, *Row, ResolvedPath, Message))
	{
		Row->Status = EHazardConfigReferenceStatus::Valid;
		Row->ResolvedObjectPath = ResolvedPath;
		Row->Message = FText::FromString(Message);
		return;
	}

	if (TryResolveRedirect(Row))
	{
		Row->Status = EHazardConfigReferenceStatus::Redirected;
		return;
	}

	Row->Status = EHazardConfigReferenceStatus::Invalid;
	Row->Message = FText::FromString(Message.IsEmpty() ? TEXT("Reference path could not be resolved.") : Message);
}

bool FHazardConfigReferenceValidator::TryResolveRedirect(const FHazardConfigReferenceRowPtr& Row) const
{
	if (!Row.IsValid() || Row->CurrentPath.IsEmpty())
	{
		return false;
	}

	if (Row->IsClassReference())
	{
		const FCoreRedirectObjectName OldName(Row->CurrentPath);
		const FCoreRedirectObjectName NewName = FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Class, OldName);
		const FString RedirectedPath = NewName.ToString();
		if (!RedirectedPath.IsEmpty() && RedirectedPath != Row->CurrentPath)
		{
			Row->SuggestedReplacementPath = RedirectedPath;
			Row->Message = FText::Format(INVTEXT("Native class redirect resolves to '{0}'."), FText::FromString(RedirectedPath));
			return true;
		}
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	const FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(Row->CurrentPath), true);
	if (!AssetData.IsValid() || AssetData.AssetClassPath != HazardTools::ConfigReferenceValidator::RedirectorClassPath())
	{
		return false;
	}

	UObject* LoadedObject = AssetData.GetAsset();
	UObjectRedirector* Redirector = Cast<UObjectRedirector>(LoadedObject);
	if (!Redirector || !Redirector->DestinationObject)
	{
		return false;
	}

	FString DestinationPath = Redirector->DestinationObject->GetPathName();
	if (Row->IsClassReference())
	{
		if (const UBlueprint* Blueprint = Cast<UBlueprint>(Redirector->DestinationObject))
		{
			if (Blueprint->GeneratedClass)
			{
				DestinationPath = GetPathForClass(Blueprint->GeneratedClass);
			}
		}
	}

	Row->SuggestedReplacementPath = DestinationPath;
	Row->Message = FText::Format(INVTEXT("Redirector resolves to '{0}'."), FText::FromString(DestinationPath));
	return true;
}

FString FHazardConfigReferenceValidator::NormalizeClassPathFromAsset(const FAssetData& AssetData, const UClass* ExpectedBaseClass)
{
	if (!AssetData.IsValid())
	{
		return FString();
	}

	UObject* LoadedAsset = AssetData.GetAsset();
	if (const UBlueprint* Blueprint = Cast<UBlueprint>(LoadedAsset))
	{
		if (Blueprint->GeneratedClass && (!ExpectedBaseClass || Blueprint->GeneratedClass->IsChildOf(ExpectedBaseClass)))
		{
			return GetPathForClass(Blueprint->GeneratedClass);
		}
	}
	if (const UClass* Class = Cast<UClass>(LoadedAsset))
	{
		if (!ExpectedBaseClass || Class->IsChildOf(ExpectedBaseClass))
		{
			return GetPathForClass(Class);
		}
	}

	const FString AssetPath = HazardTools::ConfigReferenceValidator::GetObjectPathFromAssetData(AssetData);
	if (AssetPath.EndsWith(TEXT("_C")))
	{
		return AssetPath;
	}

	return FString::Printf(TEXT("%s_C"), *AssetPath);
}

FString FHazardConfigReferenceValidator::GetPathForClass(const UClass* Class)
{
	return Class ? FSoftClassPath(Class).ToString() : FString();
}

bool FHazardConfigReferenceValidator::DoesAssetExist(const FString& Path, const FHazardConfigReferenceRow& Row, FString& OutResolvedPath, FString& OutMessage) const
{
	if (!FPackageName::IsValidObjectPath(Path))
	{
		OutMessage = FString::Printf(TEXT("'%s' is not a valid object path."), *Path);
		return false;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(Path), true);
	if (!AssetData.IsValid() && Row.IsClassReference())
	{
		const FString BlueprintAssetPath = GetBlueprintAssetPathFromGeneratedClassPath(Path);
		if (!BlueprintAssetPath.IsEmpty())
		{
			AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(BlueprintAssetPath), true);
		}
	}

	if (!AssetData.IsValid())
	{
		OutMessage = FString::Printf(TEXT("Asset Registry could not find '%s'."), *Path);
		return false;
	}

	if (AssetData.AssetClassPath == HazardTools::ConfigReferenceValidator::RedirectorClassPath())
	{
		OutMessage = TEXT("Path points at an asset redirector.");
		return false;
	}

	OutResolvedPath = HazardTools::ConfigReferenceValidator::GetObjectPathFromAssetData(AssetData);
	OutMessage = TEXT("Asset Registry found the referenced asset.");
	return true;
}

bool FHazardConfigReferenceValidator::DoesNativeClassExist(const FString& Path, const UClass* ExpectedBaseClass, FString& OutResolvedPath, FString& OutMessage) const
{
	if (!Path.StartsWith(TEXT("/Script/")))
	{
		return false;
	}

	UClass* Class = FindObject<UClass>(nullptr, *Path);
	if (!Class)
	{
		Class = LoadClass<UObject>(nullptr, *Path);
	}

	if (!Class)
	{
		OutMessage = FString::Printf(TEXT("Native class '%s' could not be found."), *Path);
		return false;
	}

	if (ExpectedBaseClass && !Class->IsChildOf(ExpectedBaseClass))
	{
		OutMessage = FString::Printf(TEXT("Native class '%s' is not a child of '%s'."), *Path, *ExpectedBaseClass->GetName());
		return false;
	}

	OutResolvedPath = GetPathForClass(Class);
	OutMessage = TEXT("Native class resolved.");
	return true;
}

FString FHazardConfigReferenceValidator::GetBlueprintAssetPathFromGeneratedClassPath(const FString& Path)
{
	FString PackagePath;
	FString ObjectName;
	if (!Path.Split(TEXT("."), &PackagePath, &ObjectName, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
	{
		return FString();
	}

	if (!ObjectName.EndsWith(TEXT("_C")))
	{
		return FString();
	}

	ObjectName.LeftChopInline(2);
	return FString::Printf(TEXT("%s.%s"), *PackagePath, *ObjectName);
}

void FHazardConfigReferenceScanner::Scan(const FHazardConfigReferenceScanOptions& Options, TArray<FHazardConfigReferenceRowPtr>& OutRows) const
{
	OutRows.Reset();
	FHazardConfigReferenceValidator Validator;

	TArray<UClass*> SettingsClasses;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (ShouldScanSettingsClass(Class))
		{
			bool bProject = false;
			bool bPlugin = false;
			bool bEngine = false;
			if (ShouldIncludeSettingsClass(Class, Options, bProject, bPlugin, bEngine))
			{
				SettingsClasses.Add(Class);
			}
		}
	}

	SettingsClasses.Sort([](const UClass& A, const UClass& B)
	{
		return A.GetName() < B.GetName();
	});

	FScopedSlowTask SlowTask(SettingsClasses.Num(), INVTEXT("Scanning Developer Settings config references..."));
	SlowTask.MakeDialogDelayed(0.5f);
	for (UClass* SettingsClass : SettingsClasses)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(SettingsClass->GetName()));
		ScanSettingsClass(SettingsClass, Options, OutRows);
	}

	for (const FHazardConfigReferenceRowPtr& Row : OutRows)
	{
		Validator.Validate(Row);
	}
}

void FHazardConfigReferenceScanner::ScanSettingsClass(UClass* SettingsClass, const FHazardConfigReferenceScanOptions& Options, TArray<FHazardConfigReferenceRowPtr>& OutRows) const
{
	bool bProject = false;
	bool bPlugin = false;
	bool bEngine = false;
	if (!ShouldIncludeSettingsClass(SettingsClass, Options, bProject, bPlugin, bEngine))
	{
		return;
	}

	UObject* SettingsObject = SettingsClass ? SettingsClass->GetDefaultObject() : nullptr;
	if (!SettingsObject)
	{
		return;
	}

	SettingsObject->ReloadConfig(SettingsClass);

	FTraversalContext Context;
	Context.SettingsObject = SettingsObject;
	Context.SettingsClass = SettingsClass;
	Context.ConfigName = SettingsClass->ClassConfigName.ToString();
	Context.ConfigFile = GetConfigFilename(SettingsObject);
	Context.ConfigSection = HazardTools::ConfigReferenceValidator::MakeConfigSectionName(SettingsClass);
	Context.bProjectSettings = bProject;
	Context.bPluginSettings = bPlugin;
	Context.bEngineSettings = bEngine;

	for (TFieldIterator<FProperty> It(SettingsClass, EFieldIterationFlags::IncludeSuper); It; ++It)
	{
		FProperty* Property = *It;
		if (!Property || !Property->HasAnyPropertyFlags(CPF_Config))
		{
			continue;
		}

		FTraversalContext PropertyContext = Context;
		PropertyContext.DeclaringClass = HazardTools::ConfigReferenceValidator::GetDeclaringClassForProperty(Property, SettingsClass);
		FHazardConfigReferencePathSegment Segment;
		Segment.Type = EHazardConfigReferencePathSegmentType::Property;
		Segment.Property = Property;
		Segment.DisplayText = HazardTools::ConfigReferenceValidator::GetFieldDisplayName(Property);
		PropertyContext.Path.Segments.Add(Segment);

		void* ValuePtr = Property->ContainerPtrToValuePtr<void>(SettingsObject);
		TraverseProperty(Property, ValuePtr, PropertyContext, OutRows);
	}
}

void FHazardConfigReferenceScanner::TraverseProperty(FProperty* Property, void* ValuePtr, FTraversalContext Context, TArray<FHazardConfigReferenceRowPtr>& OutRows) const
{
	if (!Property || !ValuePtr)
	{
		return;
	}

	if (FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
	{
		AddReferenceRow(SoftClassProperty, ValuePtr, Context, EHazardConfigReferenceKind::SoftClass, GetExpectedBaseClass(SoftClassProperty, EHazardConfigReferenceKind::SoftClass), OutRows);
		return;
	}
	if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
	{
		AddReferenceRow(SoftObjectProperty, ValuePtr, Context, EHazardConfigReferenceKind::SoftObject, GetExpectedBaseClass(SoftObjectProperty, EHazardConfigReferenceKind::SoftObject), OutRows);
		return;
	}
	if (FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
	{
		AddReferenceRow(ClassProperty, ValuePtr, Context, EHazardConfigReferenceKind::HardClass, GetExpectedBaseClass(ClassProperty, EHazardConfigReferenceKind::HardClass), OutRows);
		return;
	}
	if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
	{
		AddReferenceRow(ObjectProperty, ValuePtr, Context, EHazardConfigReferenceKind::HardObject, GetExpectedBaseClass(ObjectProperty, EHazardConfigReferenceKind::HardObject), OutRows);
		return;
	}

	if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		if (StructProperty->Struct == FSoftClassPath::StaticStruct())
		{
			AddReferenceRow(StructProperty, ValuePtr, Context, EHazardConfigReferenceKind::SoftClassPath, UObject::StaticClass(), OutRows);
			return;
		}
		if (StructProperty->Struct == FSoftObjectPath::StaticStruct())
		{
			AddReferenceRow(StructProperty, ValuePtr, Context, EHazardConfigReferenceKind::SoftObjectPath, UObject::StaticClass(), OutRows);
			return;
		}

		for (TFieldIterator<FProperty> It(StructProperty->Struct); It; ++It)
		{
			FProperty* ChildProperty = *It;
			if (!ChildProperty)
			{
				continue;
			}

			FTraversalContext ChildContext = Context;
			FHazardConfigReferencePathSegment Segment;
			Segment.Type = EHazardConfigReferencePathSegmentType::Property;
			Segment.Property = ChildProperty;
			Segment.DisplayText = HazardTools::ConfigReferenceValidator::GetFieldDisplayName(ChildProperty);
			ChildContext.Path.Segments.Add(Segment);
			TraverseProperty(ChildProperty, ChildProperty->ContainerPtrToValuePtr<void>(ValuePtr), ChildContext, OutRows);
		}
		return;
	}

	if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		FScriptArrayHelper ArrayHelper(ArrayProperty, ValuePtr);
		for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
		{
			FTraversalContext ElementContext = Context;
			FHazardConfigReferencePathSegment Segment;
			Segment.Type = EHazardConfigReferencePathSegmentType::ArrayIndex;
			Segment.Index = Index;
			Segment.DisplayText = FString::FromInt(Index);
			ElementContext.Path.Segments.Add(Segment);
			TraverseProperty(ArrayProperty->Inner, ArrayHelper.GetRawPtr(Index), ElementContext, OutRows);
		}
		return;
	}

	if (FSetProperty* SetProperty = CastField<FSetProperty>(Property))
	{
		FScriptSetHelper SetHelper(SetProperty, ValuePtr);
		for (FScriptSetHelper::FIterator It = SetHelper.CreateIterator(); It; ++It)
		{
			FTraversalContext ElementContext = Context;
			FHazardConfigReferencePathSegment Segment;
			Segment.Type = EHazardConfigReferencePathSegmentType::SetIndex;
			Segment.Index = It.GetInternalIndex();
			Segment.DisplayText = FString::FromInt(It.GetInternalIndex());
			ElementContext.Path.Segments.Add(Segment);
			ElementContext.Path.bWritable = false;
			ElementContext.Path.ReadOnlyReason = INVTEXT("Set entries are read-only because changing referenced values can invalidate the set hash.");
			TraverseProperty(SetProperty->ElementProp, SetHelper.GetElementPtr(It), ElementContext, OutRows);
		}
		return;
	}

	if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
	{
		FScriptMapHelper MapHelper(MapProperty, ValuePtr);
		for (FScriptMapHelper::FIterator It = MapHelper.CreateIterator(); It; ++It)
		{
			const int32 InternalIndex = It.GetInternalIndex();
			const FString KeyText = ExportPropertyValue(MapProperty->KeyProp, MapHelper.GetKeyPtr(It));

			FTraversalContext KeyContext = Context;
			FHazardConfigReferencePathSegment KeySegment;
			KeySegment.Type = EHazardConfigReferencePathSegmentType::MapKey;
			KeySegment.Index = InternalIndex;
			KeySegment.DisplayText = KeyText;
			KeyContext.Path.Segments.Add(KeySegment);
			KeyContext.Path.bWritable = false;
			KeyContext.Path.ReadOnlyReason = INVTEXT("Map keys are read-only because changing a key can invalidate the map hash.");
			TraverseProperty(MapProperty->KeyProp, MapHelper.GetKeyPtr(It), KeyContext, OutRows);

			FTraversalContext ValueContext = Context;
			FHazardConfigReferencePathSegment ValueSegment;
			ValueSegment.Type = EHazardConfigReferencePathSegmentType::MapValue;
			ValueSegment.Index = InternalIndex;
			ValueSegment.DisplayText = KeyText;
			ValueContext.Path.Segments.Add(ValueSegment);
			TraverseProperty(MapProperty->ValueProp, MapHelper.GetValuePtr(It), ValueContext, OutRows);
		}
		return;
	}
}

void FHazardConfigReferenceScanner::AddReferenceRow(FProperty* Property, void* ValuePtr, FTraversalContext Context, EHazardConfigReferenceKind Kind, UClass* ExpectedBaseClass, TArray<FHazardConfigReferenceRowPtr>& OutRows) const
{
	FHazardConfigReferenceRowPtr Row = MakeShared<FHazardConfigReferenceRow>();
	Row->SettingsObject = Context.SettingsObject;
	Row->SettingsClass = Context.SettingsClass;
	Row->DeclaringClass = Context.DeclaringClass ? Context.DeclaringClass : Context.SettingsClass;
	Row->ConfigName = Context.ConfigName;
	Row->ConfigFile = Context.ConfigFile;
	Row->ConfigSection = Context.ConfigSection;
	Row->ReferencePath = Context.Path;
	Row->ReferencePath.LeafProperty = Property;
	Row->ReferenceKind = Kind;
	Row->ExpectedBaseClass = ExpectedBaseClass ? ExpectedBaseClass : UObject::StaticClass();
	Row->CurrentPath = ReadReferencePath(Property, ValuePtr, Kind);
	Row->bProjectSettings = Context.bProjectSettings;
	Row->bPluginSettings = Context.bPluginSettings;
	Row->bEngineSettings = Context.bEngineSettings;
	if (!Row->ReferencePath.bWritable)
	{
		Row->Message = Row->ReferencePath.ReadOnlyReason;
	}
	OutRows.Add(Row);
}

bool FHazardConfigReferenceScanner::ShouldScanSettingsClass(const UClass* SettingsClass)
{
	if (!SettingsClass || !SettingsClass->IsChildOf(UDeveloperSettings::StaticClass()))
	{
		return false;
	}

	if (SettingsClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_Transient))
	{
		return false;
	}

	const FString Name = SettingsClass->GetName();
	return !Name.StartsWith(TEXT("SKEL_")) && !Name.StartsWith(TEXT("REINST_"));
}

bool FHazardConfigReferenceScanner::ShouldIncludeSettingsClass(const UClass* SettingsClass, const FHazardConfigReferenceScanOptions& Options, bool& bOutProject, bool& bOutPlugin, bool& bOutEngine)
{
	bOutProject = false;
	bOutPlugin = false;
	bOutEngine = false;

	if (!SettingsClass)
	{
		return false;
	}

	const FString PackageName = SettingsClass->GetOutermost()->GetName();
	bOutEngine = PackageName.StartsWith(TEXT("/Script/Engine")) || PackageName.StartsWith(TEXT("/Script/UnrealEd")) || PackageName.StartsWith(TEXT("/Engine/"));

	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(SettingsClass->GetPackage()->GetName().RightChop(8));
	bOutPlugin = Plugin.IsValid() || (!PackageName.StartsWith(TEXT("/Script/Hazard")) && !bOutEngine);
	bOutProject = !bOutPlugin && !bOutEngine;

	if (Options.bProjectSettingsOnly && !bOutProject)
	{
		return false;
	}
	if (bOutPlugin && !Options.bIncludePluginSettings)
	{
		return false;
	}
	if (bOutEngine && !Options.bIncludeEngineSettings)
	{
		return false;
	}
	return true;
}

UClass* FHazardConfigReferenceScanner::GetExpectedBaseClass(const FProperty* Property, const EHazardConfigReferenceKind Kind)
{
	if (const FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
	{
		return SoftClassProperty->MetaClass ? SoftClassProperty->MetaClass.Get() : UObject::StaticClass();
	}
	if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
	{
		return ClassProperty->MetaClass ? ClassProperty->MetaClass.Get() : UObject::StaticClass();
	}
	if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
	{
		return ObjectProperty->PropertyClass ? ObjectProperty->PropertyClass.Get() : UObject::StaticClass();
	}
	if (HazardTools::ConfigReferenceValidator::IsClassReferenceKind(Kind))
	{
		return UObject::StaticClass();
	}
	return UObject::StaticClass();
}

FString FHazardConfigReferenceScanner::ReadReferencePath(FProperty* Property, void* ValuePtr, const EHazardConfigReferenceKind Kind)
{
	if (!Property || !ValuePtr)
	{
		return FString();
	}

	switch (Kind)
	{
	case EHazardConfigReferenceKind::SoftObject:
	case EHazardConfigReferenceKind::SoftClass:
		if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
		{
			return SoftObjectProperty->GetPropertyValue(ValuePtr).ToSoftObjectPath().ToString();
		}
		break;
	case EHazardConfigReferenceKind::SoftObjectPath:
		return reinterpret_cast<FSoftObjectPath*>(ValuePtr)->ToString();
	case EHazardConfigReferenceKind::SoftClassPath:
		return reinterpret_cast<FSoftClassPath*>(ValuePtr)->ToString();
	case EHazardConfigReferenceKind::HardObject:
	case EHazardConfigReferenceKind::HardClass:
		if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			if (UObject* Object = ObjectProperty->GetObjectPropertyValue(ValuePtr))
			{
				return Object->GetPathName();
			}
		}
		break;
	default:
		break;
	}

	return HazardTools::ConfigReferenceValidator::SanitizeExportedReference(ExportPropertyValue(Property, ValuePtr));
}

FString FHazardConfigReferenceScanner::ExportPropertyValue(FProperty* Property, const void* ValuePtr)
{
	if (!Property || !ValuePtr)
	{
		return FString();
	}

	FString ExportedValue;
	Property->ExportTextItem_Direct(ExportedValue, ValuePtr, nullptr, nullptr, PPF_None);
	return ExportedValue;
}

bool FHazardConfigReferenceFixer::ApplyFix(const FHazardConfigReferenceRowPtr& Row, FText& OutError) const
{
	if (!Row.IsValid())
	{
		OutError = INVTEXT("Invalid row.");
		return false;
	}

	UObject* SettingsObject = Row->SettingsObject.Get();
	if (!SettingsObject)
	{
		OutError = INVTEXT("Settings object is no longer valid.");
		return false;
	}

	const FString ReplacementPath = Row->GetReplacementPathForApply();
	if (ReplacementPath.IsEmpty())
	{
		OutError = INVTEXT("No replacement path is selected.");
		return false;
	}

	if (!Row->IsWritable())
	{
		OutError = Row->ReferencePath.ReadOnlyReason.IsEmpty() ? INVTEXT("This row is read-only.") : Row->ReferencePath.ReadOnlyReason;
		return false;
	}

	void* ValuePtr = nullptr;
	FProperty* LeafProperty = nullptr;
	if (!Row->ReferencePath.Resolve(SettingsObject, ValuePtr, LeafProperty, OutError))
	{
		return false;
	}

	const FScopedTransaction Transaction(INVTEXT("Fix Config Reference"));
	SettingsObject->Modify();
	if (!WriteValue(Row, SettingsObject, ValuePtr, LeafProperty, ReplacementPath, OutError))
	{
		SettingsObject->ReloadConfig(SettingsObject->GetClass());
		return false;
	}

	SettingsObject->SaveConfig();
	SettingsObject->TryUpdateDefaultConfigFile();
	SettingsObject->ReloadConfig(SettingsObject->GetClass());
	return true;
}

bool FHazardConfigReferenceFixer::WriteValue(const FHazardConfigReferenceRowPtr& Row, UObject* SettingsObject, void* ValuePtr, FProperty* LeafProperty, const FString& ReplacementPath, FText& OutError) const
{
	if (!Row.IsValid() || !SettingsObject || !ValuePtr || !LeafProperty)
	{
		OutError = INVTEXT("Resolved property value is invalid.");
		return false;
	}

	if (Row->IsClassReference())
	{
		if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(LeafProperty))
		{
			SoftObjectProperty->SetPropertyValue(ValuePtr, FSoftObjectPtr(FSoftObjectPath(ReplacementPath)));
			return true;
		}
		if (FStructProperty* StructProperty = CastField<FStructProperty>(LeafProperty))
		{
			if (StructProperty->Struct == FSoftClassPath::StaticStruct())
			{
				*reinterpret_cast<FSoftClassPath*>(ValuePtr) = FSoftClassPath(ReplacementPath);
				return true;
			}
		}
		if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(LeafProperty))
		{
			UObject* ReplacementObject = LoadReplacementObject(Row, ReplacementPath, OutError);
			if (!ReplacementObject)
			{
				return false;
			}
			ObjectProperty->SetObjectPropertyValue(ValuePtr, ReplacementObject);
			return true;
		}
	}
	else
	{
		if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(LeafProperty))
		{
			SoftObjectProperty->SetPropertyValue(ValuePtr, FSoftObjectPtr(FSoftObjectPath(ReplacementPath)));
			return true;
		}
		if (FStructProperty* StructProperty = CastField<FStructProperty>(LeafProperty))
		{
			if (StructProperty->Struct == FSoftObjectPath::StaticStruct())
			{
				*reinterpret_cast<FSoftObjectPath*>(ValuePtr) = FSoftObjectPath(ReplacementPath);
				return true;
			}
		}
		if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(LeafProperty))
		{
			UObject* ReplacementObject = LoadReplacementObject(Row, ReplacementPath, OutError);
			if (!ReplacementObject)
			{
				return false;
			}
			ObjectProperty->SetObjectPropertyValue(ValuePtr, ReplacementObject);
			return true;
		}
	}

	const FString ImportText = ReplacementPath;
	if (LeafProperty->ImportText_Direct(*ImportText, ValuePtr, SettingsObject, PPF_None) != nullptr)
	{
		return true;
	}

	OutError = INVTEXT("Property type does not support reflected assignment for this replacement.");
	return false;
}

UObject* FHazardConfigReferenceFixer::LoadReplacementObject(const FHazardConfigReferenceRowPtr& Row, const FString& ReplacementPath, FText& OutError)
{
	if (!Row.IsValid())
	{
		OutError = INVTEXT("Invalid row.");
		return nullptr;
	}

	UObject* Object = nullptr;
	if (Row->IsClassReference())
	{
		Object = LoadClass<UObject>(nullptr, *ReplacementPath);
	}
	else
	{
		Object = StaticLoadObject(UObject::StaticClass(), nullptr, *ReplacementPath);
	}

	if (!Object)
	{
		OutError = FText::Format(INVTEXT("Could not load replacement '{0}'."), FText::FromString(ReplacementPath));
		return nullptr;
	}

	if (const UClass* ExpectedBaseClass = Row->ExpectedBaseClass.Get())
	{
		if (const UClass* Class = Cast<UClass>(Object))
		{
			if (!Class->IsChildOf(ExpectedBaseClass))
			{
				OutError = FText::Format(INVTEXT("Replacement class is not a child of '{0}'."), FText::FromString(ExpectedBaseClass->GetName()));
				return nullptr;
			}
		}
		else if (!Object->IsA(ExpectedBaseClass))
		{
			OutError = FText::Format(INVTEXT("Replacement object is not a '{0}'."), FText::FromString(ExpectedBaseClass->GetName()));
			return nullptr;
		}
	}

	return Object;
}
