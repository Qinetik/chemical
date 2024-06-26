// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/types/DoubleType.h"

/**
 * @brief Class representing a double value.
 */
class DoubleValue : public Value, public DoubleType {
public:
    /**
     * @brief Construct a new DoubleValue object.
     *
     * @param value The double value.
     */
    DoubleValue(double value) : value(value) {}

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    uint64_t byte_size(bool is64Bit) {
        return 8;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

#endif

    std::unique_ptr<BaseType> create_type() override {
        return std::make_unique<DoubleType>();
    }

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { this, false };
    }

    Value *copy() override {
        return new DoubleValue(value);
    }

    double as_double() override {
        return value;
    }

    ValueType value_type() const override {
        return ValueType::Double;
    }

    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Double;
    }

    double value; ///< The double value.
};