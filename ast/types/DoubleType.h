// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class DoubleType : public BaseType {
public:

    bool satisfies(ValueType type) const override {
        return type == ValueType::Double;
    }

    std::string representation() const override {
        return "double";
    }

    virtual BaseType* copy() const {
        return new DoubleType();
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};