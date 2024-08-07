// Copyright (c) Qinetik 2024.

#include "ImplDefinition.h"
#include "StructMember.h"
#include "compiler/SymbolResolver.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"

void ImplDefinition::code_gen(Codegen &gen) {
    auto unimplemented = gen.unimplemented_interfaces.find(interface_name);
    if(unimplemented == gen.unimplemented_interfaces.end()) {
        gen.error("Couldn't find interface with name '" + interface_name + "' for implementation");
        return;
    }
//    auto interface = (InterfaceDefinition*) linked;
    for (auto &function: functions()) {
        auto unimp_func = unimplemented->second.find(function->name);
        if(unimp_func == unimplemented->second.end()) {
            gen.error("Couldn't find function in interface " + interface_name);
            continue;
        }
        if(!unimp_func->second) {
            gen.error("Function '" + function->name + "' in interface '" + interface_name + "' has already been implemented");
            continue;
        }
        auto overridden = linked->child(function->name);
        if (overridden) {
            auto fn = overridden->as_function();
            if (fn) {
                function->code_gen_override(gen, fn);
                unimp_func->second = nullptr;
            }
        }
//        if(!interface->has_implemented(function.second->name)) {
//            auto overridden = linked->child(function.second->name);
//            if (overridden) {
//                auto fn = overridden->as_function();
//                if (fn) {
//                    fn->code_gen_override(gen, function.second.get());
//                    interface->set_implemented(function.second->name, true);
//                }
//            }
//        } else {
//            gen.error("Function '" + function.second->name + "' in interface '" + interface_name + "' has already been implemented");
//        }
    }
}

#endif

void ImplDefinition::declare_and_link(SymbolResolver &linker) {
    MembersContainer::declare_and_link(linker);
    if(struct_name.has_value()) {
        struct_linked = (StructDefinition*) (linker.find(struct_name.value()));
        if(!struct_linked) {
            linker.error("couldn't find struct by name " + struct_name.value() + " for implementation of interface " + interface_name);
        }
    }
    linked = (InterfaceDefinition *) (linker.find(interface_name));
    if(!linked) {
        linker.error("couldn't find interface by name " + interface_name + " for implementation");
    }
}