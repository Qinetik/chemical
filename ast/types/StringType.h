// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class StringType : public BaseType {
public:

    bool satisfies(ValueType type) const override {
        return type == ValueType::String;
    }

    std::string representation() const override {
        return "string";
    }

    virtual BaseType* copy() const {
        return new StringType();
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};