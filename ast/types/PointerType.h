// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include <memory>

class PointerType : public BaseType {
public:

    std::unique_ptr<BaseType> type;

    PointerType(std::unique_ptr<BaseType> type) : type(std::move(type)) {

    }

    bool satisfies(ValueType value_type) const override {
        return type->satisfies(value_type);
    }

    PointerType* pointer_type() override {
        return this;
    }

    std::string representation() const override {
        return  type->representation() + "*";
    }

    virtual BaseType* copy() const {
        return new PointerType(std::unique_ptr<BaseType>(type->copy()));
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};