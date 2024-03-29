// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class BoolType : public BaseType {
public:

    bool satisfies(ValueType type) const override {
        return type == ValueType::Bool;
    }

    std::string representation() const override {
        return "bool";
    }

    virtual BaseType* copy() const {
        return new BoolType();
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};