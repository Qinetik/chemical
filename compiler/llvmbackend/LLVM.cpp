// Copyright (c) Qinetik 2024.

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include "ast/base/ASTNode.h"
#include "ast/types/AnyType.h"
#include "ast/values/RetStructParamValue.h"
#include "ast/types/ArrayType.h"
#include "ast/types/GenericType.h"
#include "ast/types/BoolType.h"
#include "ast/types/CharType.h"
#include "ast/types/UCharType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/FloatType.h"
#include "ast/types/IntNType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferencedType.h"
#include "ast/types/UnionType.h"
#include "ast/types/StringType.h"
#include "ast/types/StructType.h"
#include "ast/types/VoidType.h"
#include "ast/values/BoolValue.h"
#include "ast/values/CharValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NotValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/SizeOfValue.h"
#include "ast/structures/Namespace.h"
#include "ast/values/StringValue.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/Expression.h"
#include "ast/values/CastedValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/statements/Continue.h"
#include "ast/statements/Return.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/Break.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Import.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariablesContainer.h"
#include "ast/structures/MembersContainer.h"
#include "ast/statements/ThrowStatement.h"
#include "ast/statements/DeleteStmt.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/FunctionType.h"

// -------------------- Types

llvm::Type *AnyType::llvm_type(Codegen &gen) {
    throw std::runtime_error("llvm_type called on any type");
}

llvm::Type *ArrayType::llvm_type(Codegen &gen) {
    return llvm::ArrayType::get(elem_type->llvm_type(gen), array_size);
}

llvm::Type *ArrayType::llvm_param_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Type *BoolType::llvm_type(Codegen &gen) {
    return gen.builder->getInt1Ty();
}

llvm::Type *CharType::llvm_type(Codegen &gen) {
    return gen.builder->getInt8Ty();
}

llvm::Type *UCharType::llvm_type(Codegen &gen) {
    return gen.builder->getInt8Ty();
}

llvm::Type *DoubleType::llvm_type(Codegen &gen) {
    return gen.builder->getDoubleTy();
}

llvm::Type *FloatType::llvm_type(Codegen &gen) {
    return gen.builder->getFloatTy();
}

llvm::Type *IntNType::llvm_type(Codegen &gen) {
    auto ty = gen.builder->getIntNTy(num_bits());
    if(!ty) {
        gen.error("Couldn't get intN type for int:" + std::to_string(num_bits()));
    }
    return ty;
}

llvm::Type *PointerType::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Type *PointerType::llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) {
    if(index == values.size() - 1) {
        return gen.builder->getPtrTy();
    } else {
        return type->llvm_chain_type(gen, values, index);
    }
}

llvm::Type *ReferencedType::llvm_type(Codegen &gen) {
    return linked->llvm_type(gen);
}

llvm::Type *ReferencedType::llvm_param_type(Codegen &gen) {
    return linked->llvm_param_type(gen);
}

llvm::Type *ReferencedType::llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) {
    return linked->llvm_chain_type(gen, values, index);
}

llvm::Type *GenericType::llvm_type(Codegen &gen) {
    const auto gen_struct = referenced->get_generic_struct();
    const auto prev_itr = gen_struct->active_iteration;
    gen_struct->set_active_iteration(generic_iteration);
    auto type = referenced->llvm_type(gen);
    gen_struct->set_active_iteration(prev_itr);
    return type;
}

llvm::Type *GenericType::llvm_param_type(Codegen &gen) {
    const auto gen_struct = referenced->get_generic_struct();
    const auto prev_itr = gen_struct->active_iteration;
    gen_struct->set_active_iteration(generic_iteration);
    auto type = referenced->llvm_param_type(gen);
    gen_struct->set_active_iteration(prev_itr);
    return type;
}

llvm::Type *StringType::llvm_type(Codegen &gen) {
    return gen.builder->getInt8PtrTy();
}

llvm::Type *StructType::with_elements_type(Codegen &gen, const std::vector<llvm::Type *>& elements, bool anonymous) {
    if(anonymous) {
        return llvm::StructType::get(*gen.ctx, elements);
    }
    auto stored = llvm_stored_type();
    if(!stored) {
        auto new_stored = llvm::StructType::create(*gen.ctx, elements, struct_name());
        llvm_store_type(new_stored);
        return new_stored;
    }
    return stored;
}

llvm::Type *StructType::llvm_type(Codegen &gen) {
    auto container = variables_container();
    return with_elements_type(gen, container->elements_type(gen), is_anonymous());
}

llvm::Type *StructType::llvm_param_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Type *StructType::llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) {
    auto container = variables_container();
    return with_elements_type(gen, container->elements_type(gen, values, index), true);
}

llvm::Type *UnionType::llvm_type(Codegen &gen) {
    auto container = variables_container();
    auto largest = container->largest_member();
    if(!largest) {
        gen.error("Couldn't determine the largest member of the union with name " + union_name());
        return nullptr;
    }
    auto stored = llvm_union_get_stored_type();
    if(!stored) {
        std::vector<llvm::Type*> members {largest->llvm_type(gen)};
        if(is_anonymous()) {
            return llvm::StructType::get(*gen.ctx, members);
        }
        stored = llvm::StructType::create(*gen.ctx, members, "union." + union_name());
        llvm_union_type_store(stored);
        return stored;
    }
    return stored;
}

llvm::Type *UnionType::llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) {
    auto container = variables_container();
    if(index + 1 < values.size()) {
        auto linked = values[index + 1]->linked_node();
        if(linked) {
            for (auto &member: container->variables) {
                if (member.second.get() == linked) {
                    std::vector<llvm::Type *> struct_type{member.second->llvm_chain_type(gen, values, index + 1)};
                    return llvm::StructType::get(*gen.ctx, struct_type);
                }
            }
        }
    }
    return llvm_type(gen);
}

llvm::Type *VoidType::llvm_type(Codegen &gen) {
    return gen.builder->getVoidTy();
}

// ------------------------------ Values

llvm::Type *BoolValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt1Ty();
}

llvm::Value *BoolValue::llvm_value(Codegen &gen) {
    return gen.builder->getInt1(value);
}

llvm::Type *CharValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt8Ty();
}

llvm::Value *CharValue::llvm_value(Codegen &gen) {
    return gen.builder->getInt8((int) value);
}

llvm::Type *DoubleValue::llvm_type(Codegen &gen) {
    return gen.builder->getDoubleTy();
}

llvm::Value *DoubleValue::llvm_value(Codegen &gen) {
    return llvm::ConstantFP::get(llvm_type(gen), value);
}

llvm::Type * FloatValue::llvm_type(Codegen &gen) {
    return gen.builder->getFloatTy();
}

llvm::Value * FloatValue::llvm_value(Codegen &gen) {
    return llvm::ConstantFP::get(llvm_type(gen), llvm::APFloat(value));
}

llvm::Type *IntNumValue::llvm_type(Codegen &gen) {
    return gen.builder->getIntNTy(get_num_bits());
}

llvm::Value *IntNumValue::llvm_value(Codegen &gen) {
    return gen.builder->getIntN(get_num_bits(), get_num_value());
}

llvm::Value *NegativeValue::llvm_value(Codegen &gen) {
    return gen.builder->CreateNeg(value->llvm_value(gen));
}

llvm::Value *NotValue::llvm_value(Codegen &gen) {
    return gen.builder->CreateNot(value->llvm_value(gen));
}

llvm::Value *NullValue::llvm_value(Codegen &gen) {
    auto ptrType = llvm::PointerType::get(llvm::IntegerType::get(*gen.ctx, 32), 0);
    return llvm::ConstantPointerNull::get(ptrType);
}

llvm::Type *StringValue::llvm_type(Codegen &gen) {
    if(is_array) {
        return llvm::ArrayType::get(gen.builder->getInt8Ty(), length);
    } else {
        return gen.builder->getInt8PtrTy();
    }
}

llvm::Value *StringValue::llvm_value(Codegen &gen) {
    if(is_array) {
        std::vector<llvm::Constant*> arr;
        for(auto c : value) {
            arr.emplace_back(gen.builder->getInt8(c));
        }
        int remaining = length - value.size();
        while(remaining > 0) {
            arr.emplace_back(gen.builder->getInt8('\0'));
            remaining--;
        }
        auto array_type = (llvm::ArrayType*) llvm_type(gen);
        auto initializer = llvm::ConstantArray::get(array_type, arr);
        return new llvm::GlobalVariable(
            *gen.module,
            array_type,
            true,
            llvm::GlobalValue::LinkageTypes::PrivateLinkage,
            initializer
        );
    } else {
        return gen.builder->CreateGlobalStringPtr(value);
    }
}

llvm::AllocaInst *StringValue::llvm_allocate(Codegen &gen, const std::string &identifier) {
    if(is_array) {
        auto alloc = gen.builder->CreateAlloca(llvm_type(gen), nullptr);
        auto arr = llvm_value(gen);
        gen.builder->CreateMemCpy(alloc, llvm::MaybeAlign(), arr, llvm::MaybeAlign(), length);
        return alloc;
    } else {
        return Value::llvm_allocate(gen, identifier);
    }
}

llvm::GlobalVariable * StringValue::llvm_global_variable(Codegen &gen, bool is_const, const std::string &name) {
    if(!is_const) {
        gen.error("Global string variables aren't supported at the moment");
    }
    // TODO global constant string must have type pointer to array
    // because it returns an array pointer, and we must take [0] from it to reach first pointer
    return gen.builder->CreateGlobalString(value, name, 0, gen.module.get());
}

llvm::Type *VariableIdentifier::llvm_type(Codegen &gen) {
    return linked->llvm_type(gen);
}

llvm::Type *VariableIdentifier::llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &chain, unsigned int index) {
    return linked->llvm_chain_type(gen, chain, index);
}

llvm::FunctionType *VariableIdentifier::llvm_func_type(Codegen &gen) {
    return linked->llvm_func_type(gen);
}

llvm::Value *VariableIdentifier::llvm_pointer(Codegen &gen) {
    return linked->llvm_pointer(gen);
}

llvm::Value *VariableIdentifier::llvm_value(Codegen &gen) {
    if(linked->value_type() == ValueType::Array) {
        return gen.builder->CreateGEP(llvm_type(gen), llvm_pointer(gen), {gen.builder->getInt32(0), gen.builder->getInt32(0)}, "", gen.inbounds);;
    }
    return linked->llvm_load(gen);
}

bool VariableIdentifier::add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) {
    if(parent) {
        return parent->linked_node()->add_child_index(gen, indexes, value);
    }
    return true;
}

llvm::Value *VariableIdentifier::llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) {
    return linked->llvm_ret_load(gen, returnStmt);
}

llvm::Value *VariableIdentifier::access_chain_value(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned until, std::vector<std::pair<Value*, llvm::Value*>>& destructibles) {
    if(linked->as_enum_member() != nullptr) {
        return llvm_value(gen);
    } else {
        return ChainValue::access_chain_value(gen, values, until, destructibles);
    }
}

llvm::Type *DereferenceValue::llvm_type(Codegen &gen) {
    auto addr = value->create_type();
    if(addr->kind() == BaseTypeKind::Pointer) {
        return ((PointerType*) (addr.get()))->type->llvm_type(gen);
    } else {
        gen.error("Derefencing a value that is not a pointer " + value->representation());
        return nullptr;
    }
}

llvm::Value *DereferenceValue::llvm_pointer(Codegen& gen) {
    return value->llvm_value(gen);
}

llvm::Value *DereferenceValue::llvm_value(Codegen &gen) {
    return gen.builder->CreateLoad(llvm_type(gen), value->llvm_value(gen), "deref");
}

llvm::Value *Expression::llvm_logical_expr(Codegen &gen, BaseType* firstType, BaseType* secondType) {
    if((operation == Operation::LogicalAND || operation == Operation::LogicalOR)) {
        auto first = firstValue->llvm_value(gen);
        auto current_block = gen.builder->GetInsertBlock();
        llvm::BasicBlock* second_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
        llvm::BasicBlock* end_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
        if(operation == Operation::LogicalAND) {
            gen.CreateCondBr(first, second_block, end_block);
        } else {
            gen.CreateCondBr(first, end_block, second_block);
        }
        gen.SetInsertPoint(second_block);
        auto second = secondValue->llvm_value(gen);
        gen.CreateBr(end_block);
        gen.SetInsertPoint(end_block);
        auto phi = gen.builder->CreatePHI(gen.builder->getInt1Ty(), 2);
        phi->addIncoming(gen.builder->getInt1(operation == Operation::LogicalOR), current_block);
        phi->addIncoming(second, second_block);
        return phi;
    }
    return nullptr;
}

llvm::Value *Expression::llvm_value(Codegen &gen) {
    auto firstType = firstValue->create_type();
    auto secondType = secondValue->create_type();
    auto first_pure = firstType->get_pure_type();
    auto second_pure = secondType->get_pure_type();
    replace_number_values(first_pure.get(), second_pure.get());
    shrink_literal_values(first_pure.get(), second_pure.get());
    promote_literal_values(first_pure.get(), second_pure.get());
    firstType = firstValue->create_type();
    first_pure = firstType->get_pure_type();
    secondType = secondValue->create_type();
    second_pure = secondType->get_pure_type();
    auto logical = llvm_logical_expr(gen, first_pure.get(), second_pure.get());
    if(logical) return logical;
    return gen.operate(operation, firstValue.get(), secondValue.get(), first_pure.get(), second_pure.get());
}

// a || b
// if a is true, goto then block
// if a is false, goto second block, if b is true, goto then block otherwise goto end/else block
// a && b
// if a is true, goto second block, if b is true, goto then block otherwise goto end/else block
// if a is false, goto end/else block
void Expression::llvm_conditional_branch(Codegen& gen, llvm::BasicBlock* then_block, llvm::BasicBlock* otherwise_block) {
    if((operation == Operation::LogicalAND || operation == Operation::LogicalOR)) {
        auto first = firstValue->llvm_value(gen);
        llvm::BasicBlock* second_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
        if(operation == Operation::LogicalAND) {
            gen.CreateCondBr(first, second_block, otherwise_block);
        } else {
            gen.CreateCondBr(first, then_block, second_block);
        }
        gen.SetInsertPoint(second_block);
        auto second = secondValue->llvm_value(gen);
        gen.CreateCondBr(second, then_block, otherwise_block);
    } else {
        return Value::llvm_conditional_branch(gen, then_block, otherwise_block);
    }
}

llvm::Type *Expression::llvm_type(Codegen &gen) {
    return create_type()->llvm_type(gen);
}

llvm::Type *CastedValue::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

llvm::Value *CastedValue::llvm_value(Codegen &gen) {
    auto llvm_val = value->llvm_value(gen);
    auto value_type = value->create_type();
    if(value_type->kind() == BaseTypeKind::IntN && type->kind() == BaseTypeKind::IntN) {
        auto from_num_type = (IntNType*) value_type.get();
        auto to_num_type = (IntNType*) type.get();
        if(from_num_type->num_bits() < to_num_type->num_bits()) {
            if (from_num_type->is_unsigned()) {
                return gen.builder->CreateZExt(llvm_val, to_num_type->llvm_type(gen));
            } else {
                return gen.builder->CreateSExt(llvm_val, to_num_type->llvm_type(gen));
            }
        } else if(from_num_type->num_bits() > to_num_type->num_bits()) {
            return gen.builder->CreateTrunc(llvm_val, to_num_type->llvm_type(gen));
        }
    } else if((value_type->kind() == BaseTypeKind::Float || value_type->kind() == BaseTypeKind::Double) && type->kind() == BaseTypeKind::IntN) {
        if(((IntNType*) type.get())->is_unsigned()) {
            return gen.builder->CreateFPToUI(llvm_val, type->llvm_type(gen));
        } else {
            return gen.builder->CreateFPToSI(llvm_val, type->llvm_type(gen));
        }
    } else if((value_type->kind() == BaseTypeKind::IntN && (type->kind() == BaseTypeKind::Float || type->kind() == BaseTypeKind::Double))) {
        if(((IntNType*) value_type.get())->is_unsigned()) {
            return gen.builder->CreateUIToFP(llvm_val, type->llvm_type(gen));
        } else {
            return gen.builder->CreateSIToFP(llvm_val, type->llvm_type(gen));
        }
    } else if(value_type->kind() == BaseTypeKind::Double && type->kind() == BaseTypeKind::Float) {
        return gen.builder->CreateFPTrunc(llvm_val, type->llvm_type(gen));
    } else if(value_type->kind() == BaseTypeKind::Float && type->kind() == BaseTypeKind::Double) {
        return gen.builder->CreateFPExt(llvm_val, type->llvm_type(gen));
    }
//    auto found= gen.casters.find(Codegen::caster_index(value->value_type(), type->kind()));
//    if(found != gen.casters.end()) {
//        return std::unique_ptr<Value>(found->second(&gen, value.get(), type.get()))->llvm_value(gen);
//    }
    return llvm_val;
}

bool CastedValue::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return type->linked_node()->add_child_index(gen, indexes, name);
}

llvm::Type *AddrOfValue::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Value *AddrOfValue::llvm_value(Codegen &gen) {
    return value->llvm_pointer(gen);
}

bool AddrOfValue::add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) {
    return value->add_member_index(gen, parent, indexes);
}

bool AddrOfValue::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return value->add_child_index(gen, indexes, name);
}

llvm::Value* RetStructParamValue::llvm_value(Codegen &gen) {
    if(gen.current_func_type->returnType->value_type() != ValueType::Struct) {
        gen.error("expected current function to have a struct return type for compiler::return_struct");
        return nullptr;
    }
    // TODO implicitly returning struct parameter index is hardcoded
    return gen.current_function->getArg(0);
}

llvm::Value* SizeOfValue::llvm_value(Codegen &gen) {
    value = for_type->byte_size(gen.is64Bit);
    return UBigIntValue::llvm_value(gen);
}

void AccessChain::code_gen(Codegen &gen) {
    auto value = llvm_value(gen);
    const auto call = values.back()->as_func_call();
    if(call) {
        auto ret_type = call->get_base_type();
        auto linked_struct = ret_type->linked_struct_def();
        if(linked_struct) {
            linked_struct->llvm_destruct(gen, value);
        }
    }
}

llvm::Type *AccessChain::llvm_type(Codegen &gen) {
    std::unordered_map<uint16_t, int16_t> active;
    set_generic_iterations(active);
    auto type = values[values.size() - 1]->llvm_type(gen);
    restore_active_iterations(active);
    return type;
}

llvm::Value *AccessChain::llvm_value(Codegen &gen) {
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    std::unordered_map<uint16_t, int16_t> active;
    set_generic_iterations(active);
    auto value = values[values.size() - 1]->access_chain_value(gen, values, values.size() - 1, destructibles);
    restore_active_iterations(active);
    Value::destruct(gen, destructibles);
    return value;
}

llvm::Value *AccessChain::llvm_pointer(Codegen &gen) {
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    std::unordered_map<uint16_t, int16_t> active;
    set_generic_iterations(active);
    auto value = values[values.size() - 1]->access_chain_pointer(gen, values, destructibles, values.size() - 1);
    restore_active_iterations(active);
    Value::destruct(gen, destructibles);
    return value;
}

llvm::AllocaInst *AccessChain::llvm_allocate(Codegen &gen, const std::string &identifier) {
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    std::unordered_map<uint16_t, int16_t> active;
    set_generic_iterations(active);
    auto value = values[values.size() - 1]->access_chain_allocate(gen, values, values.size() - 1);
    restore_active_iterations(active);
    Value::destruct(gen, destructibles);
    return value;
}

void AccessChain::llvm_destruct(Codegen &gen, llvm::Value *allocaInst) {
    values[values.size() - 1]->llvm_destruct(gen, allocaInst);
}

llvm::FunctionType *AccessChain::llvm_func_type(Codegen &gen) {
    return values[values.size() - 1]->llvm_func_type(gen);
}

bool AccessChain::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return values[values.size() - 1]->add_child_index(gen, indexes, name);
}

bool access_chain_store_in_parent(
    Codegen &gen,
    AccessChain* chain,
    Value *parent,
    llvm::Value *allocated,
    std::vector<llvm::Value *>& idxList,
    unsigned int index
) {
    auto func_call = chain->values[chain->values.size() - 1]->as_func_call();
    if(func_call) {
        auto func_type = func_call->create_function_type();
        if(func_type->returnType->value_type() == ValueType::Struct) {
            auto elem_pointer = Value::get_element_pointer(gen, parent->llvm_type(gen), allocated, idxList, index);
            std::vector<llvm::Value *> args;
            std::vector<std::pair<Value*, llvm::Value*>> destructibles;
            // TODO handle destructibles here, what to do, do we need to destruct them ?
            func_call->llvm_chain_value(gen, args, chain->values, chain->values.size() - 1, destructibles,elem_pointer);
            return true;
        }
    }
    return false;
}

unsigned int AccessChain::store_in_struct(
        Codegen &gen,
        StructValue *parent,
        llvm::Value *allocated,
        std::vector<llvm::Value *> idxList,
        unsigned int index
) {
    if(access_chain_store_in_parent(gen, this, (Value*) parent, allocated, idxList, index)) {
        return index + 1;
    }
    return Value::store_in_struct(gen, parent, allocated, idxList, index);
}

unsigned int AccessChain::store_in_array(
        Codegen &gen,
        ArrayValue *parent,
        llvm::AllocaInst *allocated,
        std::vector<llvm::Value *> idxList,
        unsigned int index
) {
    if(access_chain_store_in_parent(gen, this, (Value*) parent, allocated, idxList, index)) {
        return index + 1;
    }
    return Value::store_in_array(gen, parent, allocated, idxList, index);
};

// --------------------------------------- Statements

void ContinueStatement::code_gen(Codegen &gen) {
    gen.CreateBr(gen.current_loop_continue);
}

void ReturnStatement::code_gen(Codegen &gen, Scope *scope, unsigned int index) {
    // before destruction, get the return value
    llvm::Value* return_value = nullptr;
    if(value.has_value()) {
        if(value.value()->reference() && value.value()->value_type() == ValueType::Struct) {
            llvm::MaybeAlign noAlign;
            gen.builder->CreateMemCpy(gen.current_function->getArg(0), noAlign, value.value()->llvm_pointer(gen), noAlign, value.value()->byte_size(gen.is64Bit));
        } else {
            return_value = value.value()->llvm_ret_value(gen, this);
            if(func_type) {
                auto value_type = value.value()->get_pure_type();
                auto to_type = func_type->returnType->get_pure_type();
                return_value = gen.implicit_cast(return_value, value_type.get(), to_type.get());
            }
        }
    }
    // destruction
    if(!gen.has_current_block_ended) {
        int i = gen.destruct_nodes.size() - 1;
        while(i >= 0) {
            gen.destruct_nodes[i]->code_gen_destruct(gen, value.has_value() ? value->get() : nullptr);
            i--;
        }
        gen.destroy_current_scope = false;
    }
    // return the return value calculated above
    if (value.has_value()) {
        gen.CreateRet(return_value);
    } else {
        gen.DefaultRet();
    }
}

void TypealiasStatement::code_gen(Codegen &gen) {

}

llvm::Type *TypealiasStatement::llvm_type(Codegen &gen) {
    return actual_type->llvm_type(gen);
}

void BreakStatement::code_gen(Codegen &gen) {
    gen.CreateBr(gen.current_loop_exit);
}

void Scope::code_gen(Codegen &gen, unsigned destruct_begin) {
    for(auto& node : nodes) {
        node->code_gen_declare(gen);
    }
    int i = 0;
    while(i < nodes.size()) {
//        std::cout << "Generating " + std::to_string(i) << std::endl;
        nodes[i]->code_gen(gen, this, i);
//        std::cout << "Success " + std::to_string(i) << " : " << nodes[i]->representation() << std::endl;
        i++;
    }
    if(gen.destroy_current_scope) {
        i = ((int) gen.destruct_nodes.size()) - 1;
        while (i >= (int) destruct_begin) {
            gen.destruct_nodes[i]->code_gen_destruct(gen, nullptr);
            i--;
        }
    } else {
        gen.destroy_current_scope = true;
    }
    auto itr = gen.destruct_nodes.begin() + destruct_begin;
    gen.destruct_nodes.erase(itr, gen.destruct_nodes.end());
}

void Scope::code_gen(Codegen &gen) {
    code_gen(gen, gen.destruct_nodes.size());
}

void ThrowStatement::code_gen(Codegen &gen) {
    throw std::runtime_error("[UNIMPLEMENTED]");
}

void AssignStatement::code_gen(Codegen &gen) {
    if (assOp == Operation::Assignment) {
        gen.builder->CreateStore(value->llvm_assign_value(gen, lhs.get()), lhs->llvm_pointer(gen));
    } else {
        auto operated = gen.operate(assOp, lhs.get(), value.get());
        gen.builder->CreateStore(operated, lhs->llvm_pointer(gen));
    }
}

void DeleteStmt::code_gen(Codegen &gen) {
    auto pure_type = identifier->get_pure_type();
    if(is_array) {
        if(pure_type->kind() != BaseTypeKind::Array) {
            gen.error("std::delete couldn't get array type");
            return;
        }
        auto arr_type = (ArrayType*) pure_type.get();
        auto elem_type = arr_type->elem_type->get_pure_type();
        if(elem_type->kind() != BaseTypeKind::Pointer) {
            gen.error("only array of pointers can be deleted using std::delete");
            return;
        }
        auto pointee = ((PointerType*) elem_type.get())->type.get();
        gen.destruct(identifier->llvm_pointer(gen), arr_type->array_size, pointee, this, [](Codegen* gen, llvm::Value* structPtr, void* data){
            const auto stmt = (DeleteStmt*) data;
            std::vector<llvm::Value*> args;
            args.emplace_back(structPtr);
            gen->builder->CreateCall(stmt->free_func_linked->llvm_func_type(*gen), stmt->free_func_linked->llvm_pointer(*gen), args);
        });
    } else {
        auto def = pure_type->linked_node()->as_struct_def();
        auto destructor = def->destructor_func();
        std::vector<llvm::Value *> destr_args;
        if (destructor->has_self_param()) {
            destr_args.emplace_back(identifier->llvm_value(gen));
        }
        gen.builder->CreateCall(destructor->llvm_func_type(gen), destructor->llvm_pointer(gen), destr_args);
        std::vector<llvm::Value*> args;
        args.emplace_back(identifier->llvm_pointer(gen));
        gen.builder->CreateCall(free_func_linked->llvm_func_type(gen), free_func_linked->llvm_pointer(gen), args);
    }
}

void ImportStatement::code_gen(Codegen &gen) {

}

llvm::Type *EnumDeclaration::llvm_type(Codegen &gen) {
    return gen.builder->getInt32Ty();
}

// ----------- Members

llvm::Value *EnumMember::llvm_load(Codegen &gen) {
    return gen.builder->getInt32(index);
}

llvm::Type *EnumMember::llvm_type(Codegen &gen) {
    return gen.builder->getInt32Ty();
}

// ------------ Structures

void Namespace::code_gen_declare(Codegen &gen) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    for(auto& node : nodes) {
        node->code_gen_declare(gen);
    }
}

void Namespace::code_gen(Codegen &gen) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    for(auto& node : nodes) {
        node->code_gen(gen);
    }
}

void Namespace::code_gen(Codegen &gen, Scope *scope, unsigned int index) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    for(auto& node : nodes) {
        node->code_gen(gen, scope, index);
    }
}

void Namespace::code_gen_destruct(Codegen &gen, Value *returnValue) {
    throw std::runtime_error("code_gen_destruct on namespace called");
}