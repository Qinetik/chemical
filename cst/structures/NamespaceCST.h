// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class NamespaceCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitNamespace(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompNamespace;
    }

};