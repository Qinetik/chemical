// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "Scope.h"
#include "LoopScope.h"
#include "ast/base/LoopASTNode.h"

class WhileLoop : public LoopASTNode {
public:

    /**
     * initializes the loop with only a condition and empty body
     * @param condition
     */
    WhileLoop(std::unique_ptr<Value> condition) : condition(std::move(condition)) {

    }

    /**
     * @brief Construct a new WhileLoop object.
     *
     * @param condition The loop condition.
     * @param body The body of the while loop.
     */
    WhileLoop(std::unique_ptr<Value> condition, LoopScope body)
            : condition(std::move(condition)), LoopASTNode(std::move(body)) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    void interpret(InterpretScope &scope) override {
        InterpretScope child(&scope, scope.global, &body, this);
        while(condition->evaluated_bool(child)){
            body.interpret(child);
            if(stoppedInterpretation) {
                stoppedInterpretation = false;
                break;
            }
        }
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override;
#endif

    void stopInterpretation() override {
        stoppedInterpretation = true;
    }

    std::string representation() const override {
        std::string ret;
        ret.append("while(");
        ret.append(condition->representation());
        ret.append(") {\n");
        ret.append(body.representation());
        ret.append("\n}");
        return ret;
    }

private:
    std::unique_ptr<Value> condition;
    bool stoppedInterpretation = false;
};