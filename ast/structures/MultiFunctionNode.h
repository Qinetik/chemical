// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

struct OverridableFuncHandlingResult {
    // duplicate nodes already present on the map, with same name
    std::vector<ASTNode*> duplicates;
    // any new multi function node, that should be handled
    MultiFunctionNode* new_multi_func_node;
};

/**
 * multi function node is created when two functions with same name exist
 * however parameter types may be different, this allows us c++ like behavior of
 * overriding, multi function node only exists during symbol resolution
 */
class MultiFunctionNode : public ASTNode {
public:

    std::string name;
    std::vector<FunctionDeclaration*> functions;

    /**
     * constructor
     */
    explicit MultiFunctionNode(std::string name);

    FunctionDeclaration* func_for_call(std::vector<std::unique_ptr<Value>>& args);

    MultiFunctionNode *as_multi_func_node() override {
        return this;
    }

    void accept(Visitor *visitor) override {
        // don't do anything
    }

    ASTNode *parent() override {
        return nullptr;
    }

    void declare_and_link(SymbolResolver &linker) override;

};

OverridableFuncHandlingResult handle_name_overridable_function(
        const std::string& name,
        ASTNode* previous_node,
        FunctionDeclaration* declaration
);