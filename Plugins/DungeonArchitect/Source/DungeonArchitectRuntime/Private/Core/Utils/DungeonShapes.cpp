//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Utils/DungeonShapes.h"


const FName FDAShapeConstants::TagDoNotRenderOnCanvas = TEXT("CANVAS_IGNORE");

const FVector FDAShapeTexture::LocalQuadPoints[] = {
    FVector(-50, -50, 0),
    FVector( 50, -50, 0),
    FVector( 50,  50, 0),
    FVector(-50,  50, 0)
};

const FVector2D FDAShapeTexture::QuadUV[] = {
    FVector2D(0, 0),
    FVector2D(1, 0),
    FVector2D(1, 1),
    FVector2D(0, 1),
};

FBox FDAShapeUtils::CalcBounds(const FDAShapeCircle& InCircle) {
    FBox Bounds(ForceInit);

    Bounds += FVector(-InCircle.Radius, 0, 0);
    Bounds += FVector(InCircle.Radius, 0, 0);
    Bounds += FVector(0, -InCircle.Radius, 0);
    Bounds += FVector(0, InCircle.Radius, 0);
    Bounds += FVector(0, 0, InCircle.Height);

    return Bounds.TransformBy(InCircle.Transform);
}

FBox FDAShapeUtils::CalcBounds(const FDAShapeLine& InLine) {
    FBox Bounds(ForceInit);

    Bounds += FVector(InLine.LineStart, 0);
    Bounds += FVector(InLine.LineEnd, 0);
    Bounds += FVector(InLine.LineStart, InLine.Height);
    Bounds += FVector(InLine.LineEnd, InLine.Height);

    return Bounds.TransformBy(InLine.Transform);
}

FBox FDAShapeUtils::CalcBounds(const FDAShapePolygon& InPoly) {
    FBox Bounds(ForceInit);

    for (const FVector2D& Point : InPoly.Points) {
        Bounds += FVector(Point, 0);
        Bounds += FVector(Point, InPoly.Height);
    } 

    return Bounds.TransformBy(InPoly.Transform);
}

FDAShapeList FDAShapeList::TransformBy(const FTransform& InTransform) const {
	FDAShapeList Result = *this;
	
	for (FDAShapeCircle& Shape : Result.Circles) {
		Shape.Transform = Shape.Transform * InTransform;
	}

    for (FDAShapePolygon& Shape : Result.ConvexPolys) {
        Shape.Transform = Shape.Transform * InTransform;
    }
	
	return Result;
}

FBox FDAShapeList::CalcBounds() {
    FBox Bounds(ForceInit);
    for (const FDAShapePolygon& ConvexPoly : ConvexPolys) {
        Bounds += FDAShapeUtils::CalcBounds(ConvexPoly);
    }
    for (const FDAShapeCircle& Circle : Circles) {
        Bounds += FDAShapeUtils::CalcBounds(Circle);
    }
    return Bounds;
}

bool FDAShapeCollision::Intersects(const FDAShapePolygon& A, const FDAShapePolygon& B, float Tolerance) {
    if (A.Points.Num() < 3 || B.Points.Num() < 3) {
        return false;
    }
    
    if (!HeightOverlaps(A, B, Tolerance)) {
        return false;
    }

    TArray<FVector2D> PolyA, PolyB;
    TransformPoints(A.Points, A.Transform, PolyA);
    TransformPoints(B.Points, B.Transform, PolyB);
    
    TArray<FVector2D> NormalsToProject;
    {
        TSet<FVector2D> VisitedProjections;
        PopulatePolyProjectionNormals(PolyA, VisitedProjections, NormalsToProject);
        PopulatePolyProjectionNormals(PolyB, VisitedProjections, NormalsToProject);
    }


    // Separating axis algorithm to check for convex poly intersection
    // We project the points of the poly along certain axis and see if the projected points overlap
    // the axis we choose to project are chosen as the normals of the polygon edges
    // This is like projecting shadows of a 3D object on to the 2D ground (or a 2D object on a 1D ground), tested from different light angles
    // If the shadows do not overlap, the objects do not overlap
    
    for (const FVector2D& ProjectionAxis : NormalsToProject) {
        float MinA, MaxA, MinB, MaxB;
        MinA = MinB = TNumericLimits<float>::Max();
        MaxA = MaxB = TNumericLimits<float>::Lowest();

        // Project PolyA
        for (const FVector2D& Point : PolyA) {
            const float Projection = FVector2D::DotProduct(ProjectionAxis, Point);
            MinA = FMath::Min(MinA, Projection);
            MaxA = FMath::Max(MaxA, Projection);
        }
        
        // Project PolyB
        for (const FVector2D& Point : PolyB) {
            const float Projection = FVector2D::DotProduct(ProjectionAxis, Point);
            MinB = FMath::Min(MinB, Projection);
            MaxB = FMath::Max(MaxB, Projection);
        }

        // Check if they don't overlap
        const float OverlapDistance = GetOverlapsDistance(MinA, MaxA, MinB, MaxB);
        if (OverlapDistance < Tolerance) {
            // The points do not overlap.   This means the polygons themselves do not overlap
            return false;
        }
    }
    return true;
}

bool FDAShapeCollision::Intersects(const FDAShapePolygon& A, const FDAShapeCircle& B, float Tolerance) {
    if (A.Points.Num() < 3) {
        return false;
    }
    
    if (!HeightOverlaps(A, B, Tolerance)) {
        return false;
    }


    TArray<FVector2D> PolyA;
    TransformPoints(A.Points, A.Transform, PolyA);
    const FVector2D CenterB = FVector2D(B.Transform.GetLocation());

    TArray<FVector2D> NormalsToProject;
    {
        TSet<FVector2D> VisitedProjections;
        PopulatePolyProjectionNormals(PolyA, VisitedProjections, NormalsToProject);
        PopulateCircleProjectionNormals(CenterB, PolyA, VisitedProjections, NormalsToProject);
    }

    for (const FVector2D& ProjectionAxis : NormalsToProject) {
        float MinA, MaxA, MinB, MaxB;
        MinA = MinB = TNumericLimits<float>::Max();
        MaxA = MaxB = TNumericLimits<float>::Lowest();

        // Project PolyA
        for (const FVector2D& Point : PolyA) {
            const float Projection = FVector2D::DotProduct(ProjectionAxis, Point);
            MinA = FMath::Min(MinA, Projection);
            MaxA = FMath::Max(MaxA, Projection);
        }
        
        // Project CircleB
        const float CenterProjection = FVector2D::DotProduct(ProjectionAxis, CenterB);
        TArray<float> CircleProjections = {
            CenterProjection,
            CenterProjection - B.Radius,
            CenterProjection + B.Radius
        };
        
        for (const float Projection : CircleProjections) {
            MinB = FMath::Min(MinB, Projection);
            MaxB = FMath::Max(MaxB, Projection);
        }

        // Check if they don't overlap
        if (GetOverlapsDistance(MinA, MaxA, MinB, MaxB) < Tolerance) {
            // The points do not overlap.   This means the polygons themselves do not overlap
            return false;
        }
    }
    return true;
}

bool FDAShapeCollision::Intersects(const FDAShapeCircle& A, const FDAShapeCircle& B, float Tolerance) {
    if (!HeightOverlaps(A, B, Tolerance)) {
        return false;
    }

    const FVector2D CenterA(A.Transform.GetLocation());
    const FVector2D CenterB(B.Transform.GetLocation());
    const float DistanceBetweenCenters = (CenterA - CenterB).Size();
    return DistanceBetweenCenters + Tolerance < A.Radius + B.Radius;
}

void FDAShapeCollision::ConvertBoxToConvexPoly(const FBox& InBox, FDAShapePolygon& OutPoly) {
    const FVector2D BoxExtent = FVector2D(InBox.GetExtent());
    const float X = BoxExtent.X;
    const float Y = BoxExtent.Y;
    OutPoly.Points.Add(FVector2D(-X, -Y));
    OutPoly.Points.Add(FVector2D(-X, Y));
    OutPoly.Points.Add(FVector2D(X, Y));
    OutPoly.Points.Add(FVector2D(X, -Y));
    OutPoly.Height = InBox.Max.Z - InBox.Min.Z;
    OutPoly.Transform = FTransform(InBox.GetCenter() - FVector(0, 0, OutPoly.Height * 0.5));
}

void FDAShapeCollision::ConvertBoxToConvexPoly(const FTransform& InTransform, const FVector& InExtent, FDAShapePolygon& OutPoly) {
    const float X = InExtent.X;
    const float Y = InExtent.Y;
    OutPoly.Points.Add(FVector2D(-X, -Y));
    OutPoly.Points.Add(FVector2D(-X, Y));
    OutPoly.Points.Add(FVector2D(X, Y));
    OutPoly.Points.Add(FVector2D(X, -Y));
    OutPoly.Height = InExtent.Z * 2;
    OutPoly.Transform = InTransform;
}

bool FDAShapeCollision::Intersects(const FBox& A, const FDAShapePolygon& B, float Tolerance) {
    if (!BoxHeightOverlaps(A, B, Tolerance)) {
        return false;
    }

    FDAShapePolygon BoxPoly;
    ConvertBoxToConvexPoly(A, BoxPoly);
    return Intersects(BoxPoly, B, Tolerance);
}

bool FDAShapeCollision::Intersects(const FBox& A, const FDAShapeCircle& B, float Tolerance) {
    if (!BoxHeightOverlaps(A, B, Tolerance)) {
        return false;
    }

    const FVector2D Center = FVector2D(B.Transform.GetLocation());
    const FVector2D ClosestPointOnBoxToB = FVector2D(A.GetClosestPointTo(B.Transform.GetLocation()));
    const float DistanceFromBoxEdgeToCenter = (ClosestPointOnBoxToB - Center).Size();
    return DistanceFromBoxEdgeToCenter + Tolerance > B.Radius;
}

bool FDAShapeCollision::Intersects(const FDAShapeList& A, const FDAShapeList& B, float Tolerance) {
    for (const FDAShapePolygon& PolyA : A.ConvexPolys) {
        for (const FDAShapePolygon& PolyB : B.ConvexPolys) {
            if (Intersects(PolyA, PolyB, Tolerance)) {
                return true;
            }
        }

        for (const FDAShapeCircle& CircleB : B.Circles) {
            if (Intersects(PolyA, CircleB, Tolerance)) {
                return true;
            }
        } 
    }

    for (const FDAShapeCircle& CircleA : A.Circles) {
        for (const FDAShapePolygon& PolyB : B.ConvexPolys) {
            if (Intersects(PolyB, CircleA, Tolerance)) {
                return true;
            }
        }

        for (const FDAShapeCircle& CircleB : B.Circles) {
            if (Intersects(CircleA, CircleB, Tolerance)) {
                return true;
            }
        } 
    }
    return false;
}

bool FDAShapeCollision::Intersects(const FDAShapePolygon& InPoly, const FDAShapeLine& InLine, float InTolerance) {
    TArray<FVector2D> PointsPoly, PointsLine;
    TransformPoints(InPoly.Points, InPoly.Transform, PointsPoly);
    TransformPoints({ InLine.LineStart, InLine.LineEnd }, InLine.Transform, PointsLine);

    const FVector2D LineDir = PointsLine[1] - PointsLine[0];
    const float LineLength = LineDir.Length();
    for (int32 Idx = 0; Idx < PointsPoly.Num(); Idx++) {
        FVector2D EdgeDir = PointsPoly[(Idx + 1) % PointsPoly.Num()] - PointsPoly[Idx];
        FVector2D Diff = PointsPoly[Idx] - PointsLine[0];

        const float CrossProductValue = FVector2D::CrossProduct(LineDir, EdgeDir);
        // Check if they are parallel
        if (FMath::IsNearlyEqual(CrossProductValue, 0)) {
            if (InTolerance == 0) {
                const float DotProductValue = FVector2D::DotProduct(Diff, LineDir);
                if (DotProductValue >= 0 && DotProductValue <= FVector2D::DotProduct(LineDir, LineDir)) {
                    return true;
                }
            }
        }
        else {
            const float T1 = FVector2D::CrossProduct(Diff, EdgeDir) / CrossProductValue;
            const float T2 = FVector2D::CrossProduct(Diff, LineDir) / CrossProductValue;

            const float ToleranceT = InTolerance / LineLength;
            if (T1 >= ToleranceT && T1 <= 1 - ToleranceT && T2 >= ToleranceT && T2 <= 1 - ToleranceT) {
                return true; // Intersecting.
            }
        }
    }

    return false;
}

bool FDAShapeCollision::Intersects(const FBox& A, const FDAShapeList& B, float Tolerance) {
    for (const FDAShapePolygon& PolyB : B.ConvexPolys) {
        if (Intersects(A, PolyB, Tolerance)) {
            return true;
        }
    }

    for (const FDAShapeCircle& CircleB : B.Circles) {
        if (Intersects(A, CircleB, Tolerance)) {
            return true;
        }
    }

    return false;
}

void FDAShapeCollision::TransformPoints(const TArray<FVector2D>& InPoints, const FTransform& InTransform,
                                              TArray<FVector2D>& OutTransformedPoints) {
    OutTransformedPoints.Reset();
    OutTransformedPoints.Reserve(InPoints.Num());

    for (const FVector2D& LocalPoint : InPoints) {
        FVector TransformPoint = InTransform.TransformPosition(FVector(LocalPoint, 0));
        OutTransformedPoints.Add(FVector2D(TransformPoint));
    } 
}

void FDAShapeCollision::PopulatePolyProjectionNormals(const TArray<FVector2D>& InTransformedPoints, TSet<FVector2D>& Visited, TArray<FVector2D>& OutProjections) {
    for (int i = 0; i < InTransformedPoints.Num(); i++) {
        const FVector2D& P0 = InTransformedPoints[i];
        const FVector2D& P1 = InTransformedPoints[(i + 1) % InTransformedPoints.Num()];
        const FVector2D Dir = (P1 - P0).GetSafeNormal();
        const FVector2D Normal(-Dir.Y, Dir.X);
        if (!Visited.Contains(Normal)) {
            Visited.Add(Normal);
            Visited.Add(-Normal);
            OutProjections.Add(Normal);
        }
    }
}

void FDAShapeCollision::PopulateCircleProjectionNormals(const FVector2D& InTransformedCenter, const TArray<FVector2D>& InTransformedPolyPoints, TSet<FVector2D>& Visited, TArray<FVector2D>& OutProjections) {
    for (int i = 0; i < InTransformedPolyPoints.Num(); i++) {
        const FVector2D& P0 = InTransformedPolyPoints[i];
        const FVector2D& P1 = InTransformedCenter;
        const FVector2D Dir = (P1 - P0).GetSafeNormal();
        if (!Visited.Contains(Dir)) {
            Visited.Add(Dir);
            Visited.Add(-Dir);
            OutProjections.Add(Dir);
        }
    }
}

