// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "lexer/utils/AnnotationModifiers.h"
#include "lexer/utils/MacroLexers.h"

bool Lexer::lexAnnotationMacro() {

    if (!(provider.peek() == '@' || provider.peek() == '#')) {
        return false;
    }

    auto isAnnotation = provider.peek() == '@';
    chem::string macro_full_chem;
    macro_full_chem.append(provider.readCharacter());
//    auto macro_full = std::string(1, provider.readCharacter());
    provider.readAnnotationIdentifier(&macro_full_chem);
    auto macro_full = macro_full_chem.to_std_string();
    auto macro = macro_full_chem.substring(1, macro_full_chem.size()).to_std_string();

    // if it's annotation
    if (isAnnotation) {
        unsigned start = tokens.size();
        tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Annotation, backPosition(macro.size()), macro_full));
        if(lexOperatorToken('(')) {
            do {
                lexWhitespaceToken();
                if(!lexValueToken()) {
                    break;
                }
                lexWhitespaceToken();
            } while (lexOperatorToken(','));
            if(!lexOperatorToken(')')) {
                error("expected a ')' after '(' to call an annotation");
                return true;
            }
            compound_from(start, LexTokenType::CompAnnotation);
        }
        auto found = AnnotationModifiers.find(macro);
        if (found != AnnotationModifiers.end()) {
            found->second(this, tokens[start].get());
        }
        return true;
    }

    auto start = tokens.size();

    tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Identifier, backPosition(macro.size()), macro_full));

    lexWhitespaceToken();
    if (lexOperatorToken('{')) {

        lexWhitespaceAndNewLines();

        // check if this macro has a lexer defined
        auto macro_lexer = MacroHandlers.find(macro);
        if (macro_lexer != MacroHandlers.end()) {
            macro_lexer->second(this);
        } else {
            auto lex_func = binder->provide_lex_func(macro);
            if(lex_func && cbi) {
                lex_func(cbi);
            } else {
                auto current = position();
                auto content = provider.readUntil('}');
                tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::RawToken, current, std::move(content)));
            }
        }

    } else {
        error("expected '{' after the macro " + macro);
        return true;
    }

    lexWhitespaceAndNewLines();
    if (!lexOperatorToken('}')) {
        error("expected '}' for the macro " + macro + " ending");
        return true;
    }

    compound_from(start, LexTokenType::CompMacro);
    return true;

}