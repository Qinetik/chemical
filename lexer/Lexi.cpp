// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 21/02/2024.
//

#include "Lexi.h"
#include "lexer/Lexer.h"
#include "stream/SourceProvider.h"
#include "utils/Benchmark.h"

void benchLex(Lexer* lexer, const std::string& path, BenchmarkResults& results) {
    results.benchmark_begin();
    lexer->lex();
    results.benchmark_end();
}

void benchLexFile(Lexer* lexer, const std::string &path, BenchmarkResults& results) {
    auto fstream = (std::fstream*) (lexer->provider.stream);
    fstream->close();
    fstream->open(path);
    benchLex(lexer, path, results);
    fstream->close();
}

void benchLexFile(Lexer* lexer, const std::string &path) {
    BenchmarkResults results{};
    benchLexFile(lexer, path, results);
}

void lexFile(Lexer* lexer, const std::string &path) {
    auto fstream = (std::fstream*) (lexer->provider.stream);
    fstream->close();
    fstream->open(path);
    lexer->lex();
    fstream->close();
}

Lexer benchLexFile(std::istream &file, const std::string& path) {
    SourceProvider reader(&file);
    Lexer lexer(reader, path);
    BenchmarkResults results{};
    benchLex(&lexer, path, results);
    std::cout << "[Lex]" << " Completed " << results.representation() << std::endl;
    return lexer;
}

Lexer benchLexFile(const std::string &path) {
    std::ifstream file;
    file.open(path);
    if (!file.is_open()) {
        std::cerr << "Unknown error opening the file:" << path << '\n';
    }
    auto lexer = benchLexFile(file, path);
    file.close();
    return lexer;
}

Lexer lexFile(std::istream &file, const std::string& path) {
    SourceProvider reader(&file);
    Lexer lexer(reader, path);
    lexer.lex();
    return lexer;
}

Lexer lexFile(const std::string &path) {
    std::ifstream file;
    file.open(path);
    if (!file.is_open()) {
        std::cerr << "Unknown error opening the file" << '\n';
    }
    auto lexed = lexFile(file, path);
    file.close();
    return lexed;
}