// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include <memory>

class PointerType : public BaseType {
public:

    hybrid_ptr<BaseType> type;

    explicit PointerType(std::unique_ptr<BaseType> type) : type(type.release(), true) {

    }

    explicit PointerType(hybrid_ptr<BaseType> type) : type(std::move(type)) {

    }

    int16_t get_generic_iteration() override {
        return type->get_generic_iteration();
    }

    std::unique_ptr<BaseType> create_child_type() const override {
        return std::unique_ptr<BaseType>(type->copy());
    }

    hybrid_ptr<BaseType> get_child_type() override {
        return hybrid_ptr<BaseType> { type.get(), false };
    }

    uint64_t byte_size(bool is64Bit) override {
        return is64Bit ? 8 : 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(ValueType value_type) override {
        return type->satisfies(value_type);
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Pointer;
    }

    ValueType value_type() const override {
        return ValueType::Pointer;
    }

    bool satisfies(Value *value) override;

    bool is_same(BaseType *other) override {
        return other->kind() == kind() && static_cast<PointerType *>(other)->type->is_same(type.get());
    }

    PointerType *pointer_type() override {
        return this;
    }

    virtual BaseType *copy() const {
        return new PointerType(std::unique_ptr<BaseType>(type->copy()));
    }

    void link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) override;

    ASTNode *linked_node() override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) override;

#endif

};