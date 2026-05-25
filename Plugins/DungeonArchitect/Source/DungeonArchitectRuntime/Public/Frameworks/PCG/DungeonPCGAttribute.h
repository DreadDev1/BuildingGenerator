//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Metadata/PCGMetadata.h"
#include "Metadata/PCGMetadataAttributeTpl.h"
#include "PCGData.h"

template<typename TData>
class FDAPCGAttribute {
public:
	FDAPCGAttribute(const TCHAR* InName, TData InDefaultValue, bool bInAllowInterpolation, bool bInOverrideParent, bool bInOverwriteIfTypeMismatch)
		: Name(InName)
		, DefaultValue(InDefaultValue)
		, bAllowInterpolation(bInAllowInterpolation)
		, bOverrideParent(bInOverrideParent)
		, bOverwriteIfTypeMismatch(bInOverwriteIfTypeMismatch)
	{}

	FPCGMetadataAttribute<TData>* FindOrAdd(UPCGData* Data) const {
		if (Data) {
			if (UPCGMetadata* Metadata = Data->MutableMetadata()) {
				return Metadata->FindOrCreateAttribute<TData>(Name, DefaultValue, bAllowInterpolation, bOverrideParent, bOverwriteIfTypeMismatch);
			}
		}
		return nullptr;
	}

	void Create(UPCGData* Data) const {
		if (Data) {
			if (UPCGMetadata* Metadata = Data->MutableMetadata()) {
				Metadata->CreateAttribute<TData>(Name, DefaultValue, bAllowInterpolation, bOverrideParent, bOverwriteIfTypeMismatch);
			}
		}
	}
	
	FPCGMetadataAttribute<TData>* FindOrAdd(UPCGMetadata* Metadata) const {
		if (Metadata) {
			return Metadata->FindOrCreateAttribute<TData>(Name, DefaultValue, bAllowInterpolation, bOverrideParent, bOverwriteIfTypeMismatch);
		}
		return nullptr;
	}
	
	FPCGMetadataAttribute<TData>* Get(UPCGData* Data) const {
		if (Data) {
			if (UPCGMetadata* Metadata = Data->MutableMetadata()) {
				return Metadata->GetMutableTypedAttribute<TData>(Name);
			}
		}
		return nullptr;
	}

	FPCGMetadataAttribute<TData>* Get(UPCGMetadata* Metadata) const {
		if (Metadata) {
			return Metadata->GetMutableTypedAttribute<TData>(Name);
		}
		return nullptr;
	}
	

	const FPCGMetadataAttribute<TData>* GetConst(const UPCGData* Data) const {
		if (Data) {
			if (const UPCGMetadata* Metadata = Data->ConstMetadata()) {
				return Metadata->GetConstTypedAttribute<TData>(Name);
			}
		}
		return nullptr;
	}

	FName GetAttributeName() const { return Name; }
	bool HasAttribute(const UPCGMetadata* Metadata) const {
		return Metadata && Metadata->HasAttribute(Name);
	}

	void Create(UPCGMetadata* Metadata) const {
		if (!HasAttribute(Metadata)) {
			Metadata->CreateAttribute<TData>(Name, DefaultValue, bAllowInterpolation, bOverrideParent);
		}
	}
	
private:
	FName Name;
	TData DefaultValue;
	bool bAllowInterpolation;
	bool bOverrideParent;
	bool bOverwriteIfTypeMismatch;
};

struct DUNGEONARCHITECTRUNTIME_API FDungeonPCGAttributes {
	static const FDAPCGAttribute<int32> ID;
	static const FDAPCGAttribute<FName> MarkerName;
	static const FDAPCGAttribute<float> SDF;
	static const FDAPCGAttribute<float> GroundHeight;
};

