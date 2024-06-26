// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/BigIntType.h"

class BigIntValue : public IntNumValue, public BigIntType {
public:

    long long value;

    BigIntValue(long long value) : value(value) {

    }

    uint64_t byte_size(bool is64Bit) override {
        return 8;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *copy() override {
        return new BigIntValue(value);
    }

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { this, false };
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() override {
        return std::make_unique<BigIntType>();
    }

    unsigned int get_num_bits() override {
        return 64;
    }

    bool is_unsigned() override {
        return false;
    }

    int64_t get_num_value() const override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::BigInt;
    }

};