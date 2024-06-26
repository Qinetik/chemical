// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include "compiler/ASTProcessorOptions.h"
#include "utils/fwd/functional.h"

/**
 * this allows you to control the compilation process
 */
class ToCTranslatorOptions : public ASTProcessorOptions {
public:

    /**
     * is the target triple 64bit
     */
    bool is64Bit;

    /**
     * constructor
     */
    ToCTranslatorOptions(std::string exe_path, bool is64Bit);

};

class ToCAstVisitor;
class ASTProcessor;

std::string translate(const std::string& path, ToCTranslatorOptions* options, const std::function<void(ToCAstVisitor*, ASTProcessor*)>& prepare);

bool translate(const std::string& path, const std::string& output_path, ToCTranslatorOptions* options, const std::function<void(ToCAstVisitor*, ASTProcessor*)>& prepare);