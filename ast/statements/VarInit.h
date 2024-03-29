// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "lexer/model/tokens/NumberToken.h"
#include <optional>
#include "ast/base/BaseType.h"
#include "ast/base/GlobalInterpretScope.h"

class VarInitStatement : public ASTNode {
public:

    /**
     * @brief Construct a new InitStatement object.
     *
     * @param identifier The identifier being initialized.
     * @param value The value being assigned to the identifier.
     */
    VarInitStatement(
            bool is_const,
            std::string identifier,
            std::optional<std::unique_ptr<BaseType>> type,
            std::optional<std::unique_ptr<Value>> value
    ) : is_const(is_const), identifier(std::move(identifier)), type(std::move(type)), value(std::move(value)) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

#ifdef COMPILER_BUILD
    inline void declare(Codegen &gen) {
        gen.current[identifier] = this;
    }

    inline void check_has_type(Codegen& gen) {
        if (!type.has_value() && !value.has_value()) {
            gen.error("neither variable type no variable value were given");
            return;
        }
    }

    void undeclare(Codegen &gen) override {
        gen.current.erase(identifier);
        gen.allocated.erase(identifier);
    }

    llvm::Value * llvm_pointer(Codegen &gen) override {
        return value.has_value() ? value.value()->llvm_pointer(gen) : nullptr;
    }

    llvm::Type * llvm_elem_type(Codegen &gen) override {
        return value.has_value() ? value.value()->llvm_elem_type(gen) : nullptr;
    }

    llvm::Type *llvm_type(Codegen &gen) override {
        check_has_type(gen);
        return value.has_value() ? value.value()->llvm_type(gen) : type.value()->llvm_type(gen);
    }

    void code_gen(Codegen &gen) override;
#endif

    VarInitStatement *as_var_init() override {
        return this;
    }

    void interpret(InterpretScope &scope) override {
        if (value.has_value()) {
            auto initializer = value.value()->initializer_value(scope);
            scope.declare(identifier, initializer);
            is_reference = !initializer->primitive();
        }
        decl_scope = &scope;
        scope.declare(identifier, this);
    }

    /**
     * called by assignment to assign a new value in the scope that this variable was declared
     */
    void declare(Value* new_value) {
        decl_scope->declare(identifier, new_value);
    }

    /**
     * called when the value associated with this var init has been moved
     */
    inline void moved() {
        has_moved = true;
    }

    void interpret_scope_ends(InterpretScope &scope) override {
        auto found = scope.find_value_iterator(identifier);
        if (found.first != found.second.end()) {
            if(!is_reference) {
                delete found.first->second;
            }
            found.second.erase(found.first);
        } else if(!has_moved) {
            scope.error("cannot clear non existent variable on the value map " + identifier);
        }
        scope.erase_node(identifier);
    }

    std::string representation() const override {
        std::string rep;
        if(is_const) {
            rep.append("const ");
        } else {
            rep.append("var ");
        }
        rep.append(identifier);
        if (type.has_value()) {
            rep.append(" : ");
            rep.append(type.value()->representation());
        }
        if (value.has_value()) {
            rep.append(" = ");
            rep.append(value.value()->representation());
        }
        return rep;
    }

    bool is_const;
    bool is_reference = false;
    bool has_moved = false;
    InterpretScope* decl_scope = nullptr;
    std::string identifier; ///< The identifier being initialized.
    std::optional<std::unique_ptr<BaseType>> type;
    std::optional<std::unique_ptr<Value>> value; ///< The value being assigned to the identifier.

};