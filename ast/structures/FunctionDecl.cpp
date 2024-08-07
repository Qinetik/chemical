// Copyright (c) Qinetik 2024.

#include <memory>

#include "FunctionParam.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/UnionDef.h"
#include "compiler/SymbolResolver.h"
#include "CapturedVariable.h"
#include "ast/types/PointerType.h"
#include "ast/statements/VarInit.h"
#include "ast/values/CastedValue.h"
#include "ast/values/RetStructParamValue.h"
#include "ast/types/VoidType.h"
#include "ast/values/FunctionCall.h"
#include "ast/statements/Return.h"
#include "ast/utils/GenericUtils.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "ast/values/LambdaFunction.h"

llvm::Type *BaseFunctionParam::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

llvm::Type *BaseFunctionParam::llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) {
    return type->llvm_chain_type(gen, values, index);
}

llvm::FunctionType *BaseFunctionParam::llvm_func_type(Codegen &gen) {
    return type->llvm_func_type(gen);
}

void BaseFunctionParam::code_gen_destruct(Codegen &gen, Value *returnValue) {
    if(!(returnValue && returnValue->as_identifier() && returnValue->linked_node() == this)) {
        type->linked_node()->llvm_destruct(gen, gen.current_function->getArg(calculate_c_or_llvm_index()));
    }
}

llvm::Type *BaseFunctionParam::llvm_elem_type(Codegen &gen) {
    auto lType = llvm_type(gen);
    if (lType) {
        if (lType->isArrayTy()) {
            return lType->getArrayElementType();
        } else if (lType->isPointerTy()) {
            auto ptr_type = type->pointer_type();
            if (ptr_type) {
                return ptr_type->type->llvm_type(gen);
            } else {
                gen.error("type is not a pointer type for parameter " + name);
            }
        } else {
            gen.error("type is not an array / pointer for parameter " + name);
        }
    } else {
        gen.error("parameter type is invalid " + name);
    }
    return nullptr;
}

llvm::Value *BaseFunctionParam::llvm_pointer(Codegen &gen) {
    auto index = calculate_c_or_llvm_index();
    if(index > gen.current_function->arg_size()) {
        gen.error("couldn't get argument with name " + name + " since function has " + std::to_string(gen.current_function->arg_size()) + " arguments");
        return nullptr;
    }
    auto arg = gen.current_function->getArg(index);
    if (arg) {
        return arg;
    } else {
        gen.error("couldn't get argument with name " + name);
        return nullptr;
    }
}

llvm::Value *BaseFunctionParam::llvm_load(Codegen &gen) {
    if (gen.current_function != nullptr) {
        return llvm_pointer(gen);
    } else {
        gen.error("cannot provide pointer to a function parameter when not generating code for a function");
    }
    return nullptr;
}

llvm::FunctionType *FunctionDeclaration::create_llvm_func_type(Codegen &gen) {
    auto paramTypes = param_types(gen);
    if(paramTypes.empty()) {
        return llvm::FunctionType::get(llvm_func_return(gen, returnType.get()), isVariadic);
    } else {
        return llvm::FunctionType::get(llvm_func_return(gen, returnType.get()), paramTypes, isVariadic);
    }
}

llvm::FunctionType *FunctionDeclaration::llvm_func_type(Codegen &gen) {
    if(!llvm_data.empty() && active_iteration < llvm_data.size()) {
        return llvm_data[active_iteration].second;
    }
    return create_llvm_func_type(gen);
}

llvm::Function* FunctionDeclaration::llvm_func() {
    return (llvm::Function*) llvm_data[active_iteration].first;
}

void BaseFunctionType::queue_destruct_params(Codegen& gen) {
    for(auto& param : params) {
        const auto k = param->type->kind();
        if(k == BaseTypeKind::Referenced || k == BaseTypeKind::Generic) {
            const auto def = param->type->linked_struct_def();
            if(def && def->destructor_func()) {
                gen.destruct_nodes.emplace_back(param.get());
            }
        }
    }
}

void body_gen(Codegen &gen, llvm::Function* funcCallee, std::optional<LoopScope>& body, FunctionDeclaration* func_type) {
    if(body.has_value()) {
        auto prev_func_type = gen.current_func_type;
        gen.current_func_type = func_type;
        gen.current_function = funcCallee;
        const auto destruct_begin = gen.destruct_nodes.size();
        func_type->queue_destruct_params(gen);
        gen.SetInsertPoint(&gen.current_function->getEntryBlock());
        body->code_gen(gen, destruct_begin);
        gen.end_function_block();
        gen.current_function = nullptr;
        gen.current_func_type = prev_func_type;
    }
}

void body_gen(Codegen &gen, FunctionDeclaration* decl, llvm::Function* funcCallee) {
    body_gen(gen, funcCallee, decl->body, decl);
}

void FunctionDeclaration::code_gen(Codegen &gen) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    if(generic_params.empty()) {
        body_gen(gen, this, llvm_func());
    } else {
        const auto total = total_generic_iterations();
        while(bodies_gen_index < total) {
            set_active_iteration(bodies_gen_index);
            body_gen(gen, this, llvm_func());
            bodies_gen_index++;
        }
        // we set active iteration to -1, so all generics would fail if acessed without setting active iteration
        set_active_iteration(-1);
    }
}

void FunctionDeclaration::code_gen_generic(Codegen &gen) {
    code_gen(gen);
}

void llvm_func_attr(llvm::Function* func, AnnotationKind kind) {
    switch(kind) {
        case AnnotationKind::Inline:
            return;
        case AnnotationKind::AlwaysInline:
            func->addFnAttr(llvm::Attribute::AlwaysInline);
            break;
        case AnnotationKind::NoInline:
            func->addFnAttr(llvm::Attribute::NoInline);
            break;
        case AnnotationKind::InlineHint:
            func->addFnAttr(llvm::Attribute::InlineHint);
            break;
        case AnnotationKind::OptSize:
            func->addFnAttr(llvm::Attribute::OptimizeForSize);
            break;
        case AnnotationKind::MinSize:
            func->addFnAttr(llvm::Attribute::MinSize);
            break;
        default:
            return;
    }
}

void llvm_func_def_attr(llvm::Function* func) {
//    func->addFnAttr(llvm::Attribute::UWTable); // this causes error
    func->addFnAttr(llvm::Attribute::NoUnwind);
}

void set_llvm_data(FunctionDeclaration* decl, llvm::Value* func_callee, llvm::FunctionType* func_type) {
#ifdef DEBUG
    if(decl->active_iteration > decl->llvm_data.size()) {
        throw std::runtime_error("decl's generic active iteration is greater than total llvm_data size");
    }
#endif
    if(decl->active_iteration == decl->llvm_data.size()) {
        decl->llvm_data.emplace_back(func_callee, func_type);
    } else {
        decl->llvm_data[decl->active_iteration].first = func_callee;
        decl->llvm_data[decl->active_iteration].second = func_type;
    }
}

void create_non_generic_fn(Codegen& gen, FunctionDeclaration *decl, const std::string& name) {
    auto func = gen.create_function(name, decl->create_llvm_func_type(gen), decl->specifier);
    llvm_func_def_attr(func);
    decl->traverse([func](Annotation* annotation){
        llvm_func_attr(func, annotation->kind);
    });
    set_llvm_data(decl, func, func->getFunctionType());
}

void create_fn(Codegen& gen, FunctionDeclaration *decl, const std::string& name) {
    if(decl->generic_params.empty()) {
        create_non_generic_fn(gen, decl, name);
    } else {
        const auto total_use = decl->total_generic_iterations();
        auto i = (int16_t) decl->llvm_data.size();
        while(i < total_use) {
            decl->set_active_iteration(i);
            create_non_generic_fn(gen, decl, name);
            i++;
        }
        // we set active iteration to -1, so all generics would fail without setting active_iteration
        decl->set_active_iteration(-1);
    }
}

inline void create_fn(Codegen& gen, FunctionDeclaration *decl) {
    create_fn(gen, decl, decl->name);
}

void declare_fn(Codegen& gen, FunctionDeclaration *decl) {
    auto callee = gen.declare_function(decl->name, decl->create_llvm_func_type(gen));
    set_llvm_data(decl, callee.getCallee(), callee.getFunctionType());
}

void FunctionDeclaration::code_gen_declare(Codegen &gen) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    if (body.has_value()) {
        create_fn(gen, this);
    } else {
        declare_fn(gen, this);
    }
    gen.current_function = nullptr;
}

void FunctionDeclaration::code_gen_interface(Codegen &gen, InterfaceDefinition* def) {
    create_fn(gen, this, def->name + "." + name);
    gen.current_function = nullptr;
    if(body.has_value()) {
        code_gen(gen);
    }
}

void FunctionDeclaration::code_gen_override(Codegen& gen, FunctionDeclaration* decl) {
    body_gen(gen, this, decl->llvm_func());
}

void FunctionDeclaration::code_gen_declare(Codegen &gen, StructDefinition* def) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    if(has_annotation(AnnotationKind::Destructor)) {
        ensure_destructor(def);
    }
    create_fn(gen, this, def->name + "." + name);
}

void FunctionDeclaration::code_gen_override_declare(Codegen &gen, FunctionDeclaration* decl) {
    set_llvm_data(this, decl->llvm_pointer(gen), decl->llvm_func_type(gen));
}

void FunctionDeclaration::code_gen_body(Codegen &gen, StructDefinition* def) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    if(has_annotation(AnnotationKind::Destructor)) {
        code_gen_destructor(gen, def);
        return;
    }
    gen.current_function = nullptr;
    code_gen(gen);
}

//void FunctionDeclaration::code_gen_struct(Codegen &gen, StructDefinition* def) {
//    if(has_annotation(AnnotationKind::CompTime)) {
//        return;
//    }
//    create_fn(gen, this, def->name + "." + name);
//    gen.current_function = nullptr;
//    code_gen(gen);
//}

void FunctionDeclaration::code_gen_union(Codegen &gen, UnionDef* def) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    create_fn(gen, this, def->name + "." + name);
    gen.current_function = nullptr;
    code_gen(gen);
}

//void FunctionDeclaration::code_gen_constructor(Codegen& gen, StructDefinition* def) {
//    code_gen_struct(gen, def);
//}

void FunctionDeclaration::code_gen_destructor(Codegen& gen, StructDefinition* def) {
    auto func = llvm_func();
    if(body.has_value() && !body->nodes.empty()) {
        llvm::BasicBlock* cleanup_block = llvm::BasicBlock::Create(*gen.ctx, "", func);
        gen.redirect_return = cleanup_block;
        gen.current_function = nullptr;
        code_gen(gen);
        gen.CreateBr(cleanup_block); // ensure branch to cleanup block
        gen.SetInsertPoint(cleanup_block);
    } else {
        gen.SetInsertPoint(&gen.current_function->getEntryBlock());
    }
    unsigned index = 0;
    for(auto& var : def->variables) {
        if(var.second->value_type() == ValueType::Struct) {
            auto mem_type = var.second->get_value_type();
            auto mem_def = mem_type->linked_node()->as_struct_def();
            auto destructor = mem_def->destructor_func();
            if(!destructor) {
                index++;
                continue;
            }
            auto arg = func->getArg(0);
            std::vector<llvm::Value *> idxList{gen.builder->getInt32(0)};
            std::vector<llvm::Value*> args;
            auto element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), arg, idxList, index);
            if(destructor->has_self_param()) {
                args.emplace_back(element_ptr);
            }
            gen.builder->CreateCall(destructor->llvm_func_type(gen), destructor->llvm_pointer(gen), args);
        }
        index++;
    }
    gen.CreateRet(nullptr);
    gen.redirect_return = nullptr;
}

std::vector<llvm::Type *> FunctionDeclaration::param_types(Codegen &gen) {
    return llvm_func_param_types(gen, params, returnType.get(), false, isVariadic);
}

llvm::Type *FunctionDeclaration::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Value *FunctionDeclaration::llvm_load(Codegen &gen) {
    return llvm_pointer(gen);
}

llvm::Value *FunctionDeclaration::llvm_pointer(Codegen &gen) {
    return llvm_data[active_iteration].first;
}

llvm::Value *CapturedVariable::llvm_load(Codegen &gen) {
    return gen.builder->CreateLoad(llvm_type(gen), llvm_pointer(gen));
}

llvm::Value *CapturedVariable::llvm_pointer(Codegen &gen) {
    auto captured = gen.current_function->getArg(0);
    return gen.builder->CreateStructGEP(lambda->capture_struct_type(gen), captured, index);
}

llvm::Type *CapturedVariable::llvm_type(Codegen &gen) {
    if(capture_by_ref) {
        return gen.builder->getPtrTy();
    } else {
        return linked->llvm_type(gen);
    }
}

bool BaseFunctionParam::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return type->linked_node()->add_child_index(gen, indexes, name);
}

#endif

BaseFunctionParam::BaseFunctionParam(
        std::string name,
        std::unique_ptr<BaseType> type,
        BaseFunctionType* func_type
) : name(std::move(name)), type(std::move(type)), func_type(func_type) {

};

FunctionParam::FunctionParam(
        std::string name,
        std::unique_ptr<BaseType> type,
        unsigned int index,
        std::optional<std::unique_ptr<Value>> defValue,
        BaseFunctionType* func_type
) : BaseFunctionParam(
        std::move(name),
        std::move(type),
        func_type
    ),
    index(index),
    defValue(std::move(defValue))
{
    name.shrink_to_fit();
}

unsigned FunctionParam::calculate_c_or_llvm_index() {
    return func_type->c_or_llvm_arg_start_index() + index;
}

void FunctionParam::accept(Visitor *visitor) {
    visitor->visit(this);
}

ValueType BaseFunctionParam::value_type() const {
    return type->value_type();
}

BaseTypeKind BaseFunctionParam::type_kind() const {
    return type->kind();
}

FunctionParam *FunctionParam::copy() const {
    std::optional<std::unique_ptr<Value>> copied = std::nullopt;
    if (defValue.has_value()) {
        copied.emplace(defValue.value()->copy());
    }
    return new FunctionParam(name, std::unique_ptr<BaseType>(type->copy()), index, std::move(copied), func_type);
}

std::unique_ptr<BaseType> BaseFunctionParam::create_value_type() {
    return std::unique_ptr<BaseType>(type->copy());
}

hybrid_ptr<BaseType> BaseFunctionParam::get_value_type() {
    return hybrid_ptr<BaseType> { type.get(), false };
}

void BaseFunctionParam::declare_and_link(SymbolResolver &linker) {
    if(!name.empty()) {
        linker.declare(name, this);
    }
    type->link(linker, type);
}

ASTNode *BaseFunctionParam::child(const std::string &name) {
    return type->linked_node()->child(name);
}

GenericTypeParameter::GenericTypeParameter(
        std::string identifier,
        std::unique_ptr<BaseType> def_type,
        ASTNode* parent_node
) : identifier(std::move(identifier)),
def_type(std::move(def_type)), parent_node(parent_node) {

}

void GenericTypeParameter::declare_and_link(SymbolResolver &linker) {
    linker.declare(identifier, this);
    if(def_type) {
        def_type->link(linker, def_type);
    }
}

void GenericTypeParameter::register_usage(BaseType* type) {
    if(type) {
        usage.emplace_back(type);
    } else {
        if(def_type) {
            usage.emplace_back(def_type.get());
        } else {
            std::cerr << "expected a generic type argument for parameter " << identifier << " in node " << parent_node->ns_node_identifier() << std::endl;
        }
    }
}

FunctionDeclaration::FunctionDeclaration(
        std::string name,
        func_params params,
        std::unique_ptr<BaseType> returnType,
        bool isVariadic,
        ASTNode* parent_node,
        std::optional<LoopScope> body
) : BaseFunctionType(std::move(params), std::move(returnType), isVariadic),
    name(std::move(name)),
    body(std::move(body)), parent_node(parent_node) {

    params.shrink_to_fit();
    if(this->name == "main") {
        specifier = AccessSpecifier::Public;
    } else {
        specifier = AccessSpecifier::Private;
    }
}

bool FunctionDeclaration::is_exported() {
    if(has_annotation(AnnotationKind::Api)) {
        return true;
    }
    if(parent_node) {
        auto parent = parent_node->as_annotable_node();
        if (parent && parent->has_annotation(AnnotationKind::Api)) {
            return true;
        }
    }
    return false;
}


int16_t FunctionDeclaration::total_generic_iterations() {
    return ::total_generic_iterations(generic_params);
}

void FunctionDeclaration::ensure_constructor(StructDefinition* def) {
    returnType = std::make_unique<ReferencedType>(def->name, def);
}

void FunctionDeclaration::ensure_destructor(StructDefinition* def) {
    if(!has_self_param() || params.size() > 1 || params.empty()) {
        params.clear();
        params.emplace_back(std::make_unique<FunctionParam>("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>(def->name, def)), 0, std::nullopt, this));
    }
    returnType = std::make_unique<VoidType>();
    if(!body.has_value()) {
        body.emplace(std::vector<std::unique_ptr<ASTNode>> {}, this);
        body.value().nodes.emplace_back(new ReturnStatement(std::nullopt, this, &body.value()));
    }
}

void FunctionDeclaration::set_active_iteration(int16_t iteration) {
#ifdef DEBUG
    if(iteration < -1) {
        throw std::runtime_error("please fix iteration, which is less than -1, generic iteration is always greater than or equal to -1");
    }
#endif
    if(iteration == -1) {
        active_iteration = 0;
    } else {
        active_iteration = iteration;
    }
    for (auto &param: generic_params) {
        param->active_iteration = iteration;
    }
}

int16_t FunctionDeclaration::register_call(SymbolResolver& resolver, FunctionCall* call) {
    return register_generic_usage(resolver, this, generic_params, call->generic_list);
}

std::unique_ptr<BaseType> FunctionDeclaration::create_value_type() {
    std::vector<std::unique_ptr<FunctionParam>> copied;
    for(const auto& param : params) {
        copied.emplace_back(param->copy());
    }
    return std::make_unique<FunctionType>(std::move(copied), std::unique_ptr<BaseType>(returnType->copy()), isVariadic, false);
}

hybrid_ptr<BaseType> FunctionDeclaration::get_value_type() {
    return hybrid_ptr<BaseType> { create_value_type().release() };
}

void FunctionDeclaration::accept(Visitor *visitor) {
    visitor->visit(this);
}

void FunctionDeclaration::declare_top_level(SymbolResolver &linker) {
    linker.declare_function(name, this);
    if(has_annotation(AnnotationKind::Extern)) {
        annotations.push_back(AnnotationKind::Api);
        specifier = AccessSpecifier::Public;
    }
    /**
     * when a user has a call to function which is declared below current function, that function
     * has a parameter of type ref struct, the struct has implicit constructor for the value we are passing
     * we need to know the struct, we require the function's parameters to be linked, however that happens declare_and_link which happens
     * when function's body is linked, then an error happens, so we must link the types of parameters of all functions before linking bodies
     * in a single scope
     *
     * TODO However this requires that all the types used for parameters of functions must be declared above the function, because it will link
     *  in declaration stage, If the need arises that types need to be declared below a function, we should refactor this code,
     *
     * Here we are not declaring parameters, just declaring generic ones, we are linking parameters
     */
    linker.scope_start();
    for(auto& gen_param : generic_params) {
        gen_param->declare_and_link(linker);
    }
    for(auto& param : params) {
        param->type->link(linker, param->type);
    }
    linker.scope_end();
}

void FunctionDeclaration::declare_and_link(SymbolResolver &linker) {
    // if has body declare params
    linker.scope_start();
    auto prev_func_type = linker.current_func_type;
    linker.current_func_type = this;
    for(auto& gen_param : generic_params) {
        gen_param->declare_and_link(linker);
    }
    for (auto &param: params) {
        param->declare_and_link(linker);
    }
    returnType->link(linker, returnType);
    if (body.has_value()) {
        if(has_annotation(AnnotationKind::Constructor) && !has_annotation(AnnotationKind::CompTime)) {
            auto init = new VarInitStatement(true, "this", std::nullopt, std::make_unique<CastedValue>(std::make_unique<RetStructParamValue>(), std::make_unique<PointerType>(std::make_unique<ReferencedType>(parent_node->ns_node_identifier(), parent_node))), &body.value());
            body.value().nodes.insert(body.value().nodes.begin(), std::unique_ptr<VarInitStatement>(init));
        }
        body->declare_and_link(linker);
    }
    linker.scope_end();
    linker.current_func_type = prev_func_type;
}

Value *FunctionDeclaration::call(
    InterpretScope *call_scope,
    FunctionCall* call_obj,
    Value* parent,
    bool evaluate_refs
) {
    InterpretScope fn_scope(nullptr, call_scope->global);
    return call(call_scope, call_obj->values, parent, &fn_scope, evaluate_refs);
}

// called by the return statement
void FunctionDeclaration::set_return(Value *value) {
    interpretReturn = value;
    body->stopInterpretOnce();
}

FunctionDeclaration *FunctionDeclaration::as_function() {
    return this;
}

Value *FunctionDeclaration::call(
    InterpretScope *call_scope,
    std::vector<std::unique_ptr<Value>> &call_args,
    Value* parent,
    InterpretScope *fn_scope,
    bool evaluate_refs
) {
    auto self_param = get_self_param();
    auto params_given = call_args.size() + (self_param ? parent ? 1 : 0 : 0);
    if (params.size() != params_given) {
        fn_scope->error("function " + name + " requires " + std::to_string(params.size()) + ", but given params are " +
                        std::to_string(call_args.size()));
        return nullptr;
    }
    if(self_param) {
        fn_scope->declare(self_param->name, parent);
    }
    auto i = self_param ? 1 : 0;
    while (i < params.size()) {
        Value* param_val;
        if(evaluate_refs) {
            param_val = call_args[i]->param_value(*call_scope);
        } else {
            if(call_args[i]->reference()) {
                param_val = call_args[i]->copy();
            } else {
                param_val = call_args[i]->param_value(*call_scope);
            }
        }
        fn_scope->declare(params[i]->name, param_val);
        i++;
    }
    body.value().interpret(*fn_scope);
    if(self_param) {
        fn_scope->erase_value(self_param->name);
    }
    return interpretReturn;
}

std::unique_ptr<BaseType> CapturedVariable::create_value_type() {
    if(capture_by_ref) {
        return std::make_unique<PointerType>(linked->create_value_type());
    } else {
        return linked->create_value_type();
    }
}

hybrid_ptr<BaseType> CapturedVariable::get_value_type() {
    if(capture_by_ref) {
        return hybrid_ptr<BaseType> { new PointerType(linked->create_value_type()), true };
    } else {
        return linked->get_value_type();
    }
}

void CapturedVariable::declare_and_link(SymbolResolver &linker) {
    linked = linker.find(name);
    linker.declare(name, this);
}

BaseTypeKind CapturedVariable::type_kind() const {
    if(capture_by_ref) {
        return BaseTypeKind::Pointer;
    } else {
        return linked->type_kind();
    }
}

ValueType CapturedVariable::value_type() const {
    if(capture_by_ref) {
        return ValueType::Pointer;
    } else {
        return linked->value_type();
    }
}
