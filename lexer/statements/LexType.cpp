// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 2/19/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/TypeToken.h"

bool Lexer::lexTypeTokens() {
    auto type = lexAnything([&] () -> bool {
        return std::isalpha(provider.peek());
    });
    if (!type.empty()) {
        tokens.emplace_back(std::make_unique<TypeToken>(backPosition(type.length()), type));
        if(lexOperatorToken('<')) {
            if(!lexTypeTokens()) {
                error("expected a type within '<' '>' for generic type");
            }
            if(!lexOperatorToken('>')) {
                error("expected '>' for generic type");
            }
        } else if(lexOperatorToken('[')) {
            // optional array size
            lexUnsignedIntAsNumberToken();
            if(!lexOperatorToken(']')) {
                error("expected ']' for array type");
            }
        }
        lexOperatorToken('*');
        return true;
    } else {
        return false;
    }
}