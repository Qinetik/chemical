// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 25/02/2024.
//

#pragma once

#include "LexToken.h"

class CharToken : public LexToken {
public:

    using LexToken::LexToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitCharToken(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Char;
    }

};