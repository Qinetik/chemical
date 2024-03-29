// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"

/**
 * @brief Class representing an integer value.
 */
class IntValue : public Value {
public:

    /**
     * @brief Construct a new IntValue object.
     *
     * @param value The integer value.
     */
    IntValue(int value) : value(value) {}

    std::string representation() const override {
        std::string rep;
        rep.append(std::to_string(value));
        return rep;
    }

#ifdef COMPILER_BUILD
    llvm::Type * llvm_type(Codegen &gen) override;

    llvm::Value * llvm_value(Codegen &gen) override;

    llvm::Value* casted_llvm_value(Codegen &gen) override;
#endif

    Value * copy(InterpretScope& scope) override {
        return new IntValue(value);
    }

    int as_int() override {
        return value;
    }

    void *get_value() override {
        return &value;
    }

    ValueType value_type() const override {
        return ValueType::Int;
    }


private:
    int value; ///< The integer value.
};