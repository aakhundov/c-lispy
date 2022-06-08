#include "test.h"

#include <assert.h>

#include "env.h"
#include "eval.h"
#include "parse.h"
#include "value.h"

#define RUN_TEST_FN(fn, p, env)                \
    {                                          \
        printf("[%s]\n", #fn);                 \
        printf("=========================\n"); \
        fn(p, env);                            \
        printf("\n");                          \
    }

static int counter = 0;

static value* get_evaluated(parser* p, environment* env, char* input) {
    result r;
    char output[128];

    if (parser_parse(p, input, &r)) {
        tree t = result_get_tree(&r);
        value* v = value_from_tree(&t);
        value* e = value_evaluate(v, env);

        result_dispose_tree(&r);
        value_dispose(v);

        value_to_str(e, output);
        printf("%-5d \"%s\" --> \"%s\"\n", ++counter, input, output);

        return e;
    } else {
        result_print_error(&r);
        result_dispose_error(&r);

        exit(1);
    }
}

static void test_number_output(parser* p, environment* env, char* input, double expected) {
    value* e = get_evaluated(p, env, input);

    if (e != NULL) {
        assert(e->type == VALUE_NUMBER);
        assert(e->number == expected);
        value_dispose(e);
    }
}

static void test_error_output(parser* p, environment* env, char* input, char* expected) {
    value* e = get_evaluated(p, env, input);

    if (e != NULL) {
        assert(e->type == VALUE_ERROR);
        assert(strstr(e->symbol, expected));
        value_dispose(e);
    }
}

static void test_info_output(parser* p, environment* env, char* input, char* expected) {
    value* e = get_evaluated(p, env, input);

    if (e != NULL) {
        assert(e->type == VALUE_INFO);
        assert(strstr(e->symbol, expected));
        value_dispose(e);
    }
}

static void test_str_output(parser* p, environment* env, char* input, char* expected) {
    value* e = get_evaluated(p, env, input);

    if (e != NULL) {
        char buffer[1024];
        value_to_str(e, buffer);
        assert(strcmp(buffer, expected) == 0);
        value_dispose(e);
    }
}

static void test_numeric(parser* p, environment* env) {
    test_number_output(p, env, "+ 1", 1);
    test_number_output(p, env, "+ -1", -1);
    test_number_output(p, env, "+ 0", 0);
    test_number_output(p, env, "+ 1 2 3", 6);
    test_number_output(p, env, "- 1", -1);
    test_number_output(p, env, "- -1", 1);
    test_number_output(p, env, "- 1 2 3", -4);
    test_number_output(p, env, "+ 1 (- 2 3) 4", 4);
    test_number_output(p, env, " +   1 (-  2    3)    4 ", 4);
    test_number_output(p, env, "* 3.14 -2.71", -8.5094);
    test_number_output(p, env, "* 1 2 3 4 5", 120);
    test_number_output(p, env, "/ 1 2", 0.5);
    test_number_output(p, env, "/ -3 4", -0.75);
    test_number_output(p, env, "% 11 3", 2);
    test_number_output(p, env, "% 11.5 3.2", 2);
    test_number_output(p, env, "^ 2 10", 1024);
    test_number_output(p, env, "^ 2 -10", 1. / 1024);
    test_number_output(p, env, "+ 1 (* 2 3) 4 (/ 10 5) (- 6 (% 8 7)) 9", 27);
    test_number_output(p, env, "+ 0 1 2 3 4 5 6 7 8 9 10", 55);
    test_number_output(p, env, "* 0 1 2 3 4 5 6 7 8 9 10", 0);
    test_number_output(p, env, "* 1 2 3 4 5 6 7 8 9 10", 3628800);
    test_number_output(p, env, "* -1 2 -3 4 -5 6 -7 8 -9 10", -3628800);
    test_number_output(p, env, "min 1 3 -5", -5);
    test_number_output(p, env, "max 10 0 -1", 10);
    test_number_output(p, env, "min (max 1 3 5) (max 2 4 6)", 5);
    test_number_output(p, env, "5", 5);
    test_number_output(p, env, "(5)", 5);
    test_number_output(p, env, "(+ 1 2 3 (- 4 5) 6)", 11);
}

static void test_errors(parser* p, environment* env) {
    test_error_output(p, env, "/ 1 0", "division by zero");
    test_error_output(p, env, "+ 1 (/ 2 0) 3", "division by zero");
    test_error_output(p, env, "fake 1 2 3", "undefined symbol: fake");
    test_error_output(p, env, "1 2 3", "must start with a function");
    test_error_output(p, env, "1 2 3", "(1 2 3)");
    test_error_output(p, env, "(1 2 3)", "must start with a function");
    test_error_output(p, env, "(1 2 3)", "(1 2 3)");
    test_error_output(p, env, "+ 1 2 3 -", "arg #3 (<builtin ->) must be of type number");
    test_error_output(p, env, "+ 1 2 3 {4 5}", "arg #3 ({4 5}) must be of type number");
}

static void test_str(parser* p, environment* env) {
    test_str_output(p, env, "", "()");
    test_str_output(p, env, "  ", "()");
    test_str_output(p, env, "+", "<builtin +>");
    test_str_output(p, env, "min", "<builtin min>");
    test_str_output(p, env, "-5", "-5");
    test_str_output(p, env, "(-3.14)", "-3.14");
    test_str_output(p, env, "{}", "{}");
    test_str_output(p, env, "{1}", "{1}");
    test_str_output(p, env, "{1 2 3}", "{1 2 3}");
    test_str_output(p, env, "{+ 1 2 3}", "{+ 1 2 3}");
    test_str_output(p, env, "{1 2 3 +}", "{1 2 3 +}");
    test_str_output(p, env, "{+ 1 2 3 {- 4 5} 6}", "{+ 1 2 3 {- 4 5} 6}");
    test_str_output(p, env, "{+ 1 2 3 (- 4 5) 6}", "{+ 1 2 3 (- 4 5) 6}");
}

static void test_list(parser* p, environment* env) {
    test_str_output(p, env, "list 1 2 3", "{1 2 3}");
    test_str_output(p, env, "list {1 2 3}", "{{1 2 3}}");
    test_str_output(p, env, "list + - * /", "{<builtin +> <builtin -> <builtin *> <builtin />}");
    test_str_output(p, env, "list 0", "{0}");
    test_str_output(p, env, "list", "<builtin list>");
    test_str_output(p, env, "list list", "{<builtin list>}");
    test_str_output(p, env, "(list 1 2 3)", "{1 2 3}");
    test_str_output(p, env, "{list 1 2 3}", "{list 1 2 3}");
}

static void test_head(parser* p, environment* env) {
    test_str_output(p, env, "head {1 2 3}", "{1}");
    test_str_output(p, env, "head {1}", "{1}");
    test_str_output(p, env, "head {+}", "{+}");
    test_str_output(p, env, "head {+ + + -}", "{+}");
    test_str_output(p, env, "head {head + + + -}", "{head}");

    test_error_output(p, env, "head 1", "arg #0 (1) must be of type q-expr");
    test_error_output(p, env, "head {}", "arg #0 ({}) must be at least 1-long");
    test_error_output(p, env, "head 1 2 3", "expects exactly 1 arg");
}

static void test_tail(parser* p, environment* env) {
    test_str_output(p, env, "tail {1}", "{}");
    test_str_output(p, env, "tail {1 2 3}", "{2 3}");
    test_str_output(p, env, "tail {+}", "{}");
    test_str_output(p, env, "tail {+ 1}", "{1}");
    test_str_output(p, env, "tail {1 + 2 -}", "{+ 2 -}");
    test_str_output(p, env, "tail {tail tail tail}", "{tail tail}");

    test_error_output(p, env, "tail 2", "arg #0 (2) must be of type q-expr");
    test_error_output(p, env, "tail {}", "arg #0 ({}) must be at least 1-long");
    test_error_output(p, env, "tail {1} {2} {3}", "expects exactly 1 arg");
}

static void test_join(parser* p, environment* env) {
    test_str_output(p, env, "join {}", "{}");
    test_str_output(p, env, "join {} {}", "{}");
    test_str_output(p, env, "join {} {} {}", "{}");
    test_str_output(p, env, "join {1} {2}", "{1 2}");
    test_str_output(p, env, "join {1} {2 3} {(4 5) /}", "{1 2 3 (4 5) /}");
    test_str_output(p, env, "join {1} {2 3} {(4 5) /} {}", "{1 2 3 (4 5) /}");

    test_error_output(p, env, "join {1} {2 3} 5 {(4 5) /} {}", "arg #2 (5) must be of type q-expr");
    test_error_output(p, env, "join 1 2 3", "arg #0 (1) must be of type q-expr");
}

static void test_eval(parser* p, environment* env) {
    test_number_output(p, env, "eval {+ 1 2 3}", 6);
    test_str_output(p, env, "eval {}", "()");
    test_str_output(p, env, "eval {+}", "<builtin +>");
    test_str_output(p, env, "eval {list {1 2 3}}", "{{1 2 3}}");
    test_str_output(p, env, "eval {list 1 2 3} ", "{1 2 3}");
    test_str_output(p, env, "eval {eval {list + 2 3}}", "{<builtin +> 2 3}");
    test_str_output(p, env, "eval {head (list 1 2 3 4)}", "{1}");
    test_str_output(p, env, "eval (tail {tail tail {5 6 7}})", "{6 7}");
    test_number_output(p, env, "eval (head {(+ 1 2) (+ 10 20)})", 3);
    test_number_output(p, env, "eval (eval {list + 2 3})", 5);

    test_error_output(p, env, "eval {1} {2}", "expects exactly 1 arg");
    test_error_output(p, env, "eval 3.14", "arg #0 (3.14) must be of type q-expr");
}

static void test_cons(parser* p, environment* env) {
    test_str_output(p, env, "cons 1 {}", "{1}");
    test_str_output(p, env, "cons 1 {2 3}", "{1 2 3}");
    test_str_output(p, env, "cons {1} {2 3}", "{{1} 2 3}");
    test_str_output(p, env, "cons + {1 2 3}", "{<builtin +> 1 2 3}");
    test_number_output(p, env, "eval (cons + {1 2 3})", 6);
    test_str_output(p, env, "cons", "<builtin cons>");
    test_str_output(p, env, "cons {} {}", "{{}}");

    test_error_output(p, env, "cons 1", "expects exactly 2 args");
    test_error_output(p, env, "cons {}", "expects exactly 2 args");
    test_error_output(p, env, "cons 1 2 3", "expects exactly 2 args");
    test_error_output(p, env, "cons 1 2", "arg #1 (2) must be of type q-expr");
    test_error_output(p, env, "cons {} 2", "arg #1 (2) must be of type q-expr");
}

static void test_len(parser* p, environment* env) {
    test_number_output(p, env, "len {}", 0);
    test_number_output(p, env, "len {1}", 1);
    test_number_output(p, env, "len {1 2 3}", 3);
    test_number_output(p, env, "len {{1} {2 3 4 5}}", 2);

    test_error_output(p, env, "len 1", "arg #0 (1) must be of type q-expr");
    test_error_output(p, env, "len +", "arg #0 (<builtin +>) must be of type q-expr");
    test_error_output(p, env, "len {} {}", "expects exactly 1 arg");
}

static void test_init(parser* p, environment* env) {
    test_str_output(p, env, "init {1}", "{}");
    test_str_output(p, env, "init {1 2 3}", "{1 2}");
    test_str_output(p, env, "init {{1} {2 3} {4}}", "{{1} {2 3}}");
    test_str_output(p, env, "init {{1} (+ 2 3) {4}}", "{{1} (+ 2 3)}");
    test_str_output(p, env, "init {+ - * /}", "{+ - *}");

    test_error_output(p, env, "init {}", "arg #0 ({}) must be at least 1-long");
    test_error_output(p, env, "init 1", "arg #0 (1) must be of type q-expr");
    test_error_output(p, env, "init {1} {2}", "expects exactly 1 arg");
}

static void test_def(parser* p, environment* env) {
    test_error_output(p, env, "two", "undefined symbol");
    test_info_output(p, env, "def {two} 2", "defined: two");
    test_str_output(p, env, "two", "2");
    test_error_output(p, env, "pi", "undefined symbol");
    test_error_output(p, env, "times", "undefined symbol");
    test_error_output(p, env, "some", "undefined symbol");
    test_info_output(p, env, "def {pi times some} 3.14 * {xyz}", "defined: pi times some");
    test_str_output(p, env, "pi", "3.14");
    test_str_output(p, env, "times", "<builtin times>");
    test_str_output(p, env, "some", "{xyz}");
    test_number_output(p, env, "times two pi", 6.28);
    test_error_output(p, env, "arglist", "undefined symbol");
    test_info_output(p, env, "def {arglist} {one two three four}", "defined: arglist");
    test_str_output(p, env, "arglist", "{one two three four}");
    test_info_output(p, env, "def arglist 1 2 3 4", "defined: one two three four");
    test_str_output(p, env, "list one two three four", "{1 2 3 4}");
    test_number_output(p, env, "eval (join {+} (list one two three four))", 10);

    test_error_output(p, env, "def {a}", "expects at least 2 args");
    test_error_output(p, env, "def 1 2", "arg #0 (1) must be of type q-expr");
    test_error_output(p, env, "def {} 1", "arg #0 ({}) must be at least 1-long");
    test_error_output(p, env, "def {a b} 1", "expects exactly 3 args");
    test_error_output(p, env, "def {a b c} 1", "expects exactly 4 args");
    test_error_output(p, env, "def {a b c} 1 2", "expects exactly 4 args");
    test_error_output(p, env, "def {1} 2", "arg #0 ({1}) must consist of symbol children");
    test_error_output(p, env, "def {a 1} 2 3", "arg #0 ({a 1}) must consist of symbol children");
}

static void test_lambda(parser* p, environment* env) {
    test_str_output(p, env, "lambda", "<builtin lambda>");
    test_str_output(p, env, "lambda {x} {x}", "<lambda {x} {x}>");
    test_str_output(p, env, "lambda {} {x}", "<lambda {} {x}>");
    test_str_output(p, env, "lambda {x y} {+ x y}", "<lambda {x y} {+ x y}>");

    test_error_output(p, env, "lambda 1", "expects exactly 2 args");
    test_error_output(p, env, "lambda {x}", "expects exactly 2 args");
    test_error_output(p, env, "lambda {x} {x} {x}", "expects exactly 2 args");
    test_error_output(p, env, "lambda 1 2", "arg #0 (1) must be of type q-expr");
    test_error_output(p, env, "lambda {x} 2", "arg #1 (2) must be of type q-expr");
    test_error_output(p, env, "lambda 1 {x}", "arg #0 (1) must be of type q-expr");
    test_error_output(p, env, "lambda {1} {x}", "arg #0 ({1}) must consist of symbol children");
    test_error_output(p, env, "lambda {x &} {1}", "exactly one argument must follow &");
    test_error_output(p, env, "lambda {x & y z} {1}", "exactly one argument must follow &");
}

static void test_parent_env(parser* p, environment* env) {
    environment cenv;
    environment_init(&cenv);
    cenv.parent = env;

    test_error_output(p, env, "global-var", "undefined symbol: global-var");
    test_error_output(p, &cenv, "global-var", "undefined symbol: global-var");
    test_info_output(p, &cenv, "def {global-var} 1", "defined: global-var");
    test_number_output(p, env, "global-var", 1);
    test_number_output(p, &cenv, "global-var", 1);

    test_error_output(p, env, "local-var", "undefined symbol: local-var");
    test_error_output(p, &cenv, "local-var", "undefined symbol: local-var");
    test_info_output(p, &cenv, "local {local-var} 1", "defined: local-var");
    test_error_output(p, env, "local-var", "undefined symbol: local-var");
    test_number_output(p, &cenv, "local-var", 1);

    environment_dispose(&cenv);
}

static void test_function_call(parser* p, environment* env) {
    test_info_output(p, env, "def {fn-negate} (lambda {x} {- x})", "defined: fn-negate");
    test_info_output(p, env, "def {fn-restore} (lambda {x} {- (fn-negate x)})", "defined: fn-restore");
    test_info_output(p, env, "def {fn-add} (lambda {x y z} {+ x y z})", "defined: fn-add");
    test_info_output(p, env, "def {fn-add-mul} (lambda {x y} {+ x (* x y)})", "defined: fn-add-mul");
    test_info_output(p, env, "def {fn-pack} (lambda {x & y} {join (list x) y})", "defined: fn-pack");
    test_info_output(p, env, "def {fn-curry} (lambda {f args} {eval (join (list f) args)})", "defined: fn-curry");
    test_info_output(p, env, "def {fn-uncurry} (lambda {f & args} {f args})", "defined: fn-uncurry");
    test_info_output(p, env, "def {fn-wrong} (lambda {x} {+ x y})", "defined: fn-wrong");

    test_number_output(p, env, "fn-negate 1", -1);
    test_number_output(p, env, "fn-negate -3.14", 3.14);
    test_number_output(p, env, "fn-restore -3.14", -3.14);
    test_number_output(p, env, "fn-add 1 2 3", 6);
    test_number_output(p, env, "fn-add 0 0 0", 0);
    test_number_output(p, env, "fn-add-mul 10 20", 210);
    test_number_output(p, env, "fn-add-mul -7 5", -42);
    test_str_output(p, env, "fn-pack 1", "{1}");
    test_str_output(p, env, "fn-pack 1 2 3", "{1 2 3}");
    test_number_output(p, env, "fn-curry + {1 2 3}", 6);
    test_number_output(p, env, "fn-curry * {10 20}", 200);
    test_str_output(p, env, "fn-uncurry head 1 2 3", "{1}");
    test_number_output(p, env, "fn-uncurry len 1 2 3", 3);
    test_str_output(p, env, "fn-uncurry tail 1", "{}");

    test_error_output(p, env, "fn-negate 1 2", "expects exactly 1 arg");
    test_error_output(p, env, "fn-add 1 2", "expects exactly 3 args");
    test_error_output(p, env, "fn-add 1 2 3 4", "expects exactly 3 args");
    test_error_output(p, env, "fn-wrong 1", "undefined symbol: y");
}

void run_test(parser* p, environment* env) {
    counter = 0;

    RUN_TEST_FN(test_numeric, p, env);
    RUN_TEST_FN(test_errors, p, env);
    RUN_TEST_FN(test_str, p, env);
    RUN_TEST_FN(test_list, p, env);
    RUN_TEST_FN(test_head, p, env);
    RUN_TEST_FN(test_tail, p, env);
    RUN_TEST_FN(test_join, p, env);
    RUN_TEST_FN(test_eval, p, env);
    RUN_TEST_FN(test_cons, p, env);
    RUN_TEST_FN(test_len, p, env);
    RUN_TEST_FN(test_init, p, env);
    RUN_TEST_FN(test_def, p, env);
    RUN_TEST_FN(test_lambda, p, env);
    RUN_TEST_FN(test_parent_env, p, env);
    RUN_TEST_FN(test_function_call, p, env);
}
