import "basic/lambda.ch"
import "basic/expression.ch"
import "basic/floating.ch"
import "basic/nodes.ch"
import "nodes/varinit.ch"
import "nodes/struct.ch"
import "nodes/enum.ch"
import "type/datatype/numbers.ch"
import "type/datatype/strings.ch"
import "basic/macros.ch"
import "basic/arrays.ch"
import "basic/pointers.ch"
import "basic/casts.ch"
import "basic/functions.ch"
import "basic/destructors.ch"
import "nodes/union.ch"
import "nodes/namespaces.ch"
import "comptime/basic.ch"
import "comptime/vector.ch"
import "basic/external.ch"
import "generic/basic.ch"
import "type/datatype/vectors.ch"
import "type/datatype/array_refs.ch"

func main() {
    test_var_init();
    test_lambda();
    test_bodmas();
    test_floating_expr();
    test_nodes();
    test_numbers();
    test_structs();
    test_enum();
    test_strings();
    test_macros();
    test_arrays();
    test_pointer_math();
    test_casts();
    test_functions();
    test_destructors();
    test_unions();
    test_namespaces();
    test_comptime();
    test_compiler_vector();
    test_external_functions();
    test_basic_generics();
    test_vectors();
    test_array_refs();
    print_test_stats();
}