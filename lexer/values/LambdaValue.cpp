// Copyright (c) Qinetik 2024.

#include "lexer/Lexer.h"

void Lexer::lexTypeList() {
    do {
        lexWhitespaceToken();
        if (!lexTypeTokens()) {
            break;
        }
        lexWhitespaceToken();
    } while (lexOperatorToken(','));
}

void Lexer::lexIdentifierList() {
    do {
        lexWhitespaceToken();
        if (!lexIdentifierToken()) {
            break;
        }
        lexWhitespaceToken();
    } while (lexOperatorToken(','));
}

void Lexer::lexLambdaAfterParamsList(unsigned int start) {
    lexWhitespaceToken();

    if (!lexOperatorToken("=>")) {
        error("expected '=>' for a lambda");
    }

    lexWhitespaceToken();

    if (!(lexBraceBlock("lambda") || lexExpressionTokens())) {
        error("expected lambda body");
    }

    compound_from(start, LexTokenType::CompLambda);
}

bool Lexer::lexLambdaValue() {
    if (lexOperatorToken('[')) {

        auto start = tokens.size() - 1;

        do {
            lexWhitespaceAndNewLines();
            bool lexed_amp = lexOperatorToken('&');
            if (!lexVariableToken()) {
                if(lexed_amp) {
                    error("expected identifier after '&'");
                }
                break;
            }
            lexWhitespaceToken();
        } while (lexOperatorToken(','));

        if (!lexOperatorToken(']')) {
            error("expected ']' after lambda function capture list");
        }

        if (!lexOperatorToken('(')) {
            error("expected '(' for lambda parameter list");
        }

        lexParameterList(true, false);

        lexNewLineChars();

        if (!lexOperatorToken(')')) {
            error("expected ')' after the lambda parameter list");
        }

        lexLambdaAfterParamsList(start);

        return true;
    } else {
        return false;
    }
}