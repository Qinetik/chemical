// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

// A value that's preceded by a negative operator -value
class NegativeValue : public Value {
public:

    NegativeValue(std::unique_ptr<Value> value) : value(std::move(value)) {}

    hybrid_ptr<BaseType> get_base_type() override;

    uint64_t byte_size(bool is64Bit) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

    bool primitive() override;

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen) override;

#endif

    std::unique_ptr<BaseType> create_type() override;

    ValueType value_type() const override {
        return value->value_type();
    }

    BaseTypeKind type_kind() const override {
        return value->type_kind();
    }

    std::unique_ptr<Value> value;

};