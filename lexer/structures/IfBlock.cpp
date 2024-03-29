// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexIfSignature() {

    if (!lexKeywordToken("if")) {
        return false;
    }

    lexWhitespaceToken();

    if (!lexOperatorToken('(')) {
        error("expected a starting parenthesis ( when lexing a if block");
        return true;
    }

    if (!lexExpressionTokens()) {
        error("expected a conditional expression when lexing a if block");
        return true;
    }

    if (!lexOperatorToken(')')) {
        error("expected a ending parenthesis ) when lexing a if block");
        return true;
    }

    return true;

}

bool Lexer::lexSingleIf() {

    if (!lexIfSignature()) {
        return false;
    } else if (has_errors) {
        return true;
    }

    if (!lexBraceBlock("if")) {
        error("expected a brace block when lexing a brace block");
        return true;
    }

    return true;

}

bool Lexer::lexIfBlockTokens() {

    if (!lexSingleIf()) {
        return false;
    } else if (has_errors) {
        return true;
    }

    // lex whitespace
    lexWhitespaceAndNewLines();

    // keep lexing else if blocks until last else appears
    while (lexKeywordToken("else")) {
        lexWhitespaceAndNewLines();
        if (provider.peek() == '{') {
            if (!lexBraceBlock("else")) {
                error("expected a brace block after the else while lexing an if statement");
            }
            return true;
        } else {
            if (lexSingleIf()) {
                lexWhitespaceToken();
            } else {
                error("expected an if statement / brace block after the 'else' but none found");
            }
        }
    }

    return true;

}