//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Data/DungeonForgeLayoutData.h"

#include "Core/Utils/Debug/DungeonDebugVisualizer.h"

#if WITH_EDITOR
void UDungeonForgeLayoutData::WriteDebugData(UDungeonDebugVisualizer* InDebugVisualizer) const {
	FRandomStream Random(42);
	for (FDungeonLayoutDataChunkInfo ChunkShape : Layout.ChunkShapes) {
		for (const FDAShapePolygon& ConvexPoly : ChunkShape.ConvexPolys) {
			if (ConvexPoly.Points.Num() >= 3) {
				TArray<FVector> Vertices;
				TArray<int32> Indices;
				for (const FVector2D& Point2D : ConvexPoly.Points) {
					FVector WorldLocation = ConvexPoly.Transform.TransformPosition(FVector(Point2D, 0));
					WorldLocation = Layout.DungeonTransform.TransformPosition(WorldLocation);
					Vertices.Add(WorldLocation);
				}

				for (int i = 2; i < Vertices.Num(); ++i) {
					Indices.Add(0);
					Indices.Add(i - 1);
					Indices.Add(i);
				}

				InDebugVisualizer->DrawMesh(Vertices, Indices, FColor(255, 128, 128), SDPG_Foreground, 0);
			}
		}
	}
}
#endif // WITH_EDITOR
