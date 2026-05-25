//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Data/PCGTextureData.h"
#include "PCGSGFTextureData.generated.h"

class UTextureRenderTarget2D;
class UHeatmapColorRamp;

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FPCGDungeonSDFTexelData {
	GENERATED_BODY()
	UPROPERTY()
	float SDF = 0;
	
	UPROPERTY()
	float GroundHeight = 0;

	FPCGDungeonSDFTexelData operator+(const FPCGDungeonSDFTexelData& Other) const {
		FPCGDungeonSDFTexelData Result;
		Result.SDF = this->SDF + Other.SDF;
		Result.GroundHeight = this->GroundHeight + Other.GroundHeight;
		return Result;
	}

	FPCGDungeonSDFTexelData operator-(const FPCGDungeonSDFTexelData& Other) const {
		FPCGDungeonSDFTexelData Result;
		Result.SDF = this->SDF - Other.SDF;
		Result.GroundHeight = this->GroundHeight - Other.GroundHeight;
		return Result;
	}

	FPCGDungeonSDFTexelData operator*(float Scalar) const {
		FPCGDungeonSDFTexelData Result;
		Result.SDF = this->SDF * Scalar;
		Result.GroundHeight = this->GroundHeight * Scalar;
		return Result;
	}
	
	FPCGDungeonSDFTexelData operator/(float Scalar) const {
		if (FMath::IsNearlyZero(Scalar)) {
			UE_LOG(LogTemp, Warning, TEXT("Division by zero or near-zero in FPCGDungeonSDFTexelData division"));
			return *this;
		}
		FPCGDungeonSDFTexelData Result;
		Result.SDF = this->SDF / Scalar;
		Result.GroundHeight = this->GroundHeight / Scalar;
		return Result;
	}
	
	friend FPCGDungeonSDFTexelData operator*(float Scalar, const FPCGDungeonSDFTexelData& Data) {
		return Data * Scalar;
	}
};


UCLASS(BlueprintType)
class DUNGEONARCHITECTRUNTIME_API UPCGDungeonSGFTextureData  : public UPCGBaseTextureData
{
	GENERATED_BODY()

public:
	// ~Begin UPCGData interface
	virtual EPCGDataType GetDataType() const override { return EPCGDataType::Texture; }
	virtual void AddToCrc(FArchiveCrc32& Ar, bool bFullDataCrc) const override;
	virtual bool SamplePoint(const FTransform& Transform, const FBox& Bounds, FPCGPoint& OutPoint, UPCGMetadata* OutMetadata) const override;
	virtual bool IsValid() const override;
	// ~End UPCGData interface


	//~Begin UPCGSpatialData interface
protected:
	virtual UPCGSpatialData* CopyInternal() const override;
	virtual const UPCGPointData* CreatePointData(FPCGContext* Context) const override;
	//~End UPCGSpatialData interface

	bool ReadGPUTexture(UTextureRenderTarget2D* InTexture, const TFunction<void(int32, const FFloat16&)>& SetValueCallback, const TFunction<void()>& PostInitializeCallback);
	
public:
	void Initialize(UTextureRenderTarget2D* InTextureSDF, UTextureRenderTarget2D* InTextureHeight, const FTransform& InTransform,
			float GroundHeightMin, float GroundHeightMax, float GroundHeightBias, const TFunction<void()>& PostInitializeCallback);

public:
	UPROPERTY()
	TObjectPtr<UHeatmapColorRamp> HeatmapColorRamp;
	
protected:
	UPROPERTY()
	TArray<FPCGDungeonSDFTexelData> Texels;

	bool bSDFTexReadComplete = false;
	bool bHeightTexReadComplete = false;
};


