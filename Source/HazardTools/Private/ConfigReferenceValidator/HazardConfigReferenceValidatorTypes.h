// Copyright Neyl Sullivan 2026

#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "UObject/SoftObjectPath.h"

enum class EHazardConfigReferenceStatus : uint8
{
	Valid,
	Invalid,
	Empty,
	Redirected,
	Unsupported,
	PendingFix,
	SaveFailed
};

enum class EHazardConfigReferenceKind : uint8
{
	SoftObject,
	SoftClass,
	SoftObjectPath,
	SoftClassPath,
	HardObject,
	HardClass,
	Unsupported
};

enum class EHazardConfigReferencePathSegmentType : uint8
{
	Property,
	ArrayIndex,
	SetIndex,
	MapKey,
	MapValue
};

struct FHazardConfigReferencePathSegment
{
	EHazardConfigReferencePathSegmentType Type = EHazardConfigReferencePathSegmentType::Property;
	FProperty* Property = nullptr;
	int32 Index = INDEX_NONE;
	FString DisplayText;
};

struct FHazardConfigReferencePath
{
	TArray<FHazardConfigReferencePathSegment> Segments;
	FProperty* LeafProperty = nullptr;
	bool bWritable = true;
	FText ReadOnlyReason;

	FString ToString() const;
	bool Resolve(UObject* Owner, void*& OutValuePtr, FProperty*& OutLeafProperty, FText& OutError) const;
};

struct FHazardConfigReferenceRow
{
	TWeakObjectPtr<UObject> SettingsObject;
	TWeakObjectPtr<UClass> SettingsClass;
	TWeakObjectPtr<UClass> DeclaringClass;
	FString ConfigName;
	FString ConfigFile;
	FString ConfigSection;
	FHazardConfigReferencePath ReferencePath;
	EHazardConfigReferenceKind ReferenceKind = EHazardConfigReferenceKind::Unsupported;
	EHazardConfigReferenceStatus Status = EHazardConfigReferenceStatus::Unsupported;
	TWeakObjectPtr<UClass> ExpectedBaseClass;
	FString CurrentPath;
	FString ResolvedObjectPath;
	FString SuggestedReplacementPath;
	FString PendingReplacementPath;
	FText Message;
	bool bProjectSettings = false;
	bool bPluginSettings = false;
	bool bEngineSettings = false;

	bool IsClassReference() const;
	bool IsWritable() const;
	bool HasPendingReplacement() const;
	bool IsFixable() const;
	FString GetStatusString() const;
	FString GetReferenceKindString() const;
	FString GetSettingsClassName() const;
	FString GetDeclaringClassName() const;
	FString GetReplacementPathForApply() const;
	FText GetToolTipText() const;
};

using FHazardConfigReferenceRowPtr = TSharedPtr<FHazardConfigReferenceRow>;

struct FHazardConfigReferenceScanOptions
{
	bool bProjectSettingsOnly = false;
	bool bIncludePluginSettings = true;
	bool bIncludeEngineSettings = false;
};

class FHazardConfigReferenceValidator
{
public:
	void Validate(const FHazardConfigReferenceRowPtr& Row) const;
	bool TryResolveRedirect(const FHazardConfigReferenceRowPtr& Row) const;
	static FString NormalizeClassPathFromAsset(const FAssetData& AssetData, const UClass* ExpectedBaseClass);
	static FString GetPathForClass(const UClass* Class);

private:
	bool DoesAssetExist(const FString& Path, const FHazardConfigReferenceRow& Row, FString& OutResolvedPath, FString& OutMessage) const;
	bool DoesNativeClassExist(const FString& Path, const UClass* ExpectedBaseClass, FString& OutResolvedPath, FString& OutMessage) const;
	static FString GetBlueprintAssetPathFromGeneratedClassPath(const FString& Path);
};

class FHazardConfigReferenceScanner
{
public:
	void Scan(const FHazardConfigReferenceScanOptions& Options, TArray<FHazardConfigReferenceRowPtr>& OutRows) const;

private:
	struct FTraversalContext
	{
		UObject* SettingsObject = nullptr;
		UClass* SettingsClass = nullptr;
		UClass* DeclaringClass = nullptr;
		FString ConfigName;
		FString ConfigFile;
		FString ConfigSection;
		FHazardConfigReferencePath Path;
		bool bProjectSettings = false;
		bool bPluginSettings = false;
		bool bEngineSettings = false;
	};

	void ScanSettingsClass(UClass* SettingsClass, const FHazardConfigReferenceScanOptions& Options, TArray<FHazardConfigReferenceRowPtr>& OutRows) const;
	void TraverseProperty(FProperty* Property, void* ValuePtr, FTraversalContext Context, TArray<FHazardConfigReferenceRowPtr>& OutRows) const;
	void AddReferenceRow(FProperty* Property, void* ValuePtr, FTraversalContext Context, EHazardConfigReferenceKind Kind, UClass* ExpectedBaseClass, TArray<FHazardConfigReferenceRowPtr>& OutRows) const;
	static bool ShouldScanSettingsClass(const UClass* SettingsClass);
	static bool ShouldIncludeSettingsClass(const UClass* SettingsClass, const FHazardConfigReferenceScanOptions& Options, bool& bOutProject, bool& bOutPlugin, bool& bOutEngine);
	static UClass* GetExpectedBaseClass(const FProperty* Property, EHazardConfigReferenceKind Kind);
	static FString ReadReferencePath(FProperty* Property, void* ValuePtr, EHazardConfigReferenceKind Kind);
	static FString ExportPropertyValue(FProperty* Property, const void* ValuePtr);
};

class FHazardConfigReferenceFixer
{
public:
	bool ApplyFix(const FHazardConfigReferenceRowPtr& Row, FText& OutError) const;

private:
	bool WriteValue(const FHazardConfigReferenceRowPtr& Row, UObject* SettingsObject, void* ValuePtr, FProperty* LeafProperty, const FString& ReplacementPath, FText& OutError) const;
	static UObject* LoadReplacementObject(const FHazardConfigReferenceRowPtr& Row, const FString& ReplacementPath, FText& OutError);
};
