// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class PointerTypeCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitPointerType(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompPointerType;
    }

};