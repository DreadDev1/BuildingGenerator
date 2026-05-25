//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

namespace DA::VDB {
	struct FRootKey;
	template<int32 Count>
	class FBitMask {
	public:
		bool operator[](int32 Index) const {
			const uint32 Idx = Index >> Log2;
			const uint32 Off = Index & ContainerSizeMinusOne;
			return ((Mask[Idx] >> Off) & 1UL) != 0;
		}

		void Set(int32 Index, bool InValue) {
			const uint32 Idx = Index >> Log2;
			const uint32 Off = Index & ContainerSizeMinusOne;
			if (InValue) {
				Mask[Idx] |= 1ULL << Off;
			}
			else {
				Mask[Idx] &= ~(1ULL << Off);
			}
		}
		
		typedef uint64 MaskContainer_t;
		static constexpr uint32 ContainerSize = 64;
		static constexpr uint32 ContainerSizeMinusOne = ContainerSize - 1;
		static constexpr uint32 Log2 = 6;
		
		MaskContainer_t Mask[(Count - 1) / ContainerSize + 1] = {};
	};

	static FORCEINLINE int32 FloorDiv(int32 Value, int32 Divisor) {
		if (Value >= 0) {
			return Value / Divisor;
		} else {
			return (Value - Divisor + 1) / Divisor;
		}
	}
	
	static FORCEINLINE int32 PositiveMod(int32 Value, int32 Divisor) {
		int32 Result = Value % Divisor;
		return (Result < 0) ? (Result + Divisor) : Result;
	}
	
	template<typename TNode>
	static FORCEINLINE FIntVector CalculateOrigin(const FIntVector& InLocation) {
		return FIntVector {
			FloorDiv(InLocation.X, 1 << TNode::LOG2X),
			FloorDiv(InLocation.Y, 1 << TNode::LOG2Y),
			FloorDiv(InLocation.Z, 1 << TNode::LOG2Z)
		};
	}
	
	template<typename TValue, int TLog2X, int TLog2Y=TLog2X, int TLog2Z=TLog2Y>
	class TLeafNode {
	public:
		using ValueType = TValue;
		
		static constexpr int32 SIZE = 1 << (TLog2X + TLog2Y + TLog2Z);
		static constexpr int32 LOG2X = TLog2X;
		static constexpr int32 LOG2Y = TLog2Y;
		static constexpr int32 LOG2Z = TLog2Z;
		
		struct FLeafData {
			TValue Values[SIZE];
		};
		FLeafData LeafDAT;
		FBitMask<SIZE> ValueMask;
		FBitMask<SIZE> ActiveMask;  // Track active/inactive state separately
		uint64 Flags = 0;

		bool Get(const FIntVector& InLocation, ValueType& OutValue) const {
			const int32 Offset = GetLeafOffset(InLocation);
			check(Offset >= 0 && Offset < SIZE);

			if (ValueMask[Offset]) {
				OutValue = LeafDAT.Values[Offset];
				return true;  // Return true if we have data (active or inactive)
			}
			return false;  // No data at this location
		}
		
		void Set(const FIntVector& InLocation, const ValueType& InValue, bool bActive = true) {
			const int32 Offset = GetLeafOffset(InLocation);
			check(Offset >= 0 && Offset < SIZE);

			ValueMask.Set(Offset, true);
			ActiveMask.Set(Offset, bActive);
			LeafDAT.Values[Offset] = InValue;
		}
		
		void Prune(const ValueType& Tolerance = {}) {
		}
		
		static FORCEINLINE FIntVector GetOrigin(const FIntVector& InLocation) {
			return CalculateOrigin<TLeafNode>(InLocation);
		}
		
		static TLeafNode* CreateNew(const FIntVector& InLocation) {
			TLeafNode* NewNode = new TLeafNode();
			const FIntVector Origin = GetOrigin(InLocation);
			
			constexpr uint32 Bit20Mask = (1 << 20) - 1;
			const uint32 XBits = (Origin.X < 0) ? ((-Origin.X) & Bit20Mask) | (1 << 20) : (Origin.X & Bit20Mask);
			const uint32 YBits = (Origin.Y < 0) ? ((-Origin.Y) & Bit20Mask) | (1 << 20) : (Origin.Y & Bit20Mask);
			const uint32 ZBits = (Origin.Z < 0) ? ((-Origin.Z) & Bit20Mask) | (1 << 20) : (Origin.Z & Bit20Mask);
			
			NewNode->Flags =
				(static_cast<uint64>(XBits) << 42) +
				(static_cast<uint64>(YBits) << 21) +
				static_cast<uint64>(ZBits);
			
			return NewNode;
		}
		
	private:
		static FORCEINLINE int32 GetLeafOffset(const FIntVector& InLocation) {
			const int32 x = PositiveMod(InLocation.X, 1 << LOG2X);
			const int32 y = PositiveMod(InLocation.Y, 1 << LOG2Y);
			const int32 z = PositiveMod(InLocation.Z, 1 << LOG2Z);
			
			return (x << (TLog2Y + TLog2Z)) + (y << TLog2Z) + z;
		}
	};


	template<typename TChild, int TLog2X, int TLog2Y=TLog2X, int TLog2Z=TLog2Y>
	class TInternalNode {
	public:
		using ValueType = typename TChild::ValueType;
		using ChildType = TChild;

		static constexpr int32 SIZE = 1 << (TLog2X + TLog2Y + TLog2Z);
		static constexpr int32 LOG2X = TLog2X + TChild::LOG2X;
		static constexpr int32 LOG2Y = TLog2Y + TChild::LOG2Y;
		static constexpr int32 LOG2Z = TLog2Z + TChild::LOG2Z;
		
		union FInternalData {
			TChild* Child = {};
			ValueType Value;
		};
		FInternalData InternalDAT[SIZE] = {};
		FBitMask<SIZE> ValueMask;
		FBitMask<SIZE> ActiveMask;  // Track active/inactive state separately
		FBitMask<SIZE> ChildMask;
		FIntVector Origin;

		~TInternalNode() {
			for (int i = 0; i < SIZE; i++) {
				if (ChildMask[i] && InternalDAT[i].Child) {
					delete InternalDAT[i].Child;
					InternalDAT[i].Child = nullptr;
				}
			}
		}

		bool Get(const FIntVector& InLocation, ValueType& OutValue) const {
			const int32 Offset = GetInternalOffset(InLocation);
			check(Offset >= 0 && Offset < SIZE);
			
			if (ChildMask[Offset]) {
				return InternalDAT[Offset].Child->Get(InLocation, OutValue);
			}
			else if (ValueMask[Offset]) {
				OutValue = InternalDAT[Offset].Value;
				return true;  // Have tile data (active or inactive)
			}
			return false;  // No data at this location
		}

		void Set(const FIntVector& InLocation, const ValueType& InValue, bool bActive = true) {
			const int32 Offset = GetInternalOffset(InLocation);
			check(Offset >= 0 && Offset < SIZE);

			if (!ChildMask[Offset]) {
				ChildMask.Set(Offset, true);
				check(!InternalDAT[Offset].Child);
				InternalDAT[Offset].Child = TChild::CreateNew(InLocation);
			}

			InternalDAT[Offset].Child->Set(InLocation, InValue, bActive);
		}

		void SetTile(const FIntVector& InLocation, const ValueType& InValue, bool bActive = true) {
			const int32 Offset = GetInternalOffset(InLocation);
			check(Offset >= 0 && Offset < SIZE);

			if (ChildMask[Offset]) {
				delete InternalDAT[Offset].Child;
				InternalDAT[Offset].Child = nullptr;
				ChildMask.Set(Offset, false);
			}

			ValueMask.Set(Offset, true);
			ActiveMask.Set(Offset, bActive);
			InternalDAT[Offset].Value = InValue;
		}

		void Prune(const ValueType& Tolerance = {}) {
			for (int32 i = 0; i < SIZE; i++) {
				if (ChildMask[i] && InternalDAT[i].Child) {
					InternalDAT[i].Child->Prune(Tolerance);
					
					ValueType UniformValue;
					bool bUniformActive;
					if (IsChildUniform(InternalDAT[i].Child, UniformValue, bUniformActive, Tolerance)) {
						delete InternalDAT[i].Child;
						InternalDAT[i].Child = nullptr;
						ChildMask.Set(i, false);
						ValueMask.Set(i, true);
						ActiveMask.Set(i, bUniformActive);
						InternalDAT[i].Value = UniformValue;
					}
				}
			}
		}

	private:
		static bool IsChildUniform(const TChild* Child, ValueType& OutValue, bool& OutActive, const ValueType& Tolerance) {
			return false;
		}

	public:
		static TInternalNode* CreateNew(const FIntVector& InLocation) {
			TInternalNode* NewNode = new TInternalNode();
			NewNode->Origin = GetOrigin(InLocation);
			return NewNode;
		}
		
		static FORCEINLINE FIntVector GetOrigin(const FIntVector& InLocation) {
			return CalculateOrigin<TInternalNode>(InLocation);
		}
		
		static FORCEINLINE int32 GetInternalOffset(const FIntVector& InLocation) {
			constexpr int32 TLog2YZ = TLog2Y + TLog2Z;
			
			const int32 x = PositiveMod(InLocation.X, 1 << LOG2X) >> TChild::LOG2X;
			const int32 y = PositiveMod(InLocation.Y, 1 << LOG2Y) >> TChild::LOG2Y;
			const int32 z = PositiveMod(InLocation.Z, 1 << LOG2Z) >> TChild::LOG2Z;
			
			return (x << TLog2YZ) + (y << TLog2Z) + z;
		}

	};

	struct FRootKey {
		int32 X;
		int32 Y;
		int32 Z;
	};

	template<typename TChild>
	class TRootNode {
	public:
		using ValueType = typename TChild::ValueType;
		using ChildType = TChild;
		
		struct FRootData {
			TChild* Node = nullptr;
			bool TileActive = false;
			ValueType TileValue = {};
		};
		TMap<FRootKey, FRootData> RootMap;
		ValueType BackgroundValue = {};

		TRootNode() : BackgroundValue{} {}

		~TRootNode() {
			Clear();
		}

		static FORCEINLINE FRootKey CreateRootKey(const FIntVector& InLocation) {
			return FRootKey {
				FloorDiv(InLocation.X, 1 << TChild::LOG2X),
				FloorDiv(InLocation.Y, 1 << TChild::LOG2Y),
				FloorDiv(InLocation.Z, 1 << TChild::LOG2Z)
			};
		}

		bool Get(const FIntVector& InLocation, ValueType& OutValue) const {
			const FRootKey RootKey = CreateRootKey(InLocation);
			const FRootData* RootData = RootMap.Find(RootKey);
			if (!RootData) {
				OutValue = BackgroundValue;
				return false;
			}
			if (RootData->Node) {
				return RootData->Node->Get(InLocation, OutValue);
			}
			
			OutValue = RootData->TileValue;
			return RootData->TileActive;
		}
		
		void Set(const FIntVector& InLocation, const ValueType& InValue, bool bActive = true) {
			const FRootKey RootKey = CreateRootKey(InLocation);
			FRootData& RootData = RootMap.FindOrAdd(RootKey);
			if (!RootData.Node) {
				RootData.Node = TChild::CreateNew(InLocation);
			}

			RootData.Node->Set(InLocation, InValue, bActive);
		}

		void Clear() {
			for (auto& Entry : RootMap) {
				FRootData& RootData = Entry.Value;
				if (RootData.Node) {
					delete RootData.Node;
				}
			}
			RootMap.Reset();
		}
	};

	template<typename TRootNode>
	class TAccessCache {
	public:
		using ValueType = typename TRootNode::ValueType;
		typedef typename TRootNode::ChildType FNodeLOD2_t;
		typedef typename FNodeLOD2_t::ChildType FNodeLOD1_t;
		typedef typename FNodeLOD1_t::ChildType FNodeLOD0_t;

		TAccessCache(TRootNode* InRootNode) : RootNode(InRootNode) {}
		
		TRootNode* RootNode = nullptr;
		
		mutable FIntVector Node0Key = {};
		mutable FNodeLOD0_t* Node0 = nullptr;
		
		mutable FIntVector Node1Key = {};
		mutable FNodeLOD1_t* Node1 = nullptr;
		
		mutable FIntVector Node2Key = {};
		mutable FNodeLOD2_t* Node2 = nullptr;

		bool Get(const FIntVector& InLocation, ValueType& OutValue) const {
			if (Node0 != nullptr && Node0Key == FNodeLOD0_t::GetOrigin(InLocation)) {
				return Get(Node0, InLocation, OutValue);
			}
			
			if (Node1 != nullptr && Node1Key == FNodeLOD1_t::GetOrigin(InLocation)) {
				return Get(Node1, InLocation, OutValue);
			}
			
			if (Node2 != nullptr && Node2Key == FNodeLOD2_t::GetOrigin(InLocation)) {
				return Get(Node2, InLocation, OutValue);
			}

			return Get(RootNode, InLocation, OutValue);
		}
		
		void Set(const FIntVector& InLocation, const ValueType& InValue, bool bActive = true) {
			if (Node0 != nullptr && Node0Key == FNodeLOD0_t::GetOrigin(InLocation)) {
				Set(Node0, InLocation, InValue, bActive);
			}
			else if (Node1 != nullptr && Node1Key == FNodeLOD1_t::GetOrigin(InLocation)) {
				Set(Node1, InLocation, InValue, bActive);
			}
			else if (Node2 != nullptr && Node2Key == FNodeLOD2_t::GetOrigin(InLocation)) {
				Set(Node2, InLocation, InValue, bActive);
			}
			else {
				Set(RootNode, InLocation, InValue, bActive);
			}
		}
		
	private:
		// Getter functions
		FORCEINLINE bool Get(const FNodeLOD0_t* InNode, const FIntVector& InLocation, ValueType& OutValue) const {
			CacheNode(InNode, &Node0, &Node0Key, InLocation);
			return InNode->Get(InLocation, OutValue);
		}

		FORCEINLINE bool Get(const FNodeLOD1_t* InNode, const FIntVector& InLocation, ValueType& OutValue) const {
			CacheNode(InNode, &Node1, &Node1Key, InLocation);
			return GetInternal(InNode, InLocation, OutValue);
		}
		
		FORCEINLINE bool Get(const FNodeLOD2_t* InNode, const FIntVector& InLocation, ValueType& OutValue) const {
			CacheNode(InNode, &Node2, &Node2Key, InLocation);
			return GetInternal(InNode, InLocation, OutValue);
		}

		bool Get(const TRootNode* InNode, const FIntVector& InLocation, ValueType& OutValue) const {
			const FRootKey RootKey = InNode->CreateRootKey(InLocation);
			const typename TRootNode::FRootData* RootData = InNode->RootMap.Find(RootKey);
			if (!RootData) {
				OutValue = InNode->BackgroundValue;
				return false;
			}
			if (RootData->Node) {
				return Get(RootData->Node, InLocation, OutValue);
			}
			
			OutValue = RootData->TileValue;
			return RootData->TileActive;
		}
		
		template<typename TNode>
		FORCEINLINE bool GetInternal(const TNode* InNode, const FIntVector& InLocation, ValueType& OutValue) const {
			const int32 Offset = TNode::GetInternalOffset(InLocation);
			check(Offset >= 0 && Offset < TNode::SIZE);
			
			if (InNode->ChildMask[Offset]) {
				const typename TNode::ChildType* ChildNode = InNode->InternalDAT[Offset].Child;
				return Get(ChildNode, InLocation, OutValue);
			}
			else {
				const bool bValueActive = InNode->ValueMask[Offset];
				if (bValueActive) {
					OutValue = InNode->InternalDAT[Offset].Value; 
				}
				return bValueActive;
			}
		}

		// Setter functions
		FORCEINLINE void Set(FNodeLOD0_t* InNode, const FIntVector& InLocation, const ValueType& InValue, bool bActive = true) {
			CacheNode(InNode, &Node0, &Node0Key, InLocation);
			InNode->Set(InLocation, InValue, bActive);
		}
		
		FORCEINLINE void Set(FNodeLOD1_t* InNode, const FIntVector& InLocation, const ValueType& InValue, bool bActive = true) {
			CacheNode(InNode, &Node1, &Node1Key, InLocation);
			SetInternal(InNode, InLocation, InValue, bActive);
		}
		
		FORCEINLINE void Set(FNodeLOD2_t* InNode, const FIntVector& InLocation, const ValueType& InValue, bool bActive = true) {
			CacheNode(InNode, &Node2, &Node2Key, InLocation);
			SetInternal(InNode, InLocation, InValue, bActive);
		}
		
		void Set(TRootNode* InNode, const FIntVector& InLocation, const ValueType& InValue, bool bActive = true) {
			using ChildType = typename TRootNode::ChildType;
			const FRootKey RootKey = TRootNode::CreateRootKey(InLocation);
			typename TRootNode::FRootData& RootData = InNode->RootMap.FindOrAdd(RootKey);
			if (!RootData.Node) {
				RootData.Node = ChildType::CreateNew(InLocation);
			}

			Set(RootData.Node, InLocation, InValue, bActive);
		}
		
		template<typename TNode>
		FORCEINLINE void SetInternal(TNode* InNode, const FIntVector& InLocation, const ValueType& InValue, bool bActive = true) {
			using ChildType = typename TNode::ChildType;
			const int32 Offset = TNode::GetInternalOffset(InLocation);
			check(Offset >= 0 && Offset < TNode::SIZE);

			if (!InNode->ChildMask[Offset]) {
				InNode->ChildMask.Set(Offset, true);
				check(!InNode->InternalDAT[Offset].Child);
				InNode->InternalDAT[Offset].Child = ChildType::CreateNew(InLocation);
			}

			Set(InNode->InternalDAT[Offset].Child, InLocation, InValue, bActive);
		}
		
		
		// Access Caching functions
		template<typename TNode>
		FORCEINLINE void CacheNode(const TNode* InNode, TNode** CacheNodePtr, FIntVector* CacheNodeKeyPtr, const FIntVector& InLocation) const {
			if (*CacheNodePtr != InNode) {
				*CacheNodePtr = const_cast<TNode*>(InNode);
				*CacheNodeKeyPtr = TNode::GetOrigin(InLocation);
			}
		}
		
		template<typename TNode>
		FORCEINLINE void CacheNode(TNode* InNode, TNode** CacheNodePtr, FIntVector* CacheNodeKeyPtr, const FIntVector& InLocation) {
			if (*CacheNodePtr != InNode) {
				*CacheNodePtr = InNode;
				*CacheNodeKeyPtr = TNode::GetOrigin(InLocation);
			}
		}
	};

	template<typename TRootNode>
	class TVoxelGrid {
	public:
		using ValueType = typename TRootNode::ValueType;
		typedef typename TRootNode::ChildType FNodeLOD2_t;
		typedef typename FNodeLOD2_t::ChildType FNodeLOD1_t;
		typedef typename FNodeLOD1_t::ChildType FNodeLOD0_t;

		TVoxelGrid() : AccessCache(TAccessCache(&RootNode))
		{
		}
		
		bool Get(const FIntVector& InLocation, ValueType& OutValue) const {
			return AccessCache.Get(InLocation, OutValue);
		}
				
		void Set(const FIntVector& InLocation, const ValueType& InValue, bool bActive = true) {
			AccessCache.Set(InLocation, InValue, bActive);
		}

		void Clear() {
			RootNode.Clear();
		}
		
		// Active voxel iterator
		class FActiveIterator {
		public:
			FActiveIterator(const TVoxelGrid* InGrid) : Grid(InGrid) {
				InitializeFromRoot();
			}
			
			bool IsValid() const { return CurrentVoxelIndex < ActiveVoxels.Num(); }
			
			void operator++() { 
				CurrentVoxelIndex++; 
			}
			
			FIntVector GetCoordinate() const {
				return IsValid() ? ActiveVoxels[CurrentVoxelIndex].Coordinate : FIntVector::ZeroValue;
			}
			
			ValueType GetValue() const {
				return IsValid() ? ActiveVoxels[CurrentVoxelIndex].Value : ValueType{};
			}
			
		private:
			struct FActiveVoxel {
				FIntVector Coordinate;
				ValueType Value;
			};
			
			void InitializeFromRoot() {
				const TRootNode& Root = Grid->RootNode;
				
				for (const auto& RootEntry : Root.RootMap) {
					const FRootKey& Key = RootEntry.Key;
					const typename TRootNode::FRootData& Data = RootEntry.Value;
					
					if (Data.Node) {
						const FIntVector RootNodeOrigin(
							Key.X * (1 << FNodeLOD2_t::LOG2X),
							Key.Y * (1 << FNodeLOD2_t::LOG2Y),
							Key.Z * (1 << FNodeLOD2_t::LOG2Z)
						);
						
						TraverseInternalNode2(Data.Node, RootNodeOrigin);
					}
				}
			}
			
			void TraverseLeafNode(const FNodeLOD0_t* Node, const FIntVector& NodeOrigin) {
				for (int32 i = 0; i < FNodeLOD0_t::SIZE; i++) {
					if (Node->ActiveMask[i] && Node->ValueMask[i]) {
						FActiveVoxel Voxel;
						const int32 x = (i >> (FNodeLOD0_t::LOG2Y + FNodeLOD0_t::LOG2Z)) & ((1 << FNodeLOD0_t::LOG2X) - 1);
						const int32 y = (i >> FNodeLOD0_t::LOG2Z) & ((1 << FNodeLOD0_t::LOG2Y) - 1);
						const int32 z = i & ((1 << FNodeLOD0_t::LOG2Z) - 1);
						Voxel.Coordinate = NodeOrigin + FIntVector(x, y, z);
						Voxel.Value = Node->LeafDAT.Values[i];
						ActiveVoxels.Add(Voxel);
					}
				}
			}
			
			void TraverseInternalNode1(const FNodeLOD1_t* Node, const FIntVector& NodeOrigin) {
				for (int32 i = 0; i < FNodeLOD1_t::SIZE; i++) {
					if (Node->ChildMask[i] && Node->InternalDAT[i].Child) {
						const FIntVector ChildOrigin = CalculateChildOrigin<FNodeLOD1_t>(NodeOrigin, i);
						TraverseLeafNode(Node->InternalDAT[i].Child, ChildOrigin);
					} else if (Node->ActiveMask[i] && Node->ValueMask[i]) {
					}
				}
			}
			
			void TraverseInternalNode2(const FNodeLOD2_t* Node, const FIntVector& NodeOrigin) {
				for (int32 i = 0; i < FNodeLOD2_t::SIZE; i++) {
					if (Node->ChildMask[i] && Node->InternalDAT[i].Child) {
						const FIntVector ChildOrigin = CalculateChildOrigin<FNodeLOD2_t>(NodeOrigin, i);
						TraverseInternalNode1(Node->InternalDAT[i].Child, ChildOrigin);
					} else if (Node->ActiveMask[i] && Node->ValueMask[i]) {
					}
				}
			}
			
			template<typename TNode>
			FIntVector CalculateChildOrigin(const FIntVector& ParentOrigin, int32 ChildIndex) const {
				using ChildType = typename TNode::ChildType;
				constexpr int32 TLog2X = TNode::LOG2X - ChildType::LOG2X;
				constexpr int32 TLog2Y = TNode::LOG2Y - ChildType::LOG2Y;
				constexpr int32 TLog2Z = TNode::LOG2Z - ChildType::LOG2Z;
				const int32 x = (ChildIndex >> (TLog2Y + TLog2Z));
				const int32 y = (ChildIndex >> TLog2Z) & ((1 << TLog2Y) - 1);
				const int32 z = ChildIndex & ((1 << TLog2Z) - 1);
				return ParentOrigin + FIntVector(x << ChildType::LOG2X, y << ChildType::LOG2Y, z << ChildType::LOG2Z);
			}
			
			const TVoxelGrid* Grid;
			TArray<FActiveVoxel> ActiveVoxels;
			int32 CurrentVoxelIndex = 0;
		};
		
		FActiveIterator GetActiveIterator() const {
			return FActiveIterator(this);
		}
		
		TRootNode RootNode;
		
	private:
		mutable TAccessCache<TRootNode> AccessCache;
	};

	typedef uint8 VoxelMaterial_t;
	struct FVoxelData {
		VoxelMaterial_t Material = {};
		float SDF = 1.0f;  // negative=inside, positive=outside
	};
	typedef TLeafNode<FVoxelData, 3> FNodeLOD0;
	typedef TInternalNode<FNodeLOD0, 4> FNodeLOD1;
	typedef TInternalNode<FNodeLOD1, 5> FNodeLOD2;
	typedef TRootNode<FNodeLOD2> FRootNode;
	typedef TVoxelGrid<FRootNode> FVoxelGrid;
	
	FORCEINLINE uint32 GetTypeHash(const DA::VDB::FRootKey& InKey) {
		constexpr int32 Log2N = 20;
		return ((1 << Log2N) - 1) & (InKey.X * 73856093 ^ InKey.Y * 19349663 ^ InKey.Z * 83492791);
	}

	FORCEINLINE bool operator==(const DA::VDB::FRootKey& A, const DA::VDB::FRootKey& B) {
		return A.X == B.X && A.Y == B.Y && A.Z == B.Z;
	}
}

