#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* ~~~~~~~~~~~ VM ~~~~~~~~~~~ */
typedef enum BytecodeTag {
    BCT_SUBSTITUTE_MATCH,
    BCT_SUBSTITUTE_REPLACE,
    BCT_DRILL,
    BCT_AROUND,
    BCT_ECHO
} BytecodeTag;


typedef struct Bytecode {
    BytecodeTag* tag;
    char** data;
    unsigned int cnt; // count of elements in tag/data
    unsigned int contiguousc; // size of allocated memory block for tag/data
} Bytecode;

#define _print_bytecode_branch(name) case name:\
    printf(#name);\
    break;

void print_bytecode(Bytecode in) {
    for (unsigned int i = 0; i < in.cnt; i++) {
        switch (in.tag[i]) {
            _print_bytecode_branch(BCT_SUBSTITUTE_MATCH)
            _print_bytecode_branch(BCT_SUBSTITUTE_REPLACE)
            _print_bytecode_branch(BCT_DRILL)
            _print_bytecode_branch(BCT_AROUND)
            _print_bytecode_branch(BCT_ECHO)
        }
        if (in.data[i] == NULL) {
            printf(" (no data)\n");
        } else {
            printf(" %s\n", in.data[i]);
        }
    }
}

void bytecode_recursive_free(Bytecode* bc) {
    for (unsigned int i = 0; i < bc->cnt; i++) {
        free(bc->data[i]);
    }
    free(bc->tag);
    free(bc);
}

Bytecode* bytecode_new() { 
    Bytecode* bc = calloc(1, sizeof(Bytecode));
    bc->tag = NULL;
    bc->data = NULL;
    bc->cnt = 0;
    bc->contiguousc = 0;
    return bc;
}

// Reallocs the bytecode if it overflows with `newelems` entries added
void bytecode_realloc(Bytecode* in, unsigned int newelems) {
    while ((in->cnt + newelems) * sizeof(void*) > in->contiguousc) {
        in->contiguousc = (in->contiguousc + 1) * 2;
        in->tag = realloc(in->tag, in->contiguousc);
        in->data = realloc(in->data, in->contiguousc);
    }
}

// void* data should live until the deallocation of the bytecode
void bytecode_append(Bytecode* in, BytecodeTag tag, char* data) {
    bytecode_realloc(in, 1);
    in->tag[in->cnt] = tag;
    in->data[in->cnt] = data;
    in->cnt++;
}

void bytecode_concat(Bytecode* in, Bytecode other) {
    // realloc to get more space if we've run out
    bytecode_realloc(in, other.cnt);
    for (unsigned int i = 0; i < other.cnt; i++) {
        in->tag[in->cnt + i] = other.tag[i];
        in->data[in->cnt + i] = other.data[i];
    }
    in->cnt += other.cnt;
}



/* ~~~~~~~~~~~ Parser ~~~~~~~~~~~ */
#define PAD_SIZE 1024
typedef struct ParseState {
    Bytecode* out; // Did this parser produce output?
    char* altout; // Some parsers produce strings instead
    char* rest; // The rest of the input stream
} ParseState;
void free_ParseState(ParseState ps) {
    free(ps.out);
    free(ps.altout);
}

void die(const char* fmt,...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    exit(1);
}

void consume_subparse(ParseState* p, ParseState subparse) {
    if (subparse.out != NULL) {
        bytecode_concat(p->out, *subparse.out);
    }
    p->rest = subparse.rest;
}

// parses regexp ending on /, places it in altout
ParseState parse_regexp(char* in) {
    ParseState o = { .altout = "", .rest = in };
    char pad[PAD_SIZE] = "";
    unsigned int padc = 0;
    while (padc < PAD_SIZE) {
        char c = *o.rest;
        switch (c) {
            case '\0':
                die("EOF while parsing regexp");
                break;
            case '/':
                goto done;
            default:
                pad[padc++] = *o.rest;
                pad[padc] = '\0';
                o.rest++;
        }
    } done:
    o.altout = strdup(pad);
    return o;
}

// parses /regexp/
ParseState parse_drill(char* in) {
    Bytecode* bc_out = bytecode_new();
    ParseState o = { .out = bc_out, .rest = in };
    if (*(o.rest++) != '/') {
        die("Expected / at start of drill");
    }
    ParseState sub = parse_regexp(o.rest);
    consume_subparse(&o, sub);
    bytecode_append(o.out, BCT_DRILL, strdup(sub.altout));
    free_ParseState(sub);
    if (*o.rest != '/') {
        die("Expected / at end of drill");
    }
    o.rest++;

    return o;
}

// parses s/regexp/replacement/
ParseState parse_substitute(char* in) {
    Bytecode* bc_out = bytecode_new();
    ParseState o = { .out = bc_out, .rest = in };
    if (*(o.rest++) != 's') {
        die("Expected s at start of substitution");
    }
    if (*(o.rest++) != '/') {
        die("Expected s/ at start of substitution");
    }
    ParseState sub1 = parse_regexp(o.rest);
    consume_subparse(&o, sub1);
    bytecode_append(o.out, BCT_SUBSTITUTE_MATCH, strdup(sub1.altout));
    free_ParseState(sub1);
    if (*(o.rest++) != '/') {
        die("Expected / in middle of substitution");
    }
    ParseState sub2 = parse_regexp(o.rest);
    consume_subparse(&o, sub2);
    bytecode_append(o.out, BCT_SUBSTITUTE_REPLACE, strdup(sub2.altout));
    free_ParseState(sub2);
    if (*(o.rest++) != '/') {
        die("Expected / at end of substitution");
    }
    return o;
}


ParseState parse_root(char* in) {
    Bytecode* bc_out = bytecode_new();
    ParseState o = { .out = bc_out, .rest = in };
    bytecode_append(o.out, BCT_ECHO, NULL);
    while (1) {
        char c = *o.rest;
        ParseState sub;
        switch (c) {
            case '\0':
                goto done;
            case 's':
                sub = parse_substitute(o.rest);
                consume_subparse(&o, sub);
                free_ParseState(sub);
                break;
            case '/':
                sub = parse_drill(o.rest);
                consume_subparse(&o, sub);
                free_ParseState(sub);
                break;

            case ' ':
            case '\t':
            case '\n':
                o.rest++;
                break;
            default:
                die("Unexpected %c in root parser", c);
                break;
        }
    } done:
    return o;
}

Bytecode parse(char* in) {
    return *parse_root(in).out;
}

int main(int argc, char** argv) {
    Bytecode prog = parse("s/// s/he?ll{6}owo/rld/ /hello/");
    print_bytecode(prog);
    return 0;
}