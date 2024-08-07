// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class Value;

class ThrowStatement : public ASTNode {
public:

    std::unique_ptr<Value> value;
    ASTNode* parent_node;

    /**
     * constructor
     */
    ThrowStatement(std::unique_ptr<Value> value, ASTNode* parent_node);

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

};