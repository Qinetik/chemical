// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class IndexOpCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitIndexOp(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompIndexOp;
    }

};