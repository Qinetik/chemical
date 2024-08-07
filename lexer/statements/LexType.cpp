// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 2/19/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexLambdaTypeTokens(unsigned int start) {
    if(lexOperatorToken('(')) {
        lexParameterList();
        if(!lexOperatorToken(')')) {
            error("expected a ')' after the ')' in lambda function type");
        }
        lexWhitespaceToken();
        if(lexOperatorToken("=>")) {
            lexWhitespaceToken();
            if(!lexTypeTokens()) {
                error("expected a return type for lambda function type");
            }
        } else {
            error("expected '=>' for lambda function type");
        }
        compound_from(start, LexTokenType::CompFunctionType);
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexTypeTokens() {

    if(lexOperatorToken('[')) {
        unsigned start = tokens.size() - 1;
        if(!lexOperatorToken(']')) {
            error("expected ']' after '[' for lambda type");
            return true;
        }
        lexWhitespaceToken();
        if(!lexLambdaTypeTokens(start)) {
            error("expected a lambda type after '[]'");
        }
        return true;
    }

    if(lexLambdaTypeTokens(tokens.size())) {
        return true;
    }

    std::string type = provider.readIdentifier();
    if(type.empty()) return false;
    bool has_multiple = false;
    unsigned start = tokens.size();
    while(true) {
        if(provider.peek() == ':' && provider.peek(1) == ':') {
            tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Variable, backPosition(type.length()), type));
            lexOperatorToken("::");
            auto new_type = provider.readIdentifier();
            if(new_type.empty()) {
                error("expected an identifier after '" + type + "::' for a type");
                return true;
            } else {
                has_multiple = true;
                type = new_type;
            }
        } else {
            if(has_multiple) {
                tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Variable, backPosition(type.length()), type));
                compound_from(start, LexTokenType::CompAccessChain);
                compound_from(start, LexTokenType::CompReferencedValueType);
            } else {
                tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Type, backPosition(type.length()), type));
            }
            break;
        }
    }

    if(lexOperatorToken('<')) {
        do {
            lexWhitespaceToken();
            if(!lexTypeTokens()) {
                break;
            }
            lexWhitespaceToken();
        } while(lexOperatorToken(','));
        lexWhitespaceToken();
        if(!lexOperatorToken('>')) {
            error("expected '>' for generic type");
        }
        compound_from(start, LexTokenType::CompGenericType);
    }
    if(lexOperatorToken('[')) {
        // optional array size
        lexUnsignedIntAsNumberToken();
        if(!lexOperatorToken(']')) {
            error("expected ']' for array type");
        }
        compound_from(start, LexTokenType::CompArrayType);
    }
    while(lexOperatorToken('*')) {
        compound_from(start, LexTokenType::CompPointerType);
    }

    return true;

}