// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#pragma once

#include <cstdint>
#include <string>

enum class LexTokenType : uint8_t {

    CharOperator,
    Operation,
    Char,
    Comment,
    MultilineComment,
    String,
    Bool,
    Annotation,
    UserToken,

    // absolute strings
    Keyword,
    Number,
    Type,
    Null,
    StringOperator,
    Variable,
    Identifier,
    RawToken,

    // compound statements
    CompAssignment,
    CompAnnotation,
    CompBreak,
    CompContinue,
    CompThrow,
    CompIf,
    CompImport,
    CompIncDec,
    CompReturn,
    CompSwitch,
    CompTypealias,
    CompVarInit,
    CompBody,
    CompDoWhile,
    CompEnumDecl,
    CompForLoop,
    CompFunctionParam,
    CompFunction,
    CompStructDef,
    CompUnionDef,
    CompInterface,
    CompImpl,
    CompNamespace,
    CompTryCatch,
    CompMacro,
    CompWhile,

    // compound types
    CompArrayType,
    CompFunctionType,
    CompGenericType,
    CompReferencedValueType,
    CompPointerType,

    // compound values
    CompAccessChain,
    CompArrayValue,
    CompCastValue,
    CompAddrOf,
    CompDeference,
    CompExpression,
    CompFunctionCall,
    CompIndexOp,
    CompLambda,
    CompNegative,
    CompNot,
    CompStructValue,

    // other things
    CompGenericList,

    // indexes
    IndexLastToken = CompGenericList,
    IndexAbsStrStart = Keyword,
    IndexAbsStrEnd = RawToken,
    IndexCompStart = CompAssignment,
    IndexCompEnd = CompGenericList,
    IndexCompTypeStart = CompArrayType,
    IndexCompTypeEnd = CompPointerType,
    IndexCompValueStart = CompAccessChain,
    IndexCompValueEnd = CompStructValue

};

std::string toTypeString(LexTokenType token);