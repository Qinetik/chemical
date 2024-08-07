// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

bool Lexer::lexTypealiasStatement() {
    if(lexKeywordToken("typealias")) {
        unsigned start = tokens.size() - 1;
        lexWhitespaceToken();
        if(lexIdentifierToken()) {
            lexWhitespaceToken();
            if(!lexOperatorToken('=')) {
                error("expected '=' after the type tokens");
            }
            lexWhitespaceToken();
            if(!lexTypeTokens()) {
                error("expected a type after '='");
            }
            compound_collectable(start, LexTokenType::CompTypealias);
        } else {
            error("expected a type for typealias statement");
        }
        return true;
    } else {
        return false;
    }
}