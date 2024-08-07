// Copyright (c) Qinetik 2024.

#include "ASTNode.h"
#include "BaseType.h"
#include "Value.h"
#include "preprocess/RepresentationVisitor.h"
#include <sstream>

#if !defined(DEBUG) && defined(COMPILER_BUILD)
#include "compiler/Codegen.h"
#endif

std::string ASTNode::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    accept(&visitor);
    return ostring.str();
}

uint64_t ASTNode::byte_size(bool is64Bit) {
    auto holdingType = holding_value_type();
    if(holdingType) return holdingType->byte_size(is64Bit);
    auto holdingValue = holding_value();
    if(holdingValue) return holdingValue->byte_size(is64Bit);
    throw std::runtime_error("unknown byte size for linked node");
}

ASTNode* ASTNode::root_parent() {
    ASTNode* current = this;
    while(true) {
        const auto parent = current->parent();
        if(parent) {
            current = parent;
        } else {
            return current;
        }
    };
}

void ASTNode::set_parent(ASTNode* parent) {
#ifdef DEBUG
    throw std::runtime_error("set_parent called on base ast node");
#endif
}

#ifdef COMPILER_BUILD

llvm::Value *ASTNode::llvm_pointer(Codegen &gen) {
#ifdef DEBUG
    throw std::runtime_error("llvm_pointer called on bare ASTNode, with representation" + representation());
#else
    gen.early_error("ASTNode::llvm_pointer called, on node : " + representation());
    return nullptr;
#endif
};

llvm::Type *ASTNode::llvm_elem_type(Codegen &gen) {
#ifdef DEBUG
    throw std::runtime_error("llvm_elem_type called on bare ASTNode, with representation" + representation());
#else
    gen.early_error("ASTNode::llvm_elem_type called, on node : " + representation());
    return nullptr;
#endif
};

void ASTNode::code_gen(Codegen &gen) {
#ifdef DEBUG
    throw std::runtime_error("ASTNode code_gen called on bare ASTNode, with representation : " + representation());
#else
    gen.early_error("ASTNode::code_gen called, on node : " + representation());
#endif
}

bool ASTNode::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
#ifdef DEBUG
    throw std::runtime_error("add_child_index called on a ASTNode");
#else
    gen.early_error("ASTNode::add_child_index called, on node : " + representation());
    return false;
#endif
}

llvm::Value *ASTNode::llvm_load(Codegen &gen) {
#ifdef DEBUG
    throw std::runtime_error("llvm_load called on a ASTNode");
#else
    gen.early_error("ASTNode::llvm_load called, on node : " + representation());
    return nullptr;
#endif
}

void ASTNode::code_gen_generic(Codegen &gen) {
#ifdef DEBUG
    throw std::runtime_error("llvm_load called on a ASTNode");
#else
    gen.early_error("ASTNode::llvm_load called, on node : " + representation());
#endif
}

#endif

std::unique_ptr<BaseType> ASTNode::create_value_type() {
    return nullptr;
}

hybrid_ptr<BaseType> ASTNode::get_value_type() {
    return hybrid_ptr<BaseType> { nullptr, false };
}

ASTNode::~ASTNode() = default;