//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/PCG/Data/PCGSGFTextureData.h"

#include "Core/Utils/HeatmapColorRamp.h"
#include "Frameworks/Canvas/DungeonCanvasRendering.h"
#include "Frameworks/Lib/Shaders/ReadbackRTShader.h"
#include "Frameworks/PCG/DungeonPCGAttribute.h"

#include "Data/PCGPointData.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Helpers/PCGAsync.h"
#include "Helpers/PCGHelpers.h"
#include "RHIStaticStates.h"
#include "TextureResource.h"

namespace DungeonPCGSDFSamplerHelpers {

	template<typename ValueType>
	ValueType SampleInternal(FVector2D PositionLocalSpace,
		int32 Width,
		int32 Height,
		EPCGTextureFilter Filter,
		TFunctionRef<ValueType(int32 Index)> SamplingFunction)
	{
		const double TexelX = PositionLocalSpace.X * Width;
		const double TexelY = PositionLocalSpace.Y * Height;

		ValueType Result{};

		if (Filter == EPCGTextureFilter::Point)
		{
			const int32 X = FMath::Clamp(FMath::FloorToInt(TexelX), 0, Width - 1);
			const int32 Y = FMath::Clamp(FMath::FloorToInt(TexelY), 0, Height - 1);

			Result = SamplingFunction(X + Y * Width);
		}
		else if (Filter == EPCGTextureFilter::Bilinear)
		{
			// Accounts for texel values being at texel centers
			const double TexelXOffset = TexelX - 0.5;
			const double TexelYOffset = TexelY - 0.5;

			const int32 X0 = FMath::Clamp(FMath::FloorToInt(TexelXOffset), 0, Width - 1);
			const int32 X1 = FMath::Min(X0 + 1, Width - 1);
			const int32 Y0 = FMath::Clamp(FMath::FloorToInt(TexelYOffset), 0, Height - 1);
			const int32 Y1 = FMath::Min(Y0 + 1, Height - 1);

			const ValueType SampleX0Y0 = SamplingFunction(X0 + Y0 * Width);
			const ValueType SampleX1Y0 = SamplingFunction(X1 + Y0 * Width);
			const ValueType SampleX0Y1 = SamplingFunction(X0 + Y1 * Width);
			const ValueType SampleX1Y1 = SamplingFunction(X1 + Y1 * Width);

			Result = FMath::BiLerp(SampleX0Y0, SampleX1Y0, SampleX0Y1, SampleX1Y1, TexelXOffset - X0, TexelYOffset - Y0);
		}
		else
		{
			ensureMsgf(false, TEXT("Unrecognized PCG texture filtering mode."));
		}

		return Result;
	}
	
	template<typename ValueType>
	bool Sample(const FVector2D& InPosition,
		const FBox2D& InSurface,
		const UPCGBaseTextureData* InTextureData,
		int32 Width,
		int32 Height,
		ValueType& SampledValue,
		TFunctionRef<ValueType(int32 Index)> SamplingFunction)
	{
		check(Width > 0 && Height > 0);
		if (Width <= 0 || Height <= 0 || InSurface.GetSize().SquaredLength() <= 0)
		{
			return false;
		}

		check(InTextureData);

		const FVector2D LocalSpacePos = (InPosition - InSurface.Min) / InSurface.GetSize();
		FVector2D Pos = FVector2D::ZeroVector;
		if (!InTextureData->bUseAdvancedTiling)
		{
			Pos.X = FMath::Clamp(LocalSpacePos.X, 0.0, 1.0);
			Pos.Y = FMath::Clamp(LocalSpacePos.Y, 0.0, 1.0);
		}
		else
		{
			// Conceptually, we are building "tiles" in texture space with the origin being in the middle of the [0, 0] tile.
			// The offset is given in a ratio of [0, 1], applied "before" scaling & rotation.
			// Rotation is done around the center given, where the center is (0.5, 0.5) + offset
			// Scaling controls the horizon of tiles, and the tile selection is done through min-max bounds, in tile space,
			// with the origin tile being from -0.5 to 0.5.
			const FRotator Rotation = FRotator(0.0, -InTextureData->Rotation, 0.0);
			FVector Scale = FVector(InTextureData->Tiling, 1.0);
			Scale.X = ((FMath::Abs(Scale.X) > SMALL_NUMBER) ? (1.0 / Scale.X) : 0.0);
			Scale.Y = ((FMath::Abs(Scale.Y) > SMALL_NUMBER) ? (1.0 / Scale.Y) : 0.0);
			const FVector Translation = FVector(0.5 + InTextureData->CenterOffset.X, 0.5 + InTextureData->CenterOffset.Y, 0);

			FTransform Transform = FTransform(Rotation, Translation, Scale);

			// Transform to tile-space
			const FVector2D SamplePosition = FVector2D(Transform.InverseTransformPosition(FVector(LocalSpacePos, 0.f)));

			if (InTextureData->bUseTileBounds && !InTextureData->TileBounds.IsInsideOrOn(SamplePosition))
			{
				return false;
			}

			FVector::FReal X = FMath::Frac(SamplePosition.X + 0.5);
			FVector::FReal Y = FMath::Frac(SamplePosition.Y + 0.5);

			Pos = FVector2D(X, Y);
		}

		SampledValue = SampleInternal(Pos, Width, Height, InTextureData->Filter, SamplingFunction);
		return true;
	}

	float SampleFloatChannel(const FLinearColor& InColor, EPCGTextureColorChannel ColorChannel)
	{
		switch (ColorChannel)
		{
		case EPCGTextureColorChannel::Red:
			return InColor.R;
		case EPCGTextureColorChannel::Green:
			return InColor.G;
		case EPCGTextureColorChannel::Blue:
			return InColor.B;
		case EPCGTextureColorChannel::Alpha:
		default:
			return InColor.A;
		}
	}
}

const UPCGPointData* UPCGDungeonSGFTextureData::CreatePointData(FPCGContext* Context) const {
	UPCGPointData* Data = NewObject<UPCGPointData>();
	Data->InitializeFromData(this);

	// Early out for invalid data
	if (!IsValid())
	{
		UE_LOG(LogPCG, Error, TEXT("Texture data does not have valid sizes - will return empty data"));
		return Data;
	}

	TArray<FPCGPoint>& Points = Data->GetMutablePoints();

	// Map target texel size to the current physical size of the texture data.
	const FVector::FReal XSize = Width;
	const FVector::FReal YSize = Height;

	const int32 XCount = FMath::Floor(XSize / TexelSize);
	const int32 YCount = FMath::Floor(YSize / TexelSize);
	const int32 PointCount = XCount * YCount;

	if (PointCount <= 0)
	{
		UE_LOG(LogPCG, Warning, TEXT("Texture data has a texel size larger than its data - will return empty data"));
		return Data;
	}

	const FBox2D Surface(FVector2D(-1.0f, -1.0f), FVector2D(1.0f, 1.0f));

	float BoundsSize = TexelSize * FMath::Min(
		Transform.GetScale3D().X / Width,
		Transform.GetScale3D().Y / Height);

	BoundsSize *= 0.9f;	// Make the pixel a bit smaller to visualize individual pixels
	
	FPCGMetadataAttribute<float>* AttributeSDF = FDungeonPCGAttributes::SDF.FindOrAdd(Data);
	FPCGMetadataAttribute<float>* AttributeGroundHeight = FDungeonPCGAttributes::GroundHeight.FindOrAdd(Data);
	
	FPCGAsync::AsyncPointProcessing(Context, PointCount, Points, [this, XCount, YCount, &Surface, BoundsSize, AttributeSDF, AttributeGroundHeight, Data](int32 Index, FPCGPoint& OutPoint)
	{
		const int X = (Index % XCount);
		const int Y = (Index / YCount);

		FVector2D LocalCoordinate((2.0 * X + 0.5) / XCount - 1.0, (2.0 * Y + 0.5) / YCount - 1.0);
		FPCGDungeonSDFTexelData Value{};

		if (DungeonPCGSDFSamplerHelpers::Sample<FPCGDungeonSDFTexelData>(LocalCoordinate, Surface, this, Width, Height, Value, [this](int32 Index) { return Texels[Index]; }))
		{
			constexpr float Density = 1.0f;
			
			FVector LocalPosition(LocalCoordinate, 0);
			FVector WorldPosition = Transform.TransformPosition(LocalPosition);
			WorldPosition.Z = Value.GroundHeight;
				
			OutPoint = FPCGPoint(FTransform(WorldPosition),
				Density,
				PCGHelpers::ComputeSeed(X, Y));

			OutPoint.SetExtents(FVector(TexelSize / 2.0));
			OutPoint.Color = FLinearColor::White;				// TODO: Put a heat map of sorts with a color ramp
			OutPoint.BoundsMin = FVector(-BoundsSize);
			OutPoint.BoundsMax = FVector(BoundsSize);
			OutPoint.MetadataEntry = Data->Metadata->AddEntry();
			
			if (HeatmapColorRamp) {
				OutPoint.Color = HeatmapColorRamp->GetColorForDistance(Value.SDF);
			} else {
				OutPoint.Color = FLinearColor::White;
			}

			AttributeSDF->SetValue(OutPoint.MetadataEntry, Value.SDF);
			AttributeGroundHeight->SetValue(OutPoint.MetadataEntry, Value.GroundHeight);

			return true;
		}

		return false;
	});

	return Data;
}

bool UPCGDungeonSGFTextureData::ReadGPUTexture(UTextureRenderTarget2D* InTexture, const TFunction<void(int32, const FFloat16&)>& SetValueCallback, const TFunction<void()>& PostInitializeCallback) {
	if (!InTexture) {
		return false;
	}
	
	FTextureResource* TextureResource = InTexture->GetResource();
	if (TextureResource && TextureResource->TextureRHI)
	{
		FDATextureReadbackDispatchParams Params;
		Params.SourceTexture = TextureResource->TextureRHI;

		check(TextureResource->TextureRHI->GetFormat() == EPixelFormat::PF_R16F);
		
		// We should always use a point filter sampler since we are trying to get a 1 to 1 copy of the texture. We will do our own filtering later.
		Params.SourceSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		Params.SourceDimensions = FIntPoint(InTexture->SizeX, InTexture->SizeY);
		Params.PixelFormat = TextureResource->TextureRHI->GetFormat();
		
		FDATextureReadbackInterface::Dispatch(Params, [this, SetValueCallback, PostInitializeCallback](const void* OutBuffer, int32 ReadbackWidth, int32 ReadbackHeight) {
			TRACE_CPUPROFILER_EVENT_SCOPE(UPCGTextureData::Initialize::DispatchCallback);

			const int32 PixelCount = ReadbackWidth * ReadbackHeight;

			if (const FFloat16* FormattedImageData = static_cast<const FFloat16*>(OutBuffer)) {
				Width = ReadbackWidth;
				Height = ReadbackHeight;
				Texels.SetNum(PixelCount);

				for (int32 D = 0; D < PixelCount; ++D) {
					SetValueCallback(D, FormattedImageData[D]);
				}
			}
			else {
				UE_LOG(LogPCG, Error, TEXT("PCGTextureData unable to get readback results from texture"));
			}

			PostInitializeCallback();
		});
	}
	else {
		UE_LOG(LogPCG, Error, TEXT("PCGTextureData failed to acquire texture resource for texture"));
		return false;
	}
	return true;
}


void UPCGDungeonSGFTextureData::Initialize(UTextureRenderTarget2D* InTextureSDF, UTextureRenderTarget2D* InTextureHeight, const FTransform& InTransform,
	float GroundHeightMin, float GroundHeightMax, float GroundHeightBias, const TFunction<void()>& PostInitializeCallback)
{
	Transform = InTransform;
	
	Texels.Reset();
	ColorData.Reset();

	if (!InTextureSDF || !InTextureHeight || InTextureSDF->SizeX != InTextureHeight->SizeX || InTextureSDF->SizeY != InTextureHeight->SizeY) {
		UE_LOG(LogPCG, Error, TEXT("Failed to read SDF / Height texture. Incompatible dimensions"));
		PostInitializeCallback();
		return;
	}
	
	if (InTextureSDF)
	{
		Width = InTextureSDF->SizeX;
		Height = InTextureSDF->SizeY;
		FVector FullDungeonCanvasScale = Transform.GetScale3D() * 2;

		bSDFTexReadComplete = false;
		bHeightTexReadComplete = false;
		auto SDFTexReadCompleteCallback = [this, PostInitializeCallback]() {
			bSDFTexReadComplete = true;
			if (bSDFTexReadComplete && bHeightTexReadComplete) {
				PostInitializeCallback();
			}
		};
		auto HeightTexReadCompleteCallback = [this, PostInitializeCallback]() {
			bHeightTexReadComplete = true;
			if (bSDFTexReadComplete && bHeightTexReadComplete) {
				PostInitializeCallback();
			}
		};

		auto SetSDFValue = [this, FullDungeonCanvasScale](int32 Index, const FFloat16& NormalizedSDFValue) {
			// Flip the sign as we want positive SDF values inside the dungeon
			Texels[Index].SDF = FDungeonCanvasRenderingLibrary::ConvertLocalSDFValueToWorldUnits(-NormalizedSDFValue, FullDungeonCanvasScale);
		};

		// Create the attributes
		FDungeonPCGAttributes::SDF.Create(Metadata);
		FDungeonPCGAttributes::GroundHeight.Create(Metadata);
		
		if (!ReadGPUTexture(InTextureSDF, SetSDFValue, SDFTexReadCompleteCallback)) {
			UE_LOG(LogPCG, Error, TEXT("Failed to read SDF texture"));
			SDFTexReadCompleteCallback();
		}

		auto SetHeightValue = [this, GroundHeightMin, GroundHeightMax, GroundHeightBias](int32 Index, const FFloat16& TexValue) {
			float GroundHeight{};
			if (TexValue <= GroundHeightBias) {
				GroundHeight = GroundHeightMin;
			}
			else { 
				float UnbiasedAlpha = (TexValue - GroundHeightBias) / (1.0f - GroundHeightBias);
				UnbiasedAlpha = FMath::Clamp(UnbiasedAlpha, 0.0f, 1.0f);
				GroundHeight = UnbiasedAlpha * (GroundHeightMax - GroundHeightMin) + GroundHeightMin;
			}

			Texels[Index].GroundHeight = GroundHeight;
		};

		if (!ReadGPUTexture(InTextureHeight, SetHeightValue, HeightTexReadCompleteCallback)) {
			UE_LOG(LogPCG, Error, TEXT("Failed to read Height texture"));
			HeightTexReadCompleteCallback();
		}
	}
	else
	{
		Width = 0;
		Height = 0;
	}

	Bounds = FBox(EForceInit::ForceInit);
	Bounds += FVector(-1.0f, -1.0f, 0.0f);
	Bounds += FVector(1.0f, 1.0f, 0.0f);
	Bounds = Bounds.TransformBy(Transform);
}

void UPCGDungeonSGFTextureData::AddToCrc(FArchiveCrc32& Ar, bool bFullDataCrc) const
{
	Super::AddToCrc(Ar, bFullDataCrc);

	// This data does not have a bespoke CRC implementation so just use a global unique data CRC.
	AddUIDToCrc(Ar);
}

bool UPCGDungeonSGFTextureData::SamplePoint(const FTransform& InTransform, const FBox& InBounds, FPCGPoint& OutPoint, UPCGMetadata* OutMetadata) const {
	if (!IsValid()) {
		return false;
	}

	// Compute transform
	OutPoint.Transform = InTransform;
	FVector PointPositionInLocalSpace = Transform.InverseTransformPosition(InTransform.GetLocation());
	OutPoint.Transform.SetLocation(Transform.TransformPosition(PointPositionInLocalSpace));
	OutPoint.SetLocalBounds(InBounds);

	// Compute density & color (& metadata)
	FVector2D Position2D(PointPositionInLocalSpace.X, PointPositionInLocalSpace.Y);
	FBox2D Surface(FVector2D(-1.0f, -1.0f), FVector2D(1.0f, 1.0f));

	FPCGDungeonSDFTexelData TexelData{};
	if (DungeonPCGSDFSamplerHelpers::Sample<FPCGDungeonSDFTexelData>(Position2D, Surface, this, Width, Height, TexelData, [this](int32 Index) { return Texels[Index]; })) {
		if (HeatmapColorRamp) {
			OutPoint.Color = HeatmapColorRamp->GetColorForDistance(TexelData.SDF);
		} else {
			OutPoint.Color = FLinearColor::White;
		}
		OutPoint.Density = 1.0f;
		if (OutPoint.MetadataEntry == -1) {
			OutPoint.MetadataEntry = OutMetadata->AddEntry();
		}
		
		if (FPCGMetadataAttribute<float>* AttributeSDF = FDungeonPCGAttributes::SDF.FindOrAdd(OutMetadata)) {
			AttributeSDF->SetValue(OutPoint.MetadataEntry, TexelData.SDF);
		}
		if (FPCGMetadataAttribute<float>* AttributeHeight = FDungeonPCGAttributes::GroundHeight.FindOrAdd(OutMetadata)) {
			AttributeHeight->SetValue(OutPoint.MetadataEntry, TexelData.GroundHeight);
		}

		FVector PointLocation = OutPoint.Transform.GetLocation();
		PointLocation.Z = TexelData.GroundHeight;
		OutPoint.Transform.SetLocation(PointLocation);
		
		return OutPoint.Density > 0 || bKeepZeroDensityPoints;
	}
	else
	{
		return false;
	}
}

bool UPCGDungeonSGFTextureData::IsValid() const {
	return Height > 0 && Width > 0 && !Texels.IsEmpty();
}

UPCGSpatialData* UPCGDungeonSGFTextureData::CopyInternal() const
{
	UPCGDungeonSGFTextureData* NewRenderTargetData = NewObject<UPCGDungeonSGFTextureData>();

	CopyBaseTextureData(NewRenderTargetData);
	NewRenderTargetData->Texels = Texels;
	NewRenderTargetData->HeatmapColorRamp = HeatmapColorRamp;

	return NewRenderTargetData;
}

