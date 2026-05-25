//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

/////////////////
template <typename T>
class FCombination {
private:
    TArray<T> Data;
    int SelectionCount;

    struct FStackState {
        TArray<T> buffer;
        int startIndex;

        FStackState() : startIndex(0) {
        }

        FStackState(const TArray<T>& InBuffer, int32 InStartIndex)
            : buffer(InBuffer)
              , startIndex(InStartIndex) {
        }
    };

    TArray<FStackState> Stack;

public:
    FORCEINLINE bool CanRun() const { return Stack.Num() > 0; }

    FCombination(const TArray<T>& InData, int InSelectionCount) {
        Data = InData;
        SelectionCount = InSelectionCount;

        Stack.Push(FStackState());
    }

    TArray<T> Execute() {
        while (Stack.Num() > 0) {
            FStackState top = Stack.Pop();
            if (top.buffer.Num() == SelectionCount) {
                return top.buffer;
            }
            const int BufferLength = top.buffer.Num();
            int ItemsLeft = SelectionCount - BufferLength - 1;
            const int EndIndex = Data.Num() - ItemsLeft;
            for (int i = top.startIndex; i < EndIndex; ++i) {
                TArray<T> nextBuffer = TArray<T>(top.buffer);
                nextBuffer.Add(Data[i]);

                Stack.Push(FStackState(nextBuffer, i + 1));
            }
        }
        return TArray<T>();
    }
};

template <typename T>
class FPermutation {
private:
    int K = -1;
    int L = -1;
    bool first = true;

public:
    TArray<T> Data;

    // Assumes data is sorted
    FPermutation(const TArray<T>& InData) {
        Data = InData;
    }

    FORCEINLINE bool CanPermute() const { return first || K >= 0; }

    void Permutate() {
        if (first) {
            first = false;
            FindIndices();
            return;
        }

        if (!CanPermute()) {
            return;
        }

        Swap(K, L);
        Reverse(K + 1, Data.Num() - 1);
        FindIndices();
    }

private:
    void FindIndices() {
        K = -1;
        for (int k = 0; k + 1 < Data.Num(); k++) {
            if (PermuteCompareTo(Data[k], Data[k + 1]) < 0) {
                K = k;
            }
        }

        if (K >= 0) {
            for (int l = K + 1; l < Data.Num(); l++) {
                if (PermuteCompareTo(Data[K], Data[l]) < 0) {
                    L = l;
                }
            }
        }
    }

    void Reverse(int a, int b) {
        while (a < b) {
            Swap(a, b);
            a++;
            b--;
        }
    }

    void Swap(int a, int b) {
        T t = Data[a];
        Data[a] = Data[b];
        Data[b] = t;
    }
};

