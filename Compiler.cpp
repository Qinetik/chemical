// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#ifdef COMPILER_BUILD
#include <llvm/TargetParser/Host.h>
#endif
#include "lexer/Lexi.h"
#include "utils/Utils.h"
#include "compiler/InvokeUtils.h"
#include "ast/base/GlobalInterpretScope.h"
#include "compiler/Codegen.h"
#include "compiler/SymbolResolver.h"
#include "utils/CmdUtils.h"
#include "cst/base/CSTConverter.h"
#include <filesystem>
#include "utils/StrUtils.h"
#include "preprocess/ImportGraphMaker.h"
#include "preprocess/RepresentationVisitor.h"
#include "utils/PathUtils.h"
#include <functional>
#include "preprocess/2cASTVisitor.h"
#include "compiler/ASTProcessor.h"
#include "integration/libtcc/LibTccInteg.h"
#include "utils/Version.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "rang.hpp"

#ifdef COMPILER_BUILD

int chemical_clang_main(int argc, char **argv);

int chemical_clang_main2(const std::vector<std::string> &command_args);

int llvm_ar_main2(const std::vector<std::string> &command_args);

std::vector<std::unique_ptr<ASTNode>> TranslateC(const char* exe_path, const char *abs_path, const char *resources_path);

#endif

void print_help() {
    std::cout << "[Chemical] "
                 #ifdef COMPILER_BUILD
                 "Compiler v"
                 #endif
                 #ifdef TCC_BUILD
                 "Compiler based on Tiny CC v"
                 #endif
                 << PROJECT_VERSION_MAJOR << "." << PROJECT_VERSION_MINOR << "." << PROJECT_VERSION_PATCH << "\n"
                 << std::endl;
    std::cout << "Compiling a single file : \nchemical.exe <input_filename> -o <output_filename>\n\n"
                 "<input_filename> extensions supported .c , .ch\n"
                 "<output_filename> extensions supported .exe, .o, .c, .ch\n"
                 "use input extension .c and output .ch, when translating C code to Chemical\n"
                 "use input extension .ch and output .c, when translating Chemical to C code\n\n"
                 "Invoke Clang : \nchemical.exe cc <clang parameters>\n\n"
                 "--mode              -m            debug or release mode : debug, debug_quick, release_small, release_fast\n"
                 "--output            -o            specify a output file, output determined by it's extension\n"
                 "--out-ll  <path>    -[empty]      specify a path to output a .ll file containing llvm ir\n"
                 "--out-asm <path>    -[empty]      specify a path to output a .s file containing assembly\n"
                 "--out-bc  <path>    -[empty]      specify a path to output a .bc file containing llvm bitecode\n"
                 "--out-obj <path>    -[empty]      specify a path to output a .obj file\n"
                 "--out-bin <path>    -[empty]      specify a path to output a binary file\n"
                 "--ignore-extension  -[empty]      ignore the extension --output or -o option\n"
                 "--lto               -[empty]      force link time optimization\n"
                 "--assertions        -[empty]      enable assertions on generated code\n"
                 "--debug-ir          -[empty]      output llvm ir, even with errors, for debugging\n"
                 "--arg-[arg]         -arg-[arg]    can be used to provide arguments to build.lab\n"
//                 "--verify            -o            do not compile, only verify source code\n"
                 "--jit               -jit          do just in time compilation using Tiny CC\n"
                 "--no-cbi            -[empty]      this ignores cbi annotations when translating\n"
                 "--cpp-like          -[empty]      configure output of c translation to be like c++\n"
                 "--res <dir>         -res <dir>    change the location of resources directory\n"
                 "--benchmark         -bm           benchmark lexing / parsing / compilation process\n"
                 "--print-ig          -pr-ig        print import graph of the source file\n"
                 "--print-ast         -pr-ast       print representation of AST\n"
                 "--print-cst         -pr-cst       print CST for debugging\n"
                 "" << std::endl;

}

int main(int argc, char *argv[]) {

#ifdef COMPILER_BUILD
    // invoke clang cc1, this is used by clang, because it invokes (current executable)
    if(argc >= 2 && strcmp(argv[1], "-cc1") == 0) {
        return chemical_clang_main(argc, argv);
    }
#endif

    // parsing the command
    CmdOptions options;
    auto args = options.parse_cmd_options(argc, argv, 1, {"cc", "ar", "configure", "linker"});

    // check if configure is called
    auto config_option = options.option("configure");
    if(config_option.has_value()) {
        std::cout << "Successfully configured Chemical Compiler for your OS" << std::endl;
        return 0;
    }

#ifdef COMPILER_BUILD
    // use llvm archiver
    auto llvm_ar = options.option("ar", "ar");
    if(llvm_ar.has_value()) {
        auto subc = options.collect_subcommand(argc, argv, "ar");
//        subc.insert(subc.begin(), argv[0]);
        subc.insert(subc.begin(), "ar");
//        std::cout << "ar  : ";
//        for(const auto& sub : subc) {
//            std::cout << sub;
//        }
//        std::cout << std::endl;
        return llvm_ar_main2(subc);
    }
#endif

#ifdef COMPILER_BUILD
    // use raw clang
    auto rawclang = options.option("cc", "cc");
    if(rawclang.has_value()) {
        auto subc = options.collect_subcommand(argc, argv, "cc");
        subc.insert(subc.begin(), argv[0]);
//        std::cout << "rclg  : ";
//        for(const auto& sub : subc) {
//            std::cout << sub;
//        }
//        std::cout << std::endl;
        return chemical_clang_main2(subc);
    }
#endif

    auto verbose = options.option("verbose", "vv").has_value();
    if(verbose) {
        std::cout << "[Command] ";
        options.print();
        std::cout << std::endl;
    }

    if(options.option("help", "help").has_value()) {
        print_help();
        return 0;
    }

    if(args.empty()) {
        std::cerr << rang::fg::red << "no input given\n\n" << rang::fg::reset;
        print_usage();
        return 1;
    }

    bool jit = options.option("jit", "jit").has_value();
    auto output = options.option("output", "o");
    auto res = options.option("res", "res");
    auto dash_c = options.option("", "c");

    auto get_resources_path = [&res, &argv]() -> std::string{
        auto resources_path = res.has_value() ? res.value() : resources_path_rel_to_exe(std::string(argv[0]));
        if(resources_path.empty()) {
            std::cerr << "[Compiler] Couldn't locate resources path relative to compiler's executable" << std::endl;
        } else if(!res.has_value()) {
            res.emplace(resources_path);
        }
        return resources_path;
    };

    auto prepare_options = [&](LabBuildCompilerOptions* opts) -> void {
        opts->benchmark = options.option("benchmark", "bm").has_value();
        opts->print_representation = options.option("print-ast", "pr-ast").has_value();
        opts->print_cst = options.option("print-cst", "pr-cst").has_value();
        opts->print_ig = options.option("print-ig", "pr-ig").has_value();
        opts->verbose = verbose;
        opts->resources_path = get_resources_path();
        opts->isCBIEnabled = !options.option("no-cbi").has_value();
        if(options.option("lto").has_value()) {
            opts->def_lto_on = true;
        }
        if(options.option("assertions").has_value()) {
            opts->def_assertions_on = true;
        }
    };

#ifdef COMPILER_BUILD

    // get and print target
    auto target = options.option("target", "t");
    if (!target.has_value()) {
        target.emplace(llvm::sys::getDefaultTargetTriple());
    }
    if(verbose) {
        std::cout << "[Target] " << target.value() << std::endl;
    }

    // determine if is 64bit
    bool is64Bit = Codegen::is_arch_64bit(target.value());

#else
    std::optional<std::string> target = "native";
#if defined(_WIN64) || defined(__x86_64__) || defined(__ppc64__)
    bool is64Bit = true;
#else
    bool is64Bit = false;
#endif
#endif

    OutputMode mode = OutputMode::Debug;

    // configuring output mode from command line
    auto mode_opt = options.option("mode", "m");
    if(mode_opt.has_value()) {
        if(mode_opt.value() == "debug") {
            // ignore
        } else if(mode_opt.value() == "debug_quick") {
            mode = OutputMode::DebugQuick;
            if(verbose) {
                std::cout << "[Compiler] Debug Quick Enabled" << std::endl;
            }
        } else if(mode_opt.value() == "release" || mode_opt.value() == "release_fast") {
            mode = OutputMode::ReleaseFast;
            if(verbose) {
                std::cout << "[Compiler] Release Fast Enabled" << std::endl;
            }
        } else if(mode_opt.value() == "release_small") {
            if(verbose) {
                std::cout << "[Compiler] Release Small Enabled" << std::endl;
            }
            mode = OutputMode::ReleaseSmall;
        }
    }
#ifdef DEBUG
    else {
        mode = OutputMode::DebugQuick;
    }
#endif

    // build a .lab file
    if(args[0].ends_with(".lab")) {

        LabBuildCompilerOptions compiler_opts(argv[0], target.value(), is64Bit);
        LabBuildCompiler compiler(&compiler_opts);

        // translate the build.lab to a c file (for debugging)
        if(output.has_value() && output.value().ends_with(".c")) {
            LabJob job(LabJobType::ToCTranslation, chem::string("[BuildLabTranslation]"), chem::string(output.value()), chem::string(compiler_opts.build_folder), { }, { });
            LabModule module(LabModuleType::Files, chem::string("[BuildLabFile]"), chem::string((const char*) nullptr), chem::string((const char*) nullptr), chem::string((const char*) nullptr), chem::string((const char*) nullptr), { }, { });
            module.paths.emplace_back(args[0]);
            job.dependencies.emplace_back(&module);
            return compiler.do_to_c_job(&job);
        }

        LabBuildContext context(args[0]);
        prepare_options(&compiler_opts);
        compiler_opts.def_mode = mode;

        // giving build args to lab build context
        for(auto& opt : options.options) {
            if(opt.first.starts_with("arg-")) {
                context.build_args[opt.first.substr(4)] = opt.second;
            }
        }
        return compiler.build_lab_file(context, args[0]);
    }

    // compilation
    LabBuildCompilerOptions compiler_opts(argv[0], target.value(), is64Bit);
    LabBuildCompiler compiler(&compiler_opts);
    prepare_options(&compiler_opts);
    compiler_opts.def_mode = mode;

    auto ll_out = options.option("out-ll");
    auto bc_out = options.option("out-bc");
    auto obj_out = options.option("out-obj");
    auto asm_out = options.option("out-asm");
    auto bin_out = options.option("out-bin");

    // should the obj file be deleted after linking
    bool temporary_obj = false;

    if(output.has_value()) {
        if(!options.option("ignore-extension").has_value()) {
            // determining output file based on extension
            if (endsWith(output.value(), ".o")) {
                obj_out.emplace(output.value());
            } else if (endsWith(output.value(), ".s")) {
                asm_out.emplace(output.value());
            } else if (endsWith(output.value(), ".ll")) {
                ll_out.emplace(output.value());
            } else if (endsWith(output.value(), ".bc")) {
                bc_out.emplace(output.value());
            } else if(!bin_out.has_value()) {
                bin_out.emplace(output.value());
            }
        } else if(!bin_out.has_value()) {
            bin_out.emplace(output.value());
        }
    } else if(!bin_out.has_value()){
        bin_out.emplace("compiled");
    }

    // have object file output for the binary we are outputting
    if (!obj_out.has_value() && bin_out.has_value()) {
        obj_out.emplace(bin_out.value() + ".o");
        temporary_obj = !dash_c.has_value();
    }

    LabModule module(LabModuleType::Files);
    std::vector<std::unique_ptr<LabModule>> dependencies;
    for(auto& arg : args) {
        if(arg.ends_with(".c")) {
            auto dep = new LabModule(LabModuleType::CFile);
            dependencies.emplace_back(dep);
            dep->paths.emplace_back(arg);
            module.dependencies.emplace_back(dep);
        } else if(arg.ends_with(".o")) {
            auto dep = new LabModule(LabModuleType::ObjFile);
            dependencies.emplace_back(dep);
            dep->paths.emplace_back(arg);
            module.dependencies.emplace_back(dep);
        } else {
            module.paths.emplace_back(arg);
        }
    }

    // files to emit
    if(ll_out.has_value()) {
        module.llvm_ir_path.append(ll_out.value());
        if (options.option("debug-ir").has_value()) {
            compiler_opts.debug_ir = true;
        }
    }
    if(bc_out.has_value())
        module.bitcode_path.append(bc_out.value());
    if(obj_out.has_value())
        module.object_path.append(obj_out.value());
    if(asm_out.has_value())
        module.asm_path.append(asm_out.value());

    LabJob job(LabJobType::Executable);
    if(dash_c.has_value() || !bin_out.has_value()) {
        job.type = LabJobType::ProcessingOnly;
    } else if(output.has_value()) {
        if(output.value().ends_with(".c")) {
            job.type = LabJobType::ToCTranslation;
        } else if(output.value().ends_with(".ch")) {
            job.type = LabJobType::ToChemicalTranslation;
        }
    }
    job.dependencies.emplace_back(&module);
    if(output.has_value()) {
        job.abs_path.append(output.value());
    }
    auto return_int = compiler.do_job(&job);

    // delete object file which was linked
    if(temporary_obj) {
        try {
            std::filesystem::remove(obj_out.value());
        } catch (const std::filesystem::filesystem_error &ex) {
            std::cerr << rang::fg::red << "error: couldn't delete temporary object file " << obj_out.value() << " because " << ex.what() << rang::fg::reset << std::endl;
            return_int = 1;
        }
    }

    return return_int;

}