// Copyright (c) Qinetik 2024.

#pragma once

#ifdef COMPILER_BUILD

#include <memory>
#include <utility>
#include <vector>
#include <iostream>
#include "ast/utils/Operation.h"
#include "llvmfwd.h"
#include "ast/base/AccessSpecifier.h"
#include "ast/base/BaseTypeKind.h"
#include "ast/base/ValueType.h"
#include "ASTDiagnoser.h"
#include "OutputMode.h"
#include <unordered_map>
#include "CodegenEmitterOptions.h"
#include "ast/base/GlobalInterpretScope.h"

class Codegen;

class Scope;

class Value;

class BaseType;

class BaseFunctionType;

class MembersContainer;

class FunctionCall;

class FunctionDeclaration;

/**
 * A caster fn takes a pointer to a value, and a pointer to a type
 * the function then returns a new value by casting the given value to the given type
 * the value is created a new on the heap
 */
//typedef Value*(*CasterFn)(Codegen* gen, Value* val, BaseType* type);

class Codegen : public ASTDiagnoser {
public:

    /**
     * global interpret scope, is the scope used for compile time evaluation
     * of code
     */
    GlobalInterpretScope comptime_scope;

    /**
     * nodes that are being traversed to generate the machine code
     */
    std::vector<std::unique_ptr<ASTNode>> nodes;

    /**
     * This is a map between interface names and their functions map
     * the value is a map between function names and their corresponding llvm functions
     * these functions will be removed when code gen has completed.
     */
    std::unordered_map<std::string, std::unordered_map<std::string, llvm::Function *>> unimplemented_interfaces;

    /**
     * casters that take a value and cast them to a different value
     * it should be known that value created by caster is on the heap
     * the caller has the ownership and must manage memory
     */
//    std::unordered_map<int, CasterFn> casters;

    /**
     * contains references to nodes that must be destructed at return
     */
    std::vector<ASTNode*> destruct_nodes;

    /**
     * when a function is evaluated, it's value is stored on this map, so it can be looked up for destruction
     */
    std::unordered_map<FunctionCall*, std::unique_ptr<Value>> evaluated_func_calls;

    /**
     * All get element pointer instructions use this to state that the element pointer is inbounds
     * If true, results in undefined behavior when accessing element out of bounds, which is the default
     */
    bool inbounds = true;

    /**
     * whether the system we are compiling for is 64 bit
     * when compile method is called, this is determined
     */
    bool is64Bit = false;

    /**
     * constructor
     * @param nodes
     * @param path
     */
    explicit Codegen(
            std::vector<std::unique_ptr<ASTNode>> nodes,
            std::string target_triple,
            std::string curr_exe_path,
            bool is_64_bit, // can be determined using static method is_arch_64bit on Codegen
            const std::string& module_name = "ChemMod"
    );

    /**
     * initializes the llvm module and context
     */
    void module_init(const std::string& module_name);

    /**
     * determine whether the system we are compiling for is 64bit or 32bit
     */
    static bool is_arch_64bit(const std::string &target_triple);

    /**
     * provides a caster_index, which can be used to store or retrieve a caster
     * from casters map
     */
//    static constexpr int caster_index(ValueType type, BaseTypeKind kind) {
//        return ((uint8_t) type << 10) | (uint8_t) kind;
//    }

    /**
     * before compilation begins (calling compile_nodes for the first time), this should be called
     * this must be called once for a single module
     */
    void compile_begin();

    /**
     * everytime nodes are switched this method must be called, typically
     * when compiling different modules, nodes are changed
     */
    template<typename NodesVec>
    void compile_nodes(NodesVec& nodes);

    /**
     * everytime nodes are switched this method must be called, typically
     * when compiling different modules, nodes are changed
     */
    void compile_nodes();

    /**
     * after compilation has finished, of all files, this method is called
     * this must be called once, for a single module
     */
    void compile_end();

    /**
     * the actual compile function, when called module, ctx and builder members
     * are used to fill up the IR
     *
     * this just calls compile_begin, compile_nodes and compile_end
     * this should be used if compiling a single file, whose nodes are present in nodes vector
     */
    void compile();

    /**
     * when a function ends, this method is called to basically end the block
     * this is because the functions that return void may need a return statement
     * to end current BasicBlock
     */
    void end_function_block();

    /**
     * the method to create a function
     * @param name name of the function
     * @param type type of the function
     */
    llvm::Function *create_function(const std::string &name, llvm::FunctionType *type, AccessSpecifier specifier);

    /**
     * create a nested function
     */
    llvm::Function *create_nested_function(const std::string &name, llvm::FunctionType *type, BaseFunctionType* func_type, Scope &scope);

    /**
     * gets or inserts a function, similar to declaration
     */
    llvm::FunctionCallee declare_function(const std::string &name, llvm::FunctionType *type);

    /**
     * create a function prototype
     * @param name name of the function
     * @param type type of the function
     */
    llvm::Function *create_function_proto(const std::string &name, llvm::FunctionType *type, AccessSpecifier specifier);

    /**
     * create a function's basic block, with the given name
     */
    llvm::BasicBlock *createBB(const std::string &name, llvm::Function *fn);

    /**
     * packed lambda struct type
     * @return struct type of a packed lambda
     */
    llvm::StructType* packed_lambda_type();

    /**
     * packs a lambda into a struct with two pointers
     */
    llvm::AllocaInst* pack_lambda(llvm::Function* func_ptr, llvm::Value* captured_struct);

    /**
     * creates a function block, along with setting the insert point to this entry block
     */
    void createFunctionBlock(llvm::Function *fn);

    /**
     * prints the current module to console
     */
    void print_to_console();

    /**
     * sets up the module for the given target
     */
    llvm::TargetMachine *setup_for_target(const std::string &TargetTriple);

    /**
     * automatically sets up for the current target triple
     */
    inline llvm::TargetMachine *setup_for_target() {
        return setup_for_target(target_triple);
    }

    /**
     * options will be used to save to files
     */
    bool save_with_options(CodegenEmitterOptions* options);

    /**
     * prints the current module as LLVM IR to a .ll file with given out_path
     */
    bool save_to_ll_file_for_debugging(const std::string &out_path) const;

    /**
     * save as a bitcode file
     */
    bool save_to_bc_file(std::string &out_path, OutputMode mode);

    /**
     * saves as assembly file to this path
     */
    bool save_to_assembly_file(std::string &out_path, OutputMode mode);

    /**
     * saves as object file to this path
     */
    bool save_to_object_file(std::string &out_path, OutputMode mode);

#ifdef LLD_LIBS

    /**
      * You can invoke lld with this function
      */
    int invoke_lld(const std::vector<std::string> &command_args);

#endif

#ifdef CLANG_LIBS

    /**
     * You can invoke clang with this function
     */
    int invoke_clang(const std::vector<std::string> &command_args);

#endif

    /**
     * the given call is linked with given comptime function decl, that is evaluated to receive the return value
     * using this function, the evaluation is done once, so this function caches the return value
     */
    std::unique_ptr<Value>& eval_comptime(FunctionCall* call, FunctionDeclaration* decl);

    /**
     * destructs an array, also allows to add extra logic after destruction via lambda
     * the llvm::Value* provided in lambda is of the struct present in the array
     */
    void destruct(llvm::Value* allocaInst, unsigned int array_size, BaseType* elem_type, void* data, void(*after_destruct)(Codegen*, llvm::Value*, void* data));

    /**
     * destructs an array
     */
    void destruct(llvm::Value* allocaInst, unsigned int array_size, BaseType* elem_type) {
        destruct(allocaInst, array_size, elem_type, nullptr, nullptr);
    }

    /**
     * when generating code for the body of the loop, it should be wrapped with this function call
     * before and after the body generation
     * this ensures that break and continue instructions work properly by pointing to the given blocks
     */
    void loop_body_wrap(llvm::BasicBlock *condBlock, llvm::BasicBlock *endBlock);

    /**
     * This sets the insert point to this block
     * Instead of using builder.SetInsertPoint, this function should be
     * used because llvm doesn't support multiple consecutive returns or branches
     * so if we know the block has changed, we can track
     * if this block has ended previously to avoid creating branches and returns
     */
    void SetInsertPoint(llvm::BasicBlock *block);

    /**
     * The safe version of builder.CreateBr
     * this will avoid creating multiple terminator instructions
     * once you call this, no longer can you create branch, or return instructions
     * because you've already shifted to another block
     */
    void CreateBr(llvm::BasicBlock *block);

    /**
     * The safe version of builder.CreateRet
     * this will avoid creating multiple terminator instructions
     * once you call this, no longer can you create branch, or return instructions
     * because you've already shifted to another block
     */
    void CreateRet(llvm::Value *value);

    /**
     * creates a default return, it returns void
     * this takes into account the redirect_return required for cleanup blocks in
     * destructors, it should be used instead of CreateRet(nullptr) or CreateRetVoid()
     */
    void DefaultRet();

    /**
     * The safe version of builder.CreateCondBr
     * this will avoid creating multiple terminator instructions
     */
    void CreateCondBr(llvm::Value *Cond, llvm::BasicBlock *True, llvm::BasicBlock *FalseMDNode);

    /**
     * tag
     */
    std::string TAG() override {
        return "Codegen";
    }

    /**
    * this operates on two values, left and right
    * this is used by expressions to operate on two values
    */
    llvm::Value *operate(Operation op, Value *lhs, Value *rhs, BaseType* lhsType, BaseType* rhsType);

    /**
     * this operates on two values, left and right
     * this is used by expressions to operate on two values
     */
    llvm::Value *operate(Operation op, Value *lhs, Value *rhs);

    /**
     * implicitly cast given value from type to to type, if no cast is available value is returned
     */
    llvm::Value *implicit_cast(llvm::Value* value, BaseType* from_type, BaseType* to_type);

    /**
     * destructor takes care of deallocating members
     */
    ~Codegen();

    /**
     * TargetTriple , which we are generating code for !
     */
    std::string target_triple;

    /**
     * The function being compiled currently
     */
    llvm::Function *current_function = nullptr;

    /**
     * This is set by every loop so break statement can exit to this block
     */
    llvm::BasicBlock *current_loop_exit = nullptr;

    /**
     * This is set by every loop so continue statement can continue to this block
     */
    llvm::BasicBlock *current_loop_continue = nullptr;

    /**
     * the statements being generated are from this function type
     */
    BaseFunctionType* current_func_type = nullptr;

    /**
     * When given, return's are shifted to this block
     * So when a return statement is detected a branch instruction is made to this block
     * The return must be void for this to work
     */
    llvm::BasicBlock *redirect_return = nullptr;

    /**
     * LLVM context that holds modules
     */
    std::unique_ptr<llvm::LLVMContext> ctx;

    /**
     * module holds functions, global vars and stuff
     */
    std::unique_ptr<llvm::Module> module;

    /**
     * the builder that builds ir
     */
    llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter> *builder;

    /**
     * this is set to true when the branch instruction is executed
     * and set back to false, when a new block begins using SetInsertPoint
     */
    bool has_current_block_ended = false;

    /**
     * by default every scope is destroyed, however a return statement in current scope
     * sets this to true, because return destroys everything in a function
     */
    bool destroy_current_scope = true;

};

template<typename NodesVec>
void Codegen::compile_nodes(NodesVec& nodes_vec) {
    for(const auto& node : nodes_vec) {
        node->code_gen_declare(*this);
    }
    for (const auto &node: nodes_vec) {
        node->code_gen(*this);
    }
}

#endif