// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class ImplCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitImpl(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompImpl;
    }

};