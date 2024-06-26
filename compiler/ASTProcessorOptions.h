// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

/**
 * processor options, anything that processes the AST
 * for compilation can use these options
 */
class ASTProcessorOptions {
public:

    /**
     * benchmark the compilation process
     */
    bool benchmark = false;

    /**
     * print the import graph
     */
    bool print_ig = false;

    /**
     * print the representation of files (ASTNodes)
     */
    bool print_representation = false;

    /**
     * print the tokens lexed
     */
    bool print_cst = false;

    /**
     * verbose
     */
    bool verbose = false;

    /**
     * shrink nodes for each file after processing
     */
    bool shrink_nodes = true;

    /**
     * is cbi enabled
     */
    bool isCBIEnabled = true;

    /**
     * path to resources, give only if import's c files
     */
    std::string resources_path;

    /**
     * path to the current compiler executable
     */
    std::string exe_path;

    /**
     * constructor
     */
    ASTProcessorOptions(
            std::string exe_path
    ) : exe_path(std::move(exe_path)) {

    }

};