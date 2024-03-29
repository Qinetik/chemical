// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "Scope.h"
#include "ast/base/Value.h"
#include <optional>

class IfStatement : public ASTNode {
public:

    /**
     * @brief Construct a new IfStatement object.
     *
     * @param condition The condition of the if statement.
     * @param ifBody The body of the if statement.
     * @param elseBody The body of the else statement (can be nullptr if there's no else part).
     */
    IfStatement(
            std::unique_ptr<Value> condition,
            Scope ifBody,
            std::vector<std::pair<std::unique_ptr<Value>, Scope>> elseIfs,
            std::optional<Scope> elseBody
    ) : condition(std::move(condition)), ifBody(std::move(ifBody)),
        elseIfs(std::move(elseIfs)), elseBody(std::move(elseBody)) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override;
#endif

    void interpret(InterpretScope &scope) override {
        if(condition->evaluated_bool(scope)) {
            InterpretScope child(&scope, scope.global, &ifBody, this);
            ifBody.interpret(child);
        } else {
            for (auto const& elseIf:elseIfs) {
                if(elseIf.first->evaluated_bool(scope)) {
                    InterpretScope child(&scope, scope.global, const_cast<Scope*>(&elseIf.second), this);
                    const_cast<Scope*>(&elseIf.second)->interpret(child);
                    return;
                }
            }
            if(elseBody.has_value()) {
                InterpretScope child(&scope, scope.global, &elseBody.value(), this);
                elseBody->interpret(child);
            }
        }
    }

    std::string representation() const override {
        std::string rep;
        rep.append("if(");
        rep.append(condition->representation());
        rep.append("){\n");
        rep.append(ifBody.representation());
        rep.append("\n}");
        int i = 0;
        while(i < elseIfs.size()) {
            rep.append("else if(");
            rep.append(elseIfs[i].first->representation());
            rep.append(") {\n");
            rep.append(elseIfs[i].second.representation());
            rep.append("\n}");
            i++;
        }
        if(elseBody.has_value()) {
            rep.append("else {\n");
            rep.append(elseBody.value().representation());
            rep.append("\n}");
        }
        return rep;
    }

    std::unique_ptr<Value> condition;
    Scope ifBody;
    std::vector<std::pair<std::unique_ptr<Value>, Scope>> elseIfs;
    std::optional<Scope> elseBody;
};