#pragma once
#include <cstdint>
#include <iostream>

// This is a simple class that represents an individual 64-bit word from a bitboard.
// It can be used to represent a single square on the chess board, or multiple squares for more complex operations.

#define FlipIf(cond, X) ((cond) ? ((X) ^ 56) : (X))
#define RotIf(cond, X) ((cond) ? ((X) ^ 63) : (X))

class BitboardElement {
  public:
    // Constructors
    BitboardElement()
        : _data(0) { }
    explicit BitboardElement(uint64_t data)
        : _data(data) { }

    // Getters and Setters
    uint64_t getData() const { return _data; }
    void setData(uint64_t data) { _data = data; }

    // Method to loop through each bit in the element and perform an operation on it.
    template <typename Func>
    void forEachBit(Func func) const {
        if (_data != 0) {
            uint64_t tempData = _data;
            while (tempData) {
                int index = bitScanForward(tempData);
                func(index);
                tempData &= tempData - 1;
            }
        }
    }

    // Method to count the number of set bits in the element.
    int countSetBits() const {
#if defined(_MSC_VER) && !defined(__clang__)
        return __popcnt64(_data);
#else
        return __builtin_popcountll(_data);
#endif
    }

    // Overloaded operators
    // Bitwise AND (`&`)
    BitboardElement operator&(const BitboardElement& other) const {
        return BitboardElement(_data & other._data);
    }
    BitboardElement operator&(const uint64_t other) const {
        return BitboardElement(_data & other);
    }
    BitboardElement& operator&=(const BitboardElement& other) {
        _data &= other._data;
        return *this;
    }
    BitboardElement& operator&=(const uint64_t other) {
        _data &= other;
        return *this;
    }
    // Check if any common bits are set
    bool anyCommonBits(const BitboardElement& other) const {
        return (_data & other._data) != 0;
    }
    bool anyCommonBits(const uint64_t other) const {
        return (_data & other) != 0;
    }
    bool containsSquare(int square) const {
        return ((_data >> square) & 1) != 0;
    }

    const int firstBit() const {
        return bitScanForward(_data);
    }
    // Bitwise OR (`|`)
    BitboardElement operator|(const BitboardElement& other) const {
        return BitboardElement(_data | other._data);
    }
    BitboardElement operator|(const uint64_t other) const {
        return BitboardElement(_data | other);
    }
    BitboardElement& operator|=(const BitboardElement& other) {
        _data |= other._data;
        return *this;
    }
    BitboardElement& operator|=(const uint64_t other) {
        _data |= other;
        return *this;
    }

    // Bitwise NOT (`~`)
    BitboardElement operator~() const {
        return BitboardElement(~_data);
    }

    // Bitwise XOR (`^`)
    BitboardElement operator^(const BitboardElement& other) const {
        return BitboardElement(_data ^ other._data);
    }

    // Left Shift (`<<`)
    BitboardElement operator<<(int n) const {
        return BitboardElement(_data << n);
    }

    // Right Shift (`>>`)
    BitboardElement operator>>(int n) const {
        return BitboardElement(_data >> n);
    }

    // Increment and Decrement (`++`, `--`)
    BitboardElement& operator++() {
        _data = (_data + 1) & 0xFFFFFFFFFFFFFFFFULL;
        return *this;
    }

    BitboardElement& operator--() {
        _data = (_data - 1) & 0xFFFFFFFFFFFFFFFFULL;
        return *this;
    }

    // Equality and Inequality Comparisons (`==`, `!=`)
    bool operator==(const BitboardElement& other) const {
        return _data == other._data;
    }

    bool operator!=(const BitboardElement& other) const {
        return _data != other._data;
    }

    // Stream Insertion and Extraction Operators (`<<`, `>>`)
    friend std::ostream& operator<<(std::ostream& os, const BitboardElement& bb) {
        uint64_t tempData = bb._data;
        for (int i = 0; i < 64; ++i) {
            if (tempData & 1ULL << i) {
                os << "1";
            } else {
                os << "0";
            }
        }
        return os;
    }

    friend std::istream& operator>>(std::istream& is, BitboardElement& bb) {
        std::string binary;
        is >> binary;
        uint64_t data = 0;
        for (int i = 0; i < 64; ++i) {
            if (binary[i] == '1') {
                data |= 1ULL << i;
            }
        }
        bb._data = data;
        return is;
    }

    bool empty() const {
        return _data == 0;
    }

  private:
    //
    // bitboard move generation
    //
    inline int bitScanForward(uint64_t bb) const {
#if defined(_MSC_VER) && !defined(__clang__)
        unsigned long index;
        _BitScanForward64(&index, bb);
        return index;
#else
        return __builtin_ffsll(bb) - 1;
#endif
    };

    uint64_t _data;
};
