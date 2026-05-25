//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Voxel/Utils/VoxelLog.h"

#include "Misc/Compression.h"
#include "Serialization/BulkData.h"

namespace DA {
	template<typename TData>
	class TChunkDensityData {
	public:
		void Initialize(int32 InNumCellsPerChunk, const FIntVector& InOrigin, int32 InLOD, bool bInIsRootNode, int32 InPadding = 1)
		{
			NumCellsPerChunk = InNumCellsPerChunk;
			Origin = InOrigin;
			LOD = InLOD;
			bIsRootNode = bInIsRootNode;
			IdxMin = -InPadding;
			IdxMax = NumCellsPerChunk + InPadding;
			SizePerSide = IdxMax - IdxMin + 1;
			
			SizePerSideSquared = SizePerSide * SizePerSide; 
			Data.AddUninitialized(SizePerSideSquared * SizePerSide);
		}

		FORCEINLINE TData Get(int32 Index) const {
			return Data[Index];
		}

		FORCEINLINE TData Get(const FIntVector& P) const {
			int32 Index = GetCoordToIndex(P);
			return Data[Index];
		}

		FORCEINLINE void Set(int32 Index, TData Value) {
			Data[Index] = Value;
		}
	
		FORCEINLINE void Set(const FIntVector& P, TData Value) {
			int32 Index = GetCoordToIndex(P);
			Data[Index] = Value;
		}

		FORCEINLINE FIntVector GetCoord(int32 Index) const {
			FIntVector Coord;
			Coord.Z = Index / SizePerSideSquared;
			const int32 Rem = Index % SizePerSideSquared;
			Coord.Y = Rem / SizePerSide;
			Coord.X = Rem % SizePerSide;
			return Coord;
		}

		FORCEINLINE int32 GetMinIndex() const { return IdxMin; }
		FORCEINLINE int32 GetMaxIndex() const { return IdxMax; }
		FORCEINLINE int32 GetNumCellsPerChunk() const { return NumCellsPerChunk; }
		FORCEINLINE FIntVector GetOrigin() const { return Origin; }
		FORCEINLINE int32 GetLOD() const { return LOD; }
		FORCEINLINE bool IsRootNode() const { return bIsRootNode; }
		FORCEINLINE TArray<TData>& GetData() { return Data; }	
		FORCEINLINE const TArray<TData>& GetData() const { return Data; }
	
	private:
		FORCEINLINE int32 GetCoordToIndex(const FIntVector& P) const {
			const int32 X = P.X - IdxMin;
			const int32 Y = P.Y - IdxMin;
			const int32 Z = P.Z - IdxMin;
			check(X >= 0 && X < SizePerSide && Y >= 0 && Y < SizePerSide && Z >= 0 && Z < SizePerSide);
			return Z * SizePerSideSquared + Y * SizePerSide + X;
		}
	
	private:
		int32 NumCellsPerChunk = 0;
		FIntVector Origin;
		int32 LOD = INDEX_NONE;
		bool bIsRootNode = false;
		int32 IdxMin = INDEX_NONE;
		int32 IdxMax = INDEX_NONE;
		int32 SizePerSide = 0;
		int32 SizePerSideSquared = 0;
		TArray<TData> Data;
	};


	struct FChunkDensityDBEntry {
		bool bDensityLoaded = false;
		bool bDensityCompressed = false;
		size_t DensityCompressedSize = 0;
		FByteBulkData DensityData;
	};

	template<typename TData>
	class TChunkDensityDB {
	public:
		using Key_t = uint32;

		FORCEINLINE FChunkDensityDBEntry& GetEntry(Key_t InKey) {
			FScopeLock Lock(&EntriesCriticalSection);
			return Entries.FindOrAdd(InKey);
		}

		FORCEINLINE void Clear() {
			FScopeLock Lock(&EntriesCriticalSection);
			Entries.Reset();
		}

		void Load(Key_t InKey, TChunkDensityData<TData>& InData) {
			TArray<uint8> CompressedData;
			TArray<TData>& ChunkDataArray = InData.GetData();
			const int32 SizeInBytes = ChunkDataArray.Num() * sizeof(TData);
		
			{
				FScopeLock Lock(&EntriesCriticalSection);	// bulk data thread-safe access
				const FChunkDensityDBEntry& Entry = Entries.FindOrAdd(InKey);
				check(Entry.bDensityLoaded == true);
			
				const void* Buffer = Entry.DensityData.LockReadOnly();
				if (!Entry.bDensityCompressed) {
					FMemory::Memcpy(ChunkDataArray.GetData(), Buffer, SizeInBytes);
					Entry.DensityData.Unlock();
					return;
				}
				else {
					CompressedData.SetNumUninitialized(Entry.DensityCompressedSize);
					FMemory::Memcpy(CompressedData.GetData(), Buffer, Entry.DensityCompressedSize);
					Entry.DensityData.Unlock();
				}
			}

			// Data is compressed.  Decompress it
			if (!FCompression::UncompressMemory(NAME_Zlib, ChunkDataArray.GetData(), SizeInBytes, CompressedData.GetData(), CompressedData.Num()))
			{
				UE_LOG(LogDAVoxel, Error, TEXT("Failed to decompress chunk"));
				check(false);
			}
		}

		void Save(Key_t InKey, const TChunkDensityData<TData>& InData) {
			TArray<uint8> CompressedData;
			const TArray<TData>& UnCompressedData = InData.GetData();
			const int32 OriginalSize = UnCompressedData.Num() * sizeof(TData);
		
			// It is possible for compression to actually increase the size of the data, so we over allocate here to handle that.
			int32 CompressedSize = OriginalSize * 4 / 3;
			CompressedData.SetNumUninitialized(CompressedSize);

			bool bDensityCompressed = false;
			if (FCompression::CompressMemory(NAME_Zlib, CompressedData.GetData(), CompressedSize, UnCompressedData.GetData(), OriginalSize, COMPRESS_BiasSpeed))
			{
				// In the case that compressing it actually increases the size, we leave it uncompressed
				if (CompressedSize < OriginalSize) {
					bDensityCompressed = true;
				}
			}
			else {
				bDensityCompressed = false;
			
				UE_LOG(LogDAVoxel, Error, TEXT("Failed to compress chunk"));
				check(false);
			}

			const void* DataToSave = bDensityCompressed
				? static_cast<const void*>(CompressedData.GetData())
				: static_cast<const void*>(UnCompressedData.GetData());
		
			const int32 SizeInBytes = bDensityCompressed
					? CompressedSize
					: UnCompressedData.Num() * sizeof(TData);

			{
				FScopeLock Lock(&EntriesCriticalSection);
				FChunkDensityDBEntry& Entry = Entries.FindOrAdd(InKey);
				Entry.bDensityCompressed = bDensityCompressed;
				Entry.DensityCompressedSize = CompressedSize;

				FByteBulkData& BulkData = Entry.DensityData;
				void* Buffer = BulkData.Lock(LOCK_READ_WRITE);
				if (BulkData.GetBulkDataSize() != SizeInBytes) {
					Buffer = BulkData.Realloc(SizeInBytes);
				}
		
				FMemory::Memcpy(Buffer, DataToSave, SizeInBytes);
				BulkData.Unlock();
		
				Entry.bDensityLoaded = true;
			}
		}

		size_t GetMemUsage() const {
			size_t MemSize = 0;
			for (auto& Entry : Entries) {
				MemSize += Entry.Value.DensityData.GetBulkDataSize();
			}
			return MemSize;
		}
	
	private:
		FCriticalSection EntriesCriticalSection;
		TMap<Key_t, FChunkDensityDBEntry> Entries;
	};

	typedef TChunkDensityDB<float> FChunkDensityDB; 
	typedef TChunkDensityData<float> FChunkDensityData;
	typedef TSharedPtr<FChunkDensityData, ESPMode::ThreadSafe> FChunkDensityDataPtr;
}

