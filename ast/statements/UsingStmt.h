// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/values/AccessChain.h"

/**
 * using statement is just in c++, it allows users to bring symbols in current scope
 * from namespaces
 */
class UsingStmt : public ASTNode {
public:

    AccessChain chain;
    bool is_namespace = false;

    UsingStmt(
        std::vector<std::unique_ptr<ChainValue>> values,
        ASTNode* parent_node,
        bool is_namespace
    );

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void set_parent(ASTNode* new_parent) override {
        chain.set_parent(new_parent);
    }

    ASTNode *parent() override {
        return chain.parent_node;
    }

    void declare_and_link(SymbolResolver &linker) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override {
        // does nothing
    }

#endif

};