// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class ReturnCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitReturn(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompReturn;
    }

};