// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class ForLoopCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitForLoop(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompForLoop;
    }

};