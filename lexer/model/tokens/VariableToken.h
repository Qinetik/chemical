// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "RefToken.h"

class VariableToken : public RefToken {
public:

    using RefToken::RefToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitVariableToken(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Variable;
    }

};