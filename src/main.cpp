#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include "jit.hpp"

// available functions to be used within JIT-compiled code
extern "C"
{
	static int my_div(int a, int b) { return a / b; }
	static int my_mod(int a, int b) { return a % b; }
	static int my_inc(int a) { return ++a; }
	static int my_dec(int a) { return --a; }
}

const uint32_t SYMTABLE_SIZE = 100;  // symtable size in units
const uint32_t EXPR_SIZE = 100;      // max expression size in chars
const uint32_t CODE_SIZE = 4096;     // code segment size in bytes

static symbol_t symbols[SYMTABLE_SIZE + 1];
char expression_to_parse[EXPR_SIZE + 1];

static size_t 
init_symbols()
{
    memset(symbols, 0, sizeof(symbols));
    static const char* func_names[] = {"div", "mod", "inc", "dec"};

    size_t offset = 0;

    symbols[offset].name = func_names[offset];
    symbols[offset].pointer = reinterpret_cast<void*>(&my_div);
    ++offset;

    symbols[offset].name = func_names[offset];
    symbols[offset].pointer = reinterpret_cast<void*>(&my_mod);
    ++offset;

    symbols[offset].name = func_names[offset];
    symbols[offset].pointer = reinterpret_cast<void*>(&my_inc);
    ++offset;

    symbols[offset].name = func_names[offset];
    symbols[offset].pointer = reinterpret_cast<void*>(&my_dec);
    ++offset;

    return offset;
}

static symbol_t
parse_variable(const char* token)
{
    const char* delim = strchr(token, '=');

    if (!delim || delim == token) {
        fprintf(stderr, "Wrong token in input: %s\n", token);
        exit(1);
    }
    char left[128];
    char right[128];

    memset(left, 0, sizeof(left));
    memset(right, 0, sizeof(right));
    strncpy(left, token, delim - token);
    strncpy(right, delim+1, sizeof(right));

    symbol_t result;
    result.name = (const char*) calloc(1+strnlen(left,sizeof(left)), sizeof(char));
    result.pointer = calloc(1, sizeof(int));
    sscanf(left, "%s", const_cast<char*>(result.name)); // eliminate whitespaces
    sscanf(right, "%d", reinterpret_cast<int*>(result.pointer)); // parse int value
    return result;
}


static void read_input(size_t sym_start_offset)
{    
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));    

    enum class mode_t
    {
        EXPRESSION, VARS
    };

    mode_t current_mode = mode_t::EXPRESSION;
    size_t current_index = sym_start_offset;

    while (NULL != fgets(buffer, sizeof(buffer), stdin)) 
    {
        if ('#' == buffer[0]) continue;       
        else if ('.'==buffer[0]) {
            // change parsing mode
            if (strstr(buffer, "expression"))
            {
                current_mode = mode_t::EXPRESSION;
            }
            else if (strstr(buffer, "vars"))
            {
                current_mode = mode_t::VARS;
            }
        }
        else if (mode_t::EXPRESSION==current_mode)
        {
            memset(expression_to_parse, 0, sizeof(expression_to_parse));
            size_t i=0, j = 0;
            for (i=0; i<strnlen(buffer, sizeof(buffer)); ++i)
            {
                if (!isspace(buffer[i]))
                {
                    expression_to_parse[j] = buffer[i];
                    j++;
                }
            }
        }
        else if (mode_t::VARS == current_mode)
        {
            char minibuf[256];
            memset(minibuf, 0, sizeof(minibuf));
            size_t offset = 0;
            char * tokenizing_string = buffer;
            while (sscanf(tokenizing_string, "%s", minibuf) > 0)
            {
                offset = strnlen(minibuf, sizeof(minibuf));
                tokenizing_string += offset;
                while (' ' == tokenizing_string[0])
                {
                    tokenizing_string++;
                }
                symbols[current_index++] = parse_variable(minibuf);
                memset(minibuf, 0, sizeof(minibuf));
            }
        }
    }
}

static void free_symbols(size_t offset)
{
    while (NULL != symbols[offset].name)
    {
        free((char *) (symbols[offset].name));
        free(symbols[offset].pointer);
        offset++;
    }
}


static void* init_program_code_buffer()
{
    void* result = mmap(
    	0,
		CODE_SIZE,
		PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_PRIVATE | MAP_ANONYMOUS,
		-1,
		0);

    if (result == MAP_FAILED)
    {
        perror("Can't mmap: ");
        exit(2);
    }
    return result;
}

static void free_program_code_buffer(void * addr)
{
    munmap(addr, CODE_SIZE);
}

static void call_function_and_print_result(void * addr)
{
    typedef int (*jited_function_t)();
    jited_function_t function = reinterpret_cast<jited_function_t>(addr);
    int result = function();
    printf("%d\n", result);
}

int main()
{
    size_t functions_count = init_symbols();
    read_input(functions_count);
    void* code_buffer = init_program_code_buffer();

    jit_compile_expression_to_arm(
		expression_to_parse,
		symbols,
		code_buffer);

    call_function_and_print_result(code_buffer);
    
    free_symbols(functions_count);
    free_program_code_buffer(code_buffer);

    return 0;
}