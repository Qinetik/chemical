// Copyright (c) Qinetik 2024.

#include "ASTDiagnoser.h"
#include "SelfInvocation.h"
#include <iostream>
#include "ast/base/ASTNode.h"
#include <filesystem>

#define ANSI_COLOR_RESET   "\x1b[0m"

ASTDiagnoser::ASTDiagnoser(const std::string& path) : current_path(path) {

}

void ASTDiagnoser::info(const std::string &err, ASTNode *node) {
    std::string errStr = "[" + TAG() + "]\n";
    errStr += "---- message : " + err + "\n";
    errStr += "---- file path : " + current_path;
    if(node) {
        errStr += "---- node representation : " + node->representation();
    }
#ifdef DEBUG
//    std::cerr << errStr << std::endl;
#endif
    errors.emplace_back(errStr, DiagSeverity::Information);
}

void ASTDiagnoser::error(const std::string &err, ASTNode *node) {
    has_errors = true;
    auto tag = TAG();
    std::string errStr = "[" + tag + "]\n";
    errStr += "---- message : " + err + "\n";
    errStr += "---- file path : " + current_path;
    if(node) {
        errStr += "\n---- node representation : " + node->representation();
    }
#ifdef DEBUG
//    std::cerr << errStr << std::endl;
#endif
    errors.emplace_back(errStr, DiagSeverity::Error);
}

void ASTDiagnoser::print_errors() {
    std::cout << "[" << TAG() << "] " << std::to_string(errors.size()) << " diagnostics gathered" << std::endl;
    for (const auto &err: errors) {
        std::cout << color(err.severity) << err.message << ANSI_COLOR_RESET << std::endl;
    }
}