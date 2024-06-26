// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/AnnotableNode.h"
#include "ordered_map.h"

class BaseDefMember : public AnnotableNode {
public:

    std::string name;

    BaseDefMember(
        std::string name
    );

    virtual bool requires_destructor() = 0;

    virtual BaseDefMember* copy_member() = 0;

    virtual Value* default_value() {
        return nullptr;
    }

};

typedef tsl::ordered_map<std::string, std::unique_ptr<BaseDefMember>> VariablesMembersType;