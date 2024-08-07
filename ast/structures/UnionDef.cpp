// Copyright (c) Qinetik 2024.

#include "UnnamedUnion.h"
#include "UnionDef.h"
#include "FunctionDeclaration.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/ReferencedType.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void UnionDef::code_gen(Codegen &gen) {
    for (auto &function: functions()) {
        function->code_gen_union(gen, this);
    }
}

#endif

UnnamedUnion::UnnamedUnion(std::string name, ASTNode* parent_node) : BaseDefMember(std::move(name)), parent_node(parent_node) {

}

hybrid_ptr<BaseType> UnnamedUnion::get_value_type() {
    return hybrid_ptr<BaseType> { this, false };
}

UnionDef::UnionDef(std::string name, ASTNode* parent_node) : ExtendableMembersContainerNode(std::move(name)), parent_node(parent_node) {

}

BaseType *UnionDef::copy() const {
    return new ReferencedType(name, (ASTNode*) this);
}

VariablesContainer *UnionDef::copy_container() {
    auto container = new UnionDef(name, parent_node);
    for(auto& variable : variables) {
        container->variables[variable.first] = std::unique_ptr<BaseDefMember>(variable.second->copy_member());
    }
    return container;
}

BaseType *UnnamedUnion::copy() const {
    return new ReferencedType(name, (ASTNode*) this);
}

std::unique_ptr<BaseType> UnionDef::create_value_type() {
    return std::unique_ptr<BaseType>(copy());
}

hybrid_ptr<BaseType> UnionDef::get_value_type() {
    return hybrid_ptr<BaseType> { this, false };
}

void UnionDef::declare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}

void UnionDef::declare_and_link(SymbolResolver &linker) {
    MembersContainer::declare_and_link(linker);
}

void UnnamedUnion::declare_and_link(SymbolResolver &linker) {
    linker.scope_start();
    VariablesContainer::declare_and_link(linker);
    linker.scope_end();
    linker.declare(name, this);
}

BaseDefMember *UnnamedUnion::copy_member() {
    auto unnamed = new UnnamedUnion(name, parent_node);
    for(auto& variable : variables) {
        unnamed->variables[variable.first] = std::unique_ptr<BaseDefMember>(variable.second->copy_member());
    }
    return unnamed;
}

VariablesContainer *UnnamedUnion::copy_container() {
    return (VariablesContainer*) copy_member();
}