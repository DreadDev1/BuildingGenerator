//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Utils/MathUtils.h"


TArray<int32> FDAMathUtils::GetShuffledIndices(int32 Count, const FRandomStream& Random) {
    TArray<int32> Indices;
    GetShuffledIndices(Count, Random, Indices);
    return Indices;
}

void FDAMathUtils::GetShuffledIndices(int32 Count, const FRandomStream& Random, TArray<int32>& OutIndices) {
    for (int i = 0; i < Count; i++) {
        OutIndices.Add(i);
    }

    Shuffle(OutIndices, Random);
}

double FDAMathUtils::FindAngle(const FVector2d& InVec) {
    if (InVec.X == 0) {
        if (InVec.Y == 0) return 0;
        return InVec.Y > 0 ? UE_DOUBLE_PI * 0.5 : UE_DOUBLE_PI * 1.5;
    }
    else if (InVec.Y == 0) {
        return (InVec.X > 0) ? 0 : UE_DOUBLE_PI;
    }

    double Angle = FMath::Atan(InVec.Y / InVec.X);
    if (InVec.X < 0 && InVec.Y < 0) {
        // Quadrant 3
        Angle += UE_DOUBLE_PI;
    }
    else if (InVec.X < 0) {
        // Quadrant 2
        Angle += UE_DOUBLE_PI;
    }
    else if (InVec.Y < 0) {
        Angle += UE_DOUBLE_PI * 2;
    }
    return Angle;
}

bool FDAMathUtils::CalcCircumCenter(const FVector2d& A, const FVector2d& B, const FVector2d& C, FVector2d& OutCircumCenter, double& OutRadius) {
    // https://en.wikipedia.org/wiki/Circumscribed_circle#Circumcircle_equations
    // Move the points to origin to simplify the calculation
    const FVector2d b = B - A;
    const FVector2d c = C - A;

    const double d = 2 * (b.X * c.Y - b.Y * c.X);
    static constexpr double EPSILON = 1e-10;
    if (FMath::Abs(d) < EPSILON) {
        // Co-linear points
        return false;
    }

    const double ux = (c.Y * (b.X * b.X + b.Y * b.Y) - b.Y * (c.X * c.X + c.Y * c.Y)) / d;
    const double uy = (b.X * (c.X * c.X + c.Y * c.Y) - c.X * (b.X * b.X + b.Y * b.Y)) / d;

    FVector2d u{ux, uy};
    OutRadius = u.Size();
    OutCircumCenter = u + A;    // Bring it back from origin to the correct location
    return true;
}

bool FDAMathUtils::RayRayIntersection(const FVector2d& AO, const FVector2d& AD, const FVector2d& BO, const FVector2d& BD, double& OutTA, double& OutTB) {
    const FVector2d D = BO - AO;
    const double Det = BD.X * AD.Y - BD.Y * AD.X;
    static constexpr double EPSILON = 1e-10;  
    if (FMath::Abs(Det) < EPSILON) return false;

    double U = (D.Y * BD.X - D.X * BD.Y) / Det;
    double V = (D.Y * AD.X - D.X * AD.Y) / Det;

    OutTA = U;
    OutTB = V;
    return U >= 0 && V >= 0 && U <= 1 && V <= 1;
}

double FDAMathUtils::GetLineToPointDist2D(const FVector2d& LineA, const FVector2d& LineB, const FVector2d& Point) {
    const double DistSq = (LineB - LineA).SizeSquared();
    if (DistSq < 1e-6f) {
        return (LineA - Point).Size();
    }

    const double Dot = FVector2d::DotProduct(Point - LineA, LineB - LineA);
    const double T = FMath::Max(0, FMath::Min(1, Dot / DistSq));
    const FVector2d Projection = LineA + T * (LineB - LineA);
    return (Projection - Point).Size();
}


void BlurUtils::boxBlurH_4(float* scl, float* tcl, float* weights, int32 w, int32 h, int32 r) {
    float iarr = 1.0f / (r + r + 1);
    for (int32 i = 0; i < h; i++) {
        int32 ti = i * w, li = ti, ri = ti + r;
        float fv = scl[ti], lv = scl[ti + w - 1], val = (r + 1) * fv, weight;
        for (int32 j = 0; j < r; j++) val += scl[ti + j];
        for (int32 j = 0; j <= r; j++) {
            val += scl[ri++] - fv;
            weight = weights[ti];
            tcl[ti] = scl[ti] * weight + BlurRound(val * iarr) * (1 - weight);
            ti++;
        }

        for (int32 j = r + 1; j < w - r; j++) {
            val += scl[ri++] - scl[li++];
            weight = weights[ti];
            tcl[ti] = scl[ti] * weight + BlurRound(val * iarr) * (1 - weight);
            ti++;
        }

        for (int32 j = w - r; j < w; j++) {
            val += lv - scl[li++];
            weight = weights[ti];
            tcl[ti] = scl[ti] * weight + BlurRound(val * iarr) * (1 - weight);
            ti++;
        }
    }
}

void BlurUtils::boxBlurT_4(float* scl, float* tcl, float* weights, int32 w, int32 h, int32 r) {
    float iarr = 1.0f / (r + r + 1);
    for (int32 i = 0; i < w; i++) {
        int32 ti = i, li = ti, ri = ti + r * w;
        float fv = scl[ti], lv = scl[ti + w * (h - 1)], val = (r + 1) * fv, weight;
        for (int32 j = 0; j < r; j++) val += scl[ti + j * w];
        for (int32 j = 0; j <= r; j++) {
            val += scl[ri] - fv;
            weight = weights[ti];
            tcl[ti] = scl[ti] * weight + BlurRound(val * iarr) * (1 - weight);
            ri += w;
            ti += w;
        }

        for (int32 j = r + 1; j < h - r; j++) {
            val += scl[ri] - scl[li];
            weight = weights[ti];
            tcl[ti] = scl[ti] * weight + BlurRound(val * iarr) * (1 - weight);
            li += w;
            ri += w;
            ti += w;
        }

        for (int32 j = h - r; j < h; j++) {
            val += lv - scl[li];
            weight = weights[ti];
            tcl[ti] = scl[ti] * weight + BlurRound(val * iarr) * (1 - weight);
            li += w;
            ti += w;
        }
    }
}

void BlurUtils::boxBlur_4(float* scl, float* tcl, float* weights, int32 w, int32 h, int32 r) {
    int32 Count = w * h;
    for (int32 i = 0; i < Count; i++) tcl[i] = scl[i];
    boxBlurH_4(tcl, scl, weights, w, h, r);
    boxBlurT_4(scl, tcl, weights, w, h, r);
}

TArray<int32> BlurUtils::boxesForGauss(float sigma, float n) // standard deviation, number of boxes
{
    float wIdeal = FMath::Sqrt((12 * sigma * sigma / n) + 1); // Ideal averaging filter width 
    int32 wl = FMath::FloorToInt(wIdeal);
    if (wl % 2 == 0) wl--;
    int32 wu = wl + 2;

    float mIdeal = (12 * sigma * sigma - n * wl * wl - 4 * n * wl - 3 * n) / static_cast<float>(-4 * wl - 4);
    int32 m = FMath::RoundToInt(mIdeal);
    // var sigmaActual = Math.sqrt( (m*wl*wl + (n-m)*wu*wu - n)/12 );

    TArray<int32> sizes;
    for (int32 i = 0; i < n; i++) {
        sizes.Add(i < m ? wl : wu);
    }
    return sizes;
}

void BlurUtils::gaussBlur_4(float* scl, float* tcl, float* weights, int32 w, int32 h, int32 r) {
    TArray<int32> bxs = boxesForGauss(r, 3);
    boxBlur_4(scl, tcl, weights, w, h, (bxs[0] - 1) / 2);
    boxBlur_4(tcl, scl, weights, w, h, (bxs[1] - 1) / 2);
    boxBlur_4(scl, tcl, weights, w, h, (bxs[2] - 1) / 2);
}

FLinearColor FColorUtils::BrightenColor(const FLinearColor& InColor, float SaturationMultiplier,
                                        float BrightnessMultiplier) {
    FLinearColor HSV = InColor.LinearRGBToHSV();
    HSV.G *= SaturationMultiplier;
    HSV.B = FMath::Clamp(HSV.B * BrightnessMultiplier, 0.0f, 1.0f);
    return HSV.HSVToLinearRGB();
}

FLinearColor FColorUtils::GetRandomColor(const FRandomStream& Random, float Saturation) {
    const float H = Random.FRand() * 360;
    constexpr float V = 1.0f;
    return FLinearColor(H, Saturation, V).HSVToLinearRGB();
}

FVector FRandomUtils::GetPointOnTriangle(const FRandomStream& InRandom, const FVector& P0, const FVector& P1, const FVector& P2) {
    // https://blogs.sas.com/content/iml/2020/10/19/random-points-in-triangle.html 
    const FVector A = P1 - P0;
    const FVector B = P2 - P0;
    float U = InRandom.FRand();
    float V = InRandom.FRand();
    if (U + V > 1) {
        U = 1 - U;
        V = 1 - V;
    }
    const FVector W = A * U + B * V;
    return P0 + W;
}

FGuid FRandomUtils::CreateGuid(const FRandomStream& InRandom) {
    FGuid Guid;
    Guid[0] = InRandom.GetUnsignedInt();
    Guid[1] = InRandom.GetUnsignedInt();
    Guid[2] = InRandom.GetUnsignedInt();
    Guid[3] = InRandom.GetUnsignedInt();
    return Guid;
}

