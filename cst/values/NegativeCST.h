// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class NegativeCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitNegative(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompNegative;
    }

};