#ifndef DYNAMIC_BITSET_HPP
#define DYNAMIC_BITSET_HPP

#include <cstring>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <string>

namespace sjtu {

struct dynamic_bitset {
    static constexpr std::size_t INLINE_CAPACITY = 24;
    uint64_t inline_blocks[INLINE_CAPACITY] = {0};
    uint64_t *dynamic_blocks = nullptr;
    std::size_t bit_size = 0;
    std::size_t capacity = 0; // 0 means lazy allocation
    bool is_inline = true;

    uint64_t *get_blocks() { return is_inline ? inline_blocks : dynamic_blocks; }
    const uint64_t *get_blocks() const { return is_inline ? inline_blocks : dynamic_blocks; }

    dynamic_bitset() = default;

    ~dynamic_bitset() {
        if (!is_inline && dynamic_blocks) {
            delete[] dynamic_blocks;
        }
    }

    dynamic_bitset(const dynamic_bitset &other) {
        bit_size = other.bit_size;
        capacity = other.capacity;
        is_inline = other.is_inline;
        if (capacity > 0) {
            if (is_inline) {
                std::memcpy(inline_blocks, other.inline_blocks, INLINE_CAPACITY * sizeof(uint64_t));
            } else {
                dynamic_blocks = new uint64_t[capacity];
                std::memcpy(dynamic_blocks, other.dynamic_blocks, capacity * sizeof(uint64_t));
            }
        }
    }

    dynamic_bitset(dynamic_bitset &&other) noexcept {
        bit_size = other.bit_size;
        capacity = other.capacity;
        is_inline = other.is_inline;
        if (capacity > 0) {
            if (is_inline) {
                std::memcpy(inline_blocks, other.inline_blocks, INLINE_CAPACITY * sizeof(uint64_t));
            } else {
                dynamic_blocks = other.dynamic_blocks;
                other.dynamic_blocks = nullptr;
                other.is_inline = true;
                other.capacity = 0;
                other.bit_size = 0;
            }
        }
    }

    dynamic_bitset &operator=(const dynamic_bitset &other) {
        if (this == &other) return *this;
        if (other.capacity == 0) {
            if (!is_inline && dynamic_blocks) {
                delete[] dynamic_blocks;
                dynamic_blocks = nullptr;
            }
            capacity = 0;
            is_inline = true;
            bit_size = other.bit_size;
            return *this;
        }
        
        if (!is_inline && dynamic_blocks && capacity != other.capacity) {
            delete[] dynamic_blocks;
            dynamic_blocks = nullptr;
        }
        
        bit_size = other.bit_size;
        capacity = other.capacity;
        is_inline = other.is_inline;
        
        if (is_inline) {
            std::memcpy(inline_blocks, other.inline_blocks, INLINE_CAPACITY * sizeof(uint64_t));
        } else {
            if (!dynamic_blocks) {
                dynamic_blocks = new uint64_t[capacity];
            }
            std::memcpy(dynamic_blocks, other.dynamic_blocks, capacity * sizeof(uint64_t));
        }
        return *this;
    }

    dynamic_bitset &operator=(dynamic_bitset &&other) noexcept {
        if (this == &other) return *this;
        if (!is_inline && dynamic_blocks) {
            delete[] dynamic_blocks;
            dynamic_blocks = nullptr;
        }
        
        bit_size = other.bit_size;
        capacity = other.capacity;
        is_inline = other.is_inline;
        
        if (capacity > 0) {
            if (is_inline) {
                std::memcpy(inline_blocks, other.inline_blocks, INLINE_CAPACITY * sizeof(uint64_t));
            } else {
                dynamic_blocks = other.dynamic_blocks;
                other.dynamic_blocks = nullptr;
                other.is_inline = true;
                other.capacity = 0;
                other.bit_size = 0;
            }
        }
        return *this;
    }

    dynamic_bitset(std::size_t n) {
        bit_size = n;
        capacity = 0;
        is_inline = true;
    }

    dynamic_bitset(const std::string &str) {
        bit_size = str.length();
        std::size_t req_cap = (bit_size + 63) / 64;
        
        bool has_one = false;
        for (std::size_t i = 0; i < bit_size; ++i) {
            if (str[i] == '1') {
                has_one = true;
                break;
            }
        }
        
        if (!has_one) {
            capacity = 0;
            is_inline = true;
            return;
        }
        
        if (req_cap <= INLINE_CAPACITY) {
            is_inline = true;
            capacity = INLINE_CAPACITY;
            std::memset(inline_blocks, 0, INLINE_CAPACITY * sizeof(uint64_t));
        } else {
            is_inline = false;
            capacity = req_cap;
            dynamic_blocks = new uint64_t[capacity];
            std::memset(dynamic_blocks, 0, capacity * sizeof(uint64_t));
        }
        
        uint64_t *blocks = get_blocks();
        for (std::size_t i = 0; i < bit_size; ++i) {
            if (str[i] == '1') {
                blocks[i / 64] |= (1ULL << (i % 64));
            }
        }
    }

    void _clean_last_block() {
        if (capacity == 0) return;
        std::size_t rem = bit_size % 64;
        if (rem > 0) {
            std::size_t last_idx = (bit_size - 1) / 64;
            get_blocks()[last_idx] &= (1ULL << rem) - 1;
        }
    }

    void _allocate() {
        if (capacity == 0 && bit_size > 0) {
            std::size_t req_cap = (bit_size + 63) / 64;
            if (req_cap <= INLINE_CAPACITY) {
                is_inline = true;
                capacity = INLINE_CAPACITY;
                std::memset(inline_blocks, 0, INLINE_CAPACITY * sizeof(uint64_t));
            } else {
                is_inline = false;
                capacity = req_cap;
                dynamic_blocks = new uint64_t[capacity];
                std::memset(dynamic_blocks, 0, capacity * sizeof(uint64_t));
            }
        }
    }

    bool operator[](std::size_t n) const {
        if (capacity == 0) return false;
        return (get_blocks()[n / 64] >> (n % 64)) & 1;
    }

    dynamic_bitset &set(std::size_t n, bool val = true) {
        if (capacity == 0) {
            if (!val) return *this;
            _allocate();
        }
        uint64_t *blocks = get_blocks();
        if (val) {
            blocks[n / 64] |= (1ULL << (n % 64));
        } else {
            blocks[n / 64] &= ~(1ULL << (n % 64));
        }
        return *this;
    }

    void reserve(std::size_t new_cap) {
        if (new_cap > capacity) {
            if (new_cap <= INLINE_CAPACITY) {
                capacity = INLINE_CAPACITY;
            } else {
                uint64_t *new_blocks = new uint64_t[new_cap];
                std::memset(new_blocks, 0, new_cap * sizeof(uint64_t));
                if (capacity > 0) {
                    std::memcpy(new_blocks, get_blocks(), capacity * sizeof(uint64_t));
                }
                if (!is_inline && dynamic_blocks) {
                    delete[] dynamic_blocks;
                }
                dynamic_blocks = new_blocks;
                capacity = new_cap;
                is_inline = false;
            }
        }
    }

    dynamic_bitset &push_back(bool val) {
        if (capacity == 0) {
            if (!val) {
                bit_size++;
                return *this;
            }
            _allocate();
        }
        std::size_t req_cap = (bit_size + 1 + 63) / 64;
        if (req_cap > capacity) {
            reserve(capacity == 0 ? 1 : capacity * 2);
        }
        if (val) {
            get_blocks()[bit_size / 64] |= (1ULL << (bit_size % 64));
        }
        bit_size++;
        return *this;
    }

    bool none() const {
        if (capacity == 0 || bit_size == 0) return true;
        const uint64_t *blocks = get_blocks();
        std::size_t full_blocks = bit_size / 64;
        for (std::size_t i = 0; i < full_blocks; ++i) {
            if (blocks[i] != 0) return false;
        }
        std::size_t rem = bit_size % 64;
        if (rem > 0) {
            uint64_t mask = (1ULL << rem) - 1;
            if ((blocks[full_blocks] & mask) != 0) return false;
        }
        return true;
    }

    bool all() const {
        if (bit_size == 0) return true;
        if (capacity == 0) return false;
        const uint64_t *blocks = get_blocks();
        std::size_t full_blocks = bit_size / 64;
        for (std::size_t i = 0; i < full_blocks; ++i) {
            if (blocks[i] != ~0ULL) return false;
        }
        std::size_t rem = bit_size % 64;
        if (rem > 0) {
            uint64_t mask = (1ULL << rem) - 1;
            if ((blocks[full_blocks] & mask) != mask) return false;
        }
        return true;
    }

    std::size_t size() const {
        return bit_size;
    }

    dynamic_bitset &operator|=(const dynamic_bitset &other) {
        if (other.capacity == 0) return *this;
        if (capacity == 0) _allocate();
        uint64_t *blocks = get_blocks();
        const uint64_t *other_blocks = other.get_blocks();
        std::size_t min_len = std::min(bit_size, other.bit_size);
        std::size_t full_blocks = min_len / 64;
        for (std::size_t i = 0; i < full_blocks; ++i) {
            blocks[i] |= other_blocks[i];
        }
        std::size_t rem = min_len % 64;
        if (rem > 0) {
            uint64_t mask = (1ULL << rem) - 1;
            blocks[full_blocks] = (blocks[full_blocks] & ~mask) | ((blocks[full_blocks] | other_blocks[full_blocks]) & mask);
        }
        return *this;
    }

    dynamic_bitset &operator&=(const dynamic_bitset &other) {
        if (capacity == 0) return *this;
        if (other.capacity == 0) {
            std::size_t min_len = std::min(bit_size, other.bit_size);
            std::size_t full_blocks = min_len / 64;
            uint64_t *blocks = get_blocks();
            for (std::size_t i = 0; i < full_blocks; ++i) {
                blocks[i] = 0;
            }
            std::size_t rem = min_len % 64;
            if (rem > 0) {
                uint64_t mask = (1ULL << rem) - 1;
                blocks[full_blocks] &= ~mask;
            }
            return *this;
        }
        uint64_t *blocks = get_blocks();
        const uint64_t *other_blocks = other.get_blocks();
        std::size_t min_len = std::min(bit_size, other.bit_size);
        std::size_t full_blocks = min_len / 64;
        for (std::size_t i = 0; i < full_blocks; ++i) {
            blocks[i] &= other_blocks[i];
        }
        std::size_t rem = min_len % 64;
        if (rem > 0) {
            uint64_t mask = (1ULL << rem) - 1;
            blocks[full_blocks] = (blocks[full_blocks] & ~mask) | ((blocks[full_blocks] & other_blocks[full_blocks]) & mask);
        }
        return *this;
    }

    dynamic_bitset &operator^=(const dynamic_bitset &other) {
        if (other.capacity == 0) return *this;
        if (capacity == 0) _allocate();
        uint64_t *blocks = get_blocks();
        const uint64_t *other_blocks = other.get_blocks();
        std::size_t min_len = std::min(bit_size, other.bit_size);
        std::size_t full_blocks = min_len / 64;
        for (std::size_t i = 0; i < full_blocks; ++i) {
            blocks[i] ^= other_blocks[i];
        }
        std::size_t rem = min_len % 64;
        if (rem > 0) {
            uint64_t mask = (1ULL << rem) - 1;
            blocks[full_blocks] = (blocks[full_blocks] & ~mask) | ((blocks[full_blocks] ^ other_blocks[full_blocks]) & mask);
        }
        return *this;
    }

    dynamic_bitset &operator<<=(std::size_t n) {
        if (n == 0) return *this;
        std::size_t new_size = bit_size + n;
        if (capacity == 0) {
            bit_size = new_size;
            return *this;
        }
        std::size_t new_cap = (new_size + 63) / 64;
        
        uint64_t *new_blocks;
        bool new_is_inline = false;
        if (new_cap <= INLINE_CAPACITY) {
            new_blocks = new uint64_t[INLINE_CAPACITY];
            new_is_inline = true;
            new_cap = INLINE_CAPACITY;
        } else {
            new_blocks = new uint64_t[new_cap];
        }
        std::memset(new_blocks, 0, new_cap * sizeof(uint64_t));
        
        std::size_t block_shift = n / 64;
        std::size_t bit_shift = n % 64;
        
        uint64_t *blocks = get_blocks();
        std::size_t old_blocks = (bit_size + 63) / 64;
        for (std::size_t i = 0; i < old_blocks; ++i) {
            if (bit_shift == 0) {
                new_blocks[i + block_shift] = blocks[i];
            } else {
                new_blocks[i + block_shift] |= (blocks[i] << bit_shift);
                if (i + block_shift + 1 < new_cap) {
                    new_blocks[i + block_shift + 1] |= (blocks[i] >> (64 - bit_shift));
                }
            }
        }
        
        if (!is_inline && dynamic_blocks) {
            delete[] dynamic_blocks;
        }
        
        if (new_is_inline) {
            std::memcpy(inline_blocks, new_blocks, INLINE_CAPACITY * sizeof(uint64_t));
            delete[] new_blocks;
            is_inline = true;
            dynamic_blocks = nullptr;
        } else {
            dynamic_blocks = new_blocks;
            is_inline = false;
        }
        
        capacity = new_cap;
        bit_size = new_size;
        _clean_last_block();
        return *this;
    }

    dynamic_bitset &operator>>=(std::size_t n) {
        if (n == 0) return *this;
        if (n >= bit_size) {
            bit_size = 0;
            if (capacity > 0) {
                if (!is_inline && dynamic_blocks) {
                    delete[] dynamic_blocks;
                    dynamic_blocks = nullptr;
                }
                capacity = 0;
                is_inline = true;
            }
            return *this;
        }
        
        std::size_t new_size = bit_size - n;
        if (capacity == 0) {
            bit_size = new_size;
            return *this;
        }
        std::size_t new_cap = (new_size + 63) / 64;
        
        uint64_t *new_blocks;
        bool new_is_inline = false;
        if (new_cap <= INLINE_CAPACITY) {
            new_blocks = new uint64_t[INLINE_CAPACITY];
            new_is_inline = true;
            new_cap = INLINE_CAPACITY;
        } else {
            new_blocks = new uint64_t[new_cap];
        }
        std::memset(new_blocks, 0, new_cap * sizeof(uint64_t));
        
        std::size_t block_shift = n / 64;
        std::size_t bit_shift = n % 64;
        
        uint64_t *blocks = get_blocks();
        std::size_t old_blocks = (bit_size + 63) / 64;
        for (std::size_t i = block_shift; i < old_blocks; ++i) {
            if (bit_shift == 0) {
                new_blocks[i - block_shift] = blocks[i];
            } else {
                new_blocks[i - block_shift] |= (blocks[i] >> bit_shift);
                if (i > block_shift && i - block_shift - 1 < new_cap) {
                    new_blocks[i - block_shift - 1] |= (blocks[i] << (64 - bit_shift));
                }
            }
        }
        
        if (!is_inline && dynamic_blocks) {
            delete[] dynamic_blocks;
        }
        
        if (new_is_inline) {
            std::memcpy(inline_blocks, new_blocks, INLINE_CAPACITY * sizeof(uint64_t));
            delete[] new_blocks;
            is_inline = true;
            dynamic_blocks = nullptr;
        } else {
            dynamic_blocks = new_blocks;
            is_inline = false;
        }
        
        capacity = new_cap;
        bit_size = new_size;
        _clean_last_block();
        return *this;
    }

    dynamic_bitset &set() {
        if (bit_size == 0) return *this;
        if (capacity == 0) _allocate();
        std::memset(get_blocks(), 0xFF, capacity * sizeof(uint64_t));
        _clean_last_block();
        return *this;
    }

    dynamic_bitset &flip() {
        if (bit_size == 0) return *this;
        if (capacity == 0) _allocate();
        uint64_t *blocks = get_blocks();
        std::size_t num_blocks = (bit_size + 63) / 64;
        for (std::size_t i = 0; i < num_blocks; ++i) {
            blocks[i] = ~blocks[i];
        }
        _clean_last_block();
        return *this;
    }

    dynamic_bitset &reset() {
        if (capacity > 0) {
            if (!is_inline && dynamic_blocks) {
                delete[] dynamic_blocks;
                dynamic_blocks = nullptr;
            }
            capacity = 0;
            is_inline = true;
        }
        return *this;
    }
};

} // namespace sjtu

#endif // DYNAMIC_BITSET_HPP