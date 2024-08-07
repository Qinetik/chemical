// Copyright (c) Qinetik 2024.

#include "IndexOperator.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/ArrayType.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Value *IndexOperator::elem_pointer(Codegen &gen, llvm::Type *type, llvm::Value *ptr) {
    std::vector<llvm::Value *> idxList;
    if (type->isArrayTy()) {
        idxList.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    }
    for (auto &value: values) {
        idxList.emplace_back(value->llvm_value(gen));
    }
    idxList.shrink_to_fit();
    return gen.builder->CreateGEP(type, ptr, idxList, "", gen.inbounds);
}

llvm::Value *IndexOperator::elem_pointer(Codegen &gen, ASTNode *arr) {
    return elem_pointer(gen, arr->llvm_type(gen), arr->llvm_pointer(gen));
}

llvm::Value *IndexOperator::llvm_pointer(Codegen &gen) {
    return elem_pointer(gen, parent_val->linked_node());
}

llvm::Value *IndexOperator::llvm_value(Codegen &gen) {
    return gen.builder->CreateLoad(llvm_type(gen), elem_pointer(gen, parent_val->linked_node()),"idx_op");
}

llvm::Value *IndexOperator::access_chain_pointer(
        Codegen &gen,
        std::vector<std::unique_ptr<ChainValue>> &chain_values,
        std::vector<std::pair<Value *, llvm::Value *>> &destructibles,
        unsigned int until
) {
    auto pure_type = parent_val->get_pure_type();
    if(pure_type->is_pointer()) {
        auto parent_value = parent_val->access_chain_value(gen, chain_values, until - 1);
        auto child_type = pure_type->get_child_type();
        return elem_pointer(gen, child_type->llvm_type(gen), parent_value);
    } else {
        return ChainValue::access_chain_pointer(gen, chain_values, destructibles, until);
    }
}

bool IndexOperator::add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) {
    if (parent->value_type() == ValueType::Array && indexes.empty() && (parent->linked_node() == nullptr || parent->linked_node()->as_func_param() == nullptr )) {
        indexes.push_back(gen.builder->getInt32(0));
    }
    for (auto &value: values) {
        indexes.emplace_back(value->llvm_value(gen));
    }
    return true;
}

bool IndexOperator::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return create_type()->linked_node()->add_child_index(gen, indexes, name);
}

llvm::Type *IndexOperator::llvm_type(Codegen &gen) {
    return parent_val->create_type()->create_child_type()->llvm_type(gen);
}

llvm::Type *IndexOperator::llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &chain, unsigned int index) {
    return parent_val->create_type()->create_child_type()->llvm_chain_type(gen, chain, index);
}

llvm::FunctionType *IndexOperator::llvm_func_type(Codegen &gen) {
    return create_type()->llvm_func_type(gen);
}

#endif

std::unique_ptr<BaseType> IndexOperator::create_type() {
    return parent_val->create_type()->create_child_type();
}

hybrid_ptr<BaseType> IndexOperator::get_base_type() {
    return parent_val->get_child_type();
}

void IndexOperator::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) {
    for(auto& value : values) {
        value->link(linker, value);
    }
}

ASTNode *IndexOperator::linked_node() {
    // TODO use get type instead
    auto value_type = parent_val->create_type();
    return ((ArrayType *) value_type.get())->elem_type->linked_node();
}

Value *IndexOperator::find_in(InterpretScope &scope, Value *parent) {
    auto value = values[0].get();
    if (values.size() > 1) {
        scope.error("Index operator only supports a single integer index at the moment");
    }
#ifdef DEBUG
    try {
        return parent->index(scope, value->evaluated_value(scope)->as_int());
    } catch (...) {
        std::cerr << "[InterpretError] index operator only support's integer indexes at the moment";
    }
#endif
    parent->index(scope, value->evaluated_value(scope)->as_int());
    return nullptr;
}

void IndexOperator::find_link_in_parent(ChainValue *parent, SymbolResolver &resolver) {
    parent_val = parent;
    for(auto& value : values) {
        value->link(resolver, value);
    }
}

void IndexOperator::relink_parent(ChainValue *parent) {
    parent_val = parent;
}