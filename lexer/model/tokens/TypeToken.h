// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "RefToken.h"

class TypeToken : public RefToken {
public:

    using RefToken::RefToken;

    LexTokenType type() const override {
        return LexTokenType::Type;
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitTypeToken(this);
    }

};