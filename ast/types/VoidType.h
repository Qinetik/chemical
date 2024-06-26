// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class VoidType : public BaseType {
public:

    bool satisfies(ValueType type) override {
        return false;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Void;
    }

    bool satisfies(Value *value) override {
        return false;
    }

    bool is_same(BaseType *type) override {
        return type->kind() == kind();
    }

    virtual BaseType* copy() const {
        return new VoidType();
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) override;
#endif

};