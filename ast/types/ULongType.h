// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class ULongType : public IntNType {
public:

    bool is64Bit;

    ULongType(bool is64Bit) : is64Bit(is64Bit) {

    }

    unsigned int num_bits() const override {
        return is64Bit ? 64 : 32;
    }

    bool is_unsigned() override {
        return true;
    }

    bool satisfies(Value *value) override;

    uint64_t byte_size(bool is64Bit) override {
        return is64Bit ? 8 : 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    ValueType value_type() const override {
        return ValueType::ULong;
    }

    BaseType *copy() const override {
        return new ULongType(is64Bit);
    }

};