// Copyright (c) Qinetik 2024.

#include "ChainValue.h"
#include "ast/values/StructValue.h"
#include "ast/statements/Assignment.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/statements/Return.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/PointerType.h"
#include "ast/values/UIntValue.h"
#include "ast/values/AccessChain.h"
#include "ast/types/ArrayType.h"
#include <ranges>
#include "preprocess/RepresentationVisitor.h"
#include <sstream>

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "ast/structures/StructMember.h"

llvm::AllocaInst* Value::llvm_allocate_with(Codegen& gen, llvm::Value* value, llvm::Type* type) {
    auto x = gen.builder->CreateAlloca(type, nullptr);
    gen.builder->CreateStore(value, x);
    return x;
}

llvm::AllocaInst *Value::llvm_allocate(Codegen &gen, const std::string &identifier) {
    return llvm_allocate_with(gen, llvm_value(gen), llvm_type(gen));
}

llvm::GlobalVariable* Value::llvm_global_variable(Codegen& gen, bool is_const, const std::string& name) {
    return new llvm::GlobalVariable(*gen.module, llvm_type(gen), is_const, llvm::GlobalValue::LinkageTypes::PrivateLinkage, (llvm::Constant*) llvm_value(gen), name);
}

llvm::AllocaInst* ChainValue::access_chain_allocate(Codegen& gen, std::vector<std::unique_ptr<ChainValue>>& values, unsigned int until) {
    return llvm_allocate_with(gen, values[until]->access_chain_value(gen, values, until), values[until]->llvm_type(gen));
}

llvm::Value* Value::get_element_pointer(
        Codegen& gen,
        llvm::Type* in_type,
        llvm::Value* ptr,
        std::vector<llvm::Value *>& idxList,
        unsigned int index
) {
    idxList.emplace_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), index));
    return gen.builder->CreateGEP(in_type, ptr, idxList, "", gen.inbounds);
}

unsigned int Value::store_in_struct(
        Codegen& gen,
        StructValue* parent,
        llvm::Value* allocated,
        std::vector<llvm::Value *> idxList,
        unsigned int index
) {
    auto elementPtr = Value::get_element_pointer(gen, parent->llvm_type(gen), allocated, idxList, index);
    gen.builder->CreateStore(llvm_value(gen), elementPtr);
    return index + 1;
}

unsigned int Value::store_in_array(
        Codegen& gen,
        ArrayValue* parent,
        llvm::AllocaInst* ptr,
        std::vector<llvm::Value *> idxList,
        unsigned int index
) {
    auto elementPtr = Value::get_element_pointer(gen, parent->llvm_type(gen), ptr, idxList, index);
    gen.builder->CreateStore(llvm_value(gen), elementPtr);
    return index + 1;
}

void Value::destruct(Codegen& gen, std::vector<std::pair<Value*, llvm::Value*>>& destructibles) {
    for(auto& val : std::ranges::reverse_view(destructibles)) {
        val.first->llvm_destruct(gen, val.second);
    }
}

llvm::Value* ChainValue::access_chain_value(Codegen &gen, std::vector<std::unique_ptr<ChainValue>>& values, unsigned int until, std::vector<std::pair<Value*, llvm::Value*>>& destructibles) {
    if(until == 0) return values[0]->llvm_value(gen);
    return gen.builder->CreateLoad(values[until]->llvm_type(gen), access_chain_pointer(gen, values, destructibles, until), "acc");
}

llvm::Value* create_gep(Codegen &gen, std::vector<std::unique_ptr<ChainValue>>& values, unsigned index, llvm::Value* pointer, std::vector<llvm::Value*>& idxList) {
    const auto parent = values[index].get();
    auto type_kind = parent->type_kind();
    if(type_kind == BaseTypeKind::Array && parent->linked_node() && parent->linked_node()->as_func_param()) {
        auto type = parent->create_type();
        auto arr_type = (ArrayType*) type.get();
        return gen.builder->CreateGEP(arr_type->elem_type->llvm_type(gen), pointer, idxList, "", gen.inbounds);
    } else if(type_kind == BaseTypeKind::Pointer) {
        auto ty = parent->create_type();
        return gen.builder->CreateGEP(((PointerType*) (ty.get()))->type->llvm_chain_type(gen, values, index), pointer, idxList, "", gen.inbounds);
    } else {
        return gen.builder->CreateGEP(parent->llvm_chain_type(gen, values, index), pointer, idxList, "", gen.inbounds);
    }
}

// stored pointer into a variable, that must be loaded, before using
bool is_stored_pointer(Value* value) {
    auto linked = value->linked_node();
    if(!linked) return false;
    if(linked->as_struct_member()) {
        return linked->as_struct_member()->type->is_pointer();
    } else if(linked->as_var_init() && !linked->as_var_init()->is_const) {
        auto kind = linked->as_var_init()->type_kind();
        return kind == BaseTypeKind::Pointer || kind == BaseTypeKind::String;
    }
    return false;
}

llvm::Value* ChainValue::access_chain_pointer(
        Codegen &gen,
        std::vector<std::unique_ptr<ChainValue>>& values,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
        unsigned int until
) {

    if(until == 0) {
        return values[0]->llvm_pointer(gen);
    }

    const auto last = values[until].get();
    const auto last_func_call = last->as_func_call();
    if(last_func_call) {
        const auto func_decl = last_func_call->safe_linked_func();
        if(func_decl && func_decl->has_annotation(AnnotationKind::CompTime)) {
            auto& ret_value = gen.eval_comptime(last_func_call, func_decl);
            if(ret_value) {
                return ret_value->llvm_pointer(gen);
            } else {
                return nullptr;
            }
        }
    }

    unsigned parent_index = 0;
    Value* parent = values[0].get();
    llvm::Value* pointer = parent->llvm_pointer(gen);

    unsigned i = 1;
    unsigned j = 1;
    while(j <= until) {
        const auto func_call = values[j]->as_func_call();
        if(func_call) {
            pointer = func_call->access_chain_value(gen, values, j, destructibles);
            parent = func_call;
            parent_index = j;
            if(j + 1 <= until) {
                destructibles.emplace_back(parent, pointer);
            }
            i = j + 1;
        }
        j++;
    }

    if(is_stored_pointer(parent) && i <= until) {
        pointer = gen.builder->CreateLoad(parent->llvm_type(gen), pointer);
    }

    std::vector<llvm::Value*> idxList;

    while (i <= until) {
        if(i + 1 <= until && values[i]->is_pointer()) {
            llvm::Value* gep;
            if(idxList.empty()) {
                gep = pointer;
            } else {
                gep = create_gep(gen, values, parent_index, pointer, idxList);
            }
            pointer = gen.builder->CreateLoad(values[i]->llvm_type(gen), gep);
            parent = values[i].get();
            parent_index = i;
            idxList.clear();
        } else {
            if (!values[i]->add_member_index(gen, values[i - 1].get(), idxList)) {
                gen.error("couldn't add member index for fragment '" + values[i]->representation() +
                          "' in access chain '" + representation() + "'");
            }
        }
        i++;
    }
    return create_gep(gen, values, parent_index, pointer, idxList);
}

void Value::llvm_conditional_branch(Codegen& gen, llvm::BasicBlock* then_block, llvm::BasicBlock* otherwise_block) {
    gen.CreateCondBr(llvm_value(gen), then_block, otherwise_block);
}

#endif

hybrid_ptr<BaseType> Value::get_base_type() {
    throw std::runtime_error("get_base_type called on bare Value with type : " + std::to_string((unsigned int) value_type()));
}

std::unique_ptr<BaseType> Value::create_type() {
    return nullptr;
}

std::unique_ptr<BaseType> ChainValue::create_type(std::vector<std::unique_ptr<ChainValue>>& chain, unsigned int index) {
    return create_type();
}

std::unique_ptr<Value> Value::create_evaluated_value(InterpretScope& scope) {
    return nullptr;
}

hybrid_ptr<Value> Value::evaluated_chain_value(InterpretScope& scope, Value* parent) {
    throw std::runtime_error("evaluated chain value called on base value");
}

unsigned Value::as_uint() {
    return ((UIntValue*) this)->value;
}

std::string Value::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    accept(&visitor);
    return ostring.str();
}

hybrid_ptr<BaseType> Value::get_child_type() {
    auto base_type = get_base_type();
    if(base_type.get_will_free()) {
        return hybrid_ptr<BaseType> { base_type->create_child_type().release() };
    } else {
        return base_type->get_child_type();
    }
}

hybrid_ptr<BaseType> Value::get_pure_type() {
    // TODO check this method, it should free types properly
    auto base_type = get_base_type();
    auto pure_type = base_type->get_pure_type();
    if(pure_type.get() == base_type.get()) {
        return base_type;
    } else {
        return pure_type;
    }
}

bool Value::should_build_chain_type(std::vector<std::unique_ptr<Value>>& chain, unsigned index) {
    ASTNode* linked;
    VariablesContainer* union_container = nullptr;
    while(index < chain.size()) {
        linked = chain[index]->linked_node();
        if(linked && (linked->as_union_def() || linked->as_unnamed_union())) {
            union_container = linked->as_variables_container();
        } else if(union_container && linked != union_container->largest_member()) {
            return true;
        }
        index++;
    }
    return false;
}

int16_t ChainValue::set_generic_iteration() {
    const auto type = create_type();
    if (type) {
        const auto prev_itr = type->set_generic_iteration(type->get_generic_iteration());
        if(prev_itr > -2) {
            return prev_itr;
        }
    }
    return -2;
}

void Value::link(SymbolResolver& linker, VarInitStatement* stmnt) {
    link(linker, stmnt->value.value());
}

void ChainValue::link(
    SymbolResolver& linker,
    ChainValue* parent,
    std::vector<std::unique_ptr<ChainValue>>& values,
    unsigned index
) {
    if(parent) {
        find_link_in_parent(parent, linker);
    } else {
        link(linker, (std::unique_ptr<Value>&) values[index]);
    }
}

void ChainValue::relink_parent(ChainValue* parent) {
    throw std::runtime_error("relink_parent called on base chain value");
}

void Value::link(SymbolResolver& linker, AssignStatement* stmnt, bool lhs) {
    if(lhs) {
        link(linker, stmnt->lhs);
    } else {
        link(linker, stmnt->value);
    }
}

void Value::link(SymbolResolver& linker, StructValue* value, const std::string& name) {
    link(linker, value->values[name]);
}

void Value::link(SymbolResolver& linker, FunctionCall* call, unsigned int index) {
    link(linker, call->values[index]);
}

void Value::link(SymbolResolver& linker, ReturnStatement* returnStmt) {
    link(linker, returnStmt->value.value());
}

Value::~Value() = default;