// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexContinueStatement() {
    return lexKeywordToken("continue");
}

bool Lexer::lexBreakStatement() {
    return lexKeywordToken("break");
}

bool Lexer::lexForBlockTokens() {

    if (!lexKeywordToken("for")) {
        return false;
    }

    // whitespace
    lexWhitespaceToken();

    // start parenthesis
    if(!lexOperatorToken('(')) {
        error("expected a starting parenthesis ( in a for block");
    }

    // lex strict initialization
    lexVarInitializationTokens(false);

    // lex ;
    if(!lexOperatorToken(';')){
        error("expected semicolon ; after the initialization in for block");
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // lex conditional expression
    lexExpressionTokens();

    if(!lexOperatorToken(';')){
        error("expected semicolon ; after the condition in for block");
        return true;
    }

    // whitespace
    lexWhitespaceToken();

    // lex assignment token
    if(!lexAssignmentTokens()) {
        error("missing assignment in for block");
        return true;
    }

    // end parenthesis
    if(!lexOperatorToken(')')) {
        error("expected a closing parenthesis ) in a for block");
    }

    // { statement(s) } with continue & break support
    isLexContinueStatement = true;
    isLexBreakStatement = true;
    if(!lexBraceBlock("forloop")) {
        error("expected a brace block in a for block");
    }
    isLexContinueStatement = false;
    isLexBreakStatement = false;

    return true;

}