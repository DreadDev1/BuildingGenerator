//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

struct FRandomStream;

class INoiseTable2D {
public:
    virtual ~INoiseTable2D() {
    }

    virtual void Init(int32 InSize, const FRandomStream& InRandom) = 0;
    virtual float GetNoise(float u, float v) const = 0;
};

template <typename T>
struct FNoiseTableCell2D {
    T N00;
    T N10;
    T N01;
    T N11;
};

template <typename T, typename TNoisePolicy>
class FNoiseTable2D : public INoiseTable2D {
public:
    virtual void Init(int32 InSize, const FRandomStream& InRandom) override {
        Size = InSize;
        int32 NumElements = Size * Size;
        Data.AddUninitialized(NumElements);

        int32 LastIdx = Size - 1;
        for (int y = 0; y < Size; y++) {
            for (int x = 0; x < Size; x++) {
                if (x == LastIdx || y == LastIdx) {
                    int ix = x % LastIdx;
                    int iy = y % LastIdx;
                    Data[IDX(x, y)] = Data[IDX(ix, iy)];
                }
                else {
                    Data[IDX(x, y)] = TNoisePolicy::GetRandom(InRandom);
                }
            }
        }
    }

    void GetCell(float x, float y, FNoiseTableCell2D<T>& OutCell) const {
        int x0 = FMath::FloorToInt(x) % Size;
        int y0 = FMath::FloorToInt(y) % Size;
        if (x0 < 0) { x0 += Size; }
        if (y0 < 0) { y0 += Size; }
        int x1 = (x0 + 1) % Size;
        int y1 = (y0 + 1) % Size;

        float fx = FMath::Frac(x);
        float fy = FMath::Frac(y);

        OutCell.N00 = Data[IDX(x0, y0)];
        OutCell.N10 = Data[IDX(x1, y0)];
        OutCell.N01 = Data[IDX(x0, y1)];
        OutCell.N11 = Data[IDX(x1, y1)];
    }

    virtual float GetNoise(float u, float v) const override {
        float x = u * (Size - 1);
        float y = v * (Size - 1);

        return TNoisePolicy::Sample(x, y, *this);
    }

    float GetFbmNoise(const FVector2D& Location, int32 Octaves) const {
        FVector2D P = Location / static_cast<float>(Size);
        float Noise = 0;
        float Amp = 1;
        for (int i = 0; i < Octaves; i++) {
            Noise += Amp * GetNoise(P.X, P.Y);
            P *= 1.986576f;
            Amp *= 0.5f;
        }
        Noise = 0.5f + Noise * 0.5f;
        return FMath::Clamp(Noise, 0.0f, 1.0f);
    }

    FORCEINLINE T GetTableData(int32 x, int32 y) const {
        return Data[IDX(x, y)];
    }

    FORCEINLINE int32 GetSize() const {
        return Size;
    }

protected:
    FORCEINLINE int32 IDX(int32 x, int32 y) const {
        return y * Size + x;
    }

protected:
    int32 Size;
    TArray<T> Data;
};

////////////////////////////// Value Noise //////////////////////////////
typedef FNoiseTable2D<float, class FValueNoisePolicy2D> FValueNoiseTable2D;

class FValueNoisePolicy2D {
public:
    static float Sample(float x, float y, const FValueNoiseTable2D& NoiseTable);
    static float GetRandom(const FRandomStream& InRandom);
};


//////////////////////////// Gradient Noise /////////////////////////////
typedef FNoiseTable2D<FVector2D, class FGradientNoisePolicy2D> FGradientNoiseTable2D;

class FGradientNoisePolicy2D {
public:
    static float Sample(float x, float y, const FGradientNoiseTable2D& NoiseTable);
    static FVector2D GetRandom(const FRandomStream& InRandom);
};

//////////////////////////// Worley Noise /////////////////////////////
typedef FNoiseTable2D<FVector2D, class FWorleyNoisePolicy2D> FWorleyNoiseTable2D;

class FWorleyNoisePolicy2D {
public:
    static float Sample(float x, float y, const FWorleyNoiseTable2D& NoiseTable);
    static FVector2D GetRandom(const FRandomStream& InRandom);

private:
    static FORCEINLINE int32 GetNeighborIndex(int32 Idx, int32 TableSize) {
        if (Idx < 0) {
            Idx += TableSize - 1;
        }
        Idx = Idx % (TableSize - 1);
        return Idx;
    }
};

