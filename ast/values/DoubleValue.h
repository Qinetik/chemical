// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"

/**
 * @brief Class representing a double value.
 */
class DoubleValue : public Value {
public:
    /**
     * @brief Construct a new DoubleValue object.
     *
     * @param value The double value.
     */
    DoubleValue(double value) : value(value) {}

#ifdef COMPILER_BUILD
    llvm::Type * llvm_type(Codegen &gen) override;

    llvm::Value * llvm_value(Codegen &gen) override;
#endif

    Value * copy(InterpretScope& scope) override {
        return new DoubleValue(value);
    }

    double as_double() override {
        return value;
    }

    std::string representation() const override {
        std::string rep;
        rep.append(std::to_string(value));
        return rep;
    }

    void * get_value() override {
        return &value;
    }

private:
    double value; ///< The double value.
};