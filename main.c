// DRILL.
// WRITTEN BY OLUS2000.
// INSPIRED BY SAM. http://sam.cat-v.org/ 
// COMES WITH NO WARRANTY.
// WILL EAT YOUR CAT.
// HACK THE PLANET.
// YOU WANT TO USE IT? EDIT MAIN.
// COMES WITH LOVENSE INTEGRATION.
// BORN TO DIE.
// WORLD IS A FUCK.
// 鬼䘥 KILL EM ALL 1989.
// I AM TRASH MAN.
// 410,757,864,530 DEAD COPS.
// THE ONLY WINNING MOVE IS NOT TO PLAY.
// REST IN PEACE AUGUST 20, 2018.

// BUILD WITH `make`. RUN WITH `./drill`.
// IT'S NOTHING BUT PURE COCAINE AND C99.
// I LOVE YOU.

// HERE'S A FUCKING PHILOSOPHY FOR YOU: 
// https://12ft.io/proxy?q=https%3A%2F%2Fwww.theatlantic.com%2Fscience%2Farchive%2F2017%2F08%2Fannie-dillards-total-eclipse%2F536148%2F
// READ 'EM AND WEEP.

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define DEBUG
// Remove the below line for debug mode.
#undef DEBUG

/* ~~~~~~~~~~~ VM ~~~~~~~~~~~ */
typedef struct Span {
    char* start;
    char* end;
} Span;

typedef struct Foci {
    unsigned long cnt;         // how many foci exist
    unsigned long contiguousc; // size of the foci arena.
    Span* s;                   // the foci
} Foci;

void foci_init(Foci* uninitialized) {
    *uninitialized = (Foci){ .cnt = 0, .contiguousc = 0, .s = NULL };
}

void foci_free(Foci in) {
    free(in.s);
}

typedef enum BytecodeTag {
    BCT_SUBSTITUTE, // Replace the foci with a given string
    BCT_DRILL,      // Narrow the foci to a given regex (well, string)
    BCT_AROUND,     // Narrow the foci to their 0-length beginnings and ends
    BCT_STARTS,     // Narrow the foci to their 0-length beginnings
    BCT_ENDS,       // Narrow the foci to their 0-length ends
    BCT_COMPLEMENT  // Invert focus
} BytecodeTag;

typedef struct Bytecode {
    BytecodeTag* tag;
    char** data;
    unsigned int cnt;         // count of elements in tags and data
    unsigned int contiguousc; // size of allocated arenas for tags and data
} Bytecode;

#define _print_bytecode_branch(idx, name) case name:\
    printf("%ld " #name, ip);\
    break;

void print_single_bytecode(Bytecode in, unsigned long ip) {
    switch (in.tag[ip]) {
        _print_bytecode_branch(ip, BCT_SUBSTITUTE)
        _print_bytecode_branch(ip, BCT_DRILL)
        _print_bytecode_branch(ip, BCT_AROUND)
        _print_bytecode_branch(ip, BCT_STARTS)
        _print_bytecode_branch(ip, BCT_ENDS)
        _print_bytecode_branch(ip, BCT_COMPLEMENT)
    }
    if (in.data[ip] == NULL) {
        printf(" (no data)\n");
    } else {
        printf(" \"%s\"\n", in.data[ip]);
    }
}

void print_bytecode(Bytecode in) {
    for (unsigned int i = 0; i < in.cnt; i++) {
        print_single_bytecode(in, i);
    }
}

void bytecode_recursive_free(Bytecode* bc) {
    for (unsigned int i = 0; i < bc->cnt; i++) {
        free(bc->data[i]);
    }
    free(bc->tag);
    free(bc);
}

void bytecode_init(Bytecode* uninitialized) { 
    *uninitialized = (Bytecode){ .tag = NULL, .data = NULL, .cnt = 0, .contiguousc = 0 };
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


// once all appending actions have been done, make sure
// to call foci_merge_overlapping to deduplicate spans
void foci_append(Foci* foci, char* start, char* end) {    
    if (foci->contiguousc <= (foci->cnt + 1) * sizeof(Span)) {
        foci->contiguousc = (foci->cnt + 1) * sizeof(Span) * 2;
        foci->s = realloc(foci->s, foci->contiguousc);
    }
    foci->s[foci->cnt].start = start;
    foci->s[foci->cnt].end = end;
    foci->cnt++;
}

char* foci_min_start(Foci foci) {
    char* min_start = foci.s[0].start;
    for (unsigned long i = 1; i < foci.cnt; i++) {
        if (foci.s[i].start < min_start) {
            min_start = foci.s[i].start;
        }
    }
    return min_start;
}

char* foci_max_end(Foci foci) {
    char* max_end = foci.s[0].end;
    for (unsigned long i = 1; i < foci.cnt; i++) {
        if (foci.s[i].end > max_end) {
            max_end = foci.s[i].end;
        }
    }
    return max_end;
}

// returns the index of the first span focusing on the ptr, or -1.
long foci_check(Foci in, char* ptr) {
    for (unsigned long i = 0; i < in.cnt; i++) {
        Span tester = in.s[i];
        if (ptr >= tester.start && ptr < tester.end) {
            return i;
        }
    }
    return -1;
}

// this creates a distinct copy, it's safe to call foci_free on the input afterwards
// TODO: support distinct foci bordering each other... iterate and check if we're in two spans?
// TODO: this breaks 0 length spans
// TODO: can some sweet, silly code guesser fix this function for me? thx...
Foci foci_merge_overlapping(Foci in) {
    Foci out;
    foci_init(&out);
    if (in.cnt == 0) {
        return out;
    }
    char* min_start = foci_min_start(in);
    char* max_end = foci_max_end(in);
    foci_append(&out, min_start, min_start+1);
    char scanning = 1;
    for (char* ptr = min_start; ptr < max_end; ptr++) {
        if (foci_check(in, ptr) >= 0) {
            if (scanning) {
                out.s[out.cnt - 1].end = ptr+1;
            } else {
                foci_append(&out, ptr, ptr+1);
                scanning = 1;
            }
        } else {
            if (scanning) {
                scanning = 0;
            }
        }
    }
    return out;
}

void print_span(Span s) {
    printf("\"%.*s\"\n", (int)(s.end - s.start), s.start);
}

void print_foci(Foci f) {
    printf("Foci cnt: %ld\n", f.cnt);
    for (unsigned long i = 0; i < f.cnt; i++) {
        printf("%ld ", i);
        print_span(f.s[i]);
    }
}

typedef struct VM {            // Execution model
    unsigned long ip;          // current execution offset into Bytecode tag+data
    Bytecode bc;               // the bytecode we're executing
    char* text;                // the text to mutate
    Foci foci;                 // what text is currently focused?
} VM;

void vm_init(VM* uninitialized, char* in, Bytecode bc) {
    Foci foci;
    foci_init(&foci);
    // VMs start with everything focused
    foci_append(&foci, in, strchr(in, '\0'));
    *uninitialized = (VM){ .ip = 0, .bc = bc, .text = in, .foci = foci };
}

void vm_merge_overlapping_foci(VM* vm) {
    Foci old_foci = vm->foci;
    vm->foci = foci_merge_overlapping(vm->foci);
    foci_free(old_foci);
}

// expects vm->ip to be pointing to a BCT_DRILL tag and a regex in data
void vm_run_drill(VM* vm) {
    char* regex = vm->bc.data[vm->ip];
    unsigned long match_length = strlen(regex);
    Foci new_foci;
    foci_init(&new_foci);
    for (unsigned long i = 0; i < vm->foci.cnt; i++) {
        Span focus = vm->foci.s[i];
        unsigned long focus_length = focus.end - focus.start;
        char* haystack = focus.start;
        while (*haystack != '\0') {
            #ifdef DEBUG
            printf("calling strncmp with %s, %s, %ld, got %d\n", haystack, regex, match_length, strncmp(haystack, regex, match_length));
            #endif
            if (strncmp(haystack, regex, MIN(match_length, focus_length)) == 0) { // TODO: regex
                foci_append(&new_foci, haystack, haystack + match_length);
            } 
            haystack++;
        }
    }
    foci_free(vm->foci);
    vm->foci = new_foci;
}

// expects vm->ip to be pointing to a BCT_SUBSTITUTE tag and the sub string in data
void vm_run_substitute(VM* vm) {
    char* subby = vm->bc.data[vm->ip];
    // calculate length of string post-substitution
    unsigned long foci_length = 0;
    for (unsigned long i = 0; i < vm->foci.cnt; i++) {
        foci_length += vm->foci.s[i].end - vm->foci.s[i].end;
    }
    char* new_text = calloc(strlen(vm->text) - foci_length + strlen(subby) * vm->foci.cnt, sizeof(char));
    // copy focus structure
    Foci new_foci;
    foci_init(&new_foci);
    for (unsigned int i = 0; i < vm->foci.cnt; i++) {
        char* new_start = new_text + (vm->foci.s[i].start - vm->text);
        char* new_end = new_text + (vm->foci.s[i].end - vm->text);
        foci_append(&new_foci, new_start, new_end);
    }
    // build new_text from successive concatenations
    char* done = vm->text;
    unsigned long i = 0;
    for (; i < vm->foci.cnt; i++) {
        Span focus = vm->foci.s[i];
        strncat(new_text, done, focus.start - done);
        strcat(new_text, subby);
        done = focus.end;
        // adjust foci affected by this substitution, including pointing them to new_text
        unsigned long text_length_change = strlen(subby) - (focus.end - focus.start);
        new_foci.s[i].end += text_length_change;
        for (unsigned long j = i+1; j < vm->foci.cnt; j++) {
            new_foci.s[j].start += text_length_change;
            new_foci.s[j].end += text_length_change;
        }
    }
    strcat(new_text, done);
    free(vm->text);
    vm->text = new_text;
    foci_free(vm->foci);
    vm->foci = new_foci;
}

// expects vm->ip to be pointing to a BCT_AROUND tag
void vm_run_around(VM* vm) {    
    Foci new_foci;
    foci_init(&new_foci);
    for (unsigned long i = 0; i < vm->foci.cnt; i++) {
        Span focus = vm->foci.s[i];
        foci_append(&new_foci, focus.start, focus.start);
        foci_append(&new_foci, focus.end, focus.end);
    }
    foci_free(vm->foci);
    vm->foci = new_foci;
}

// expects vm->ip to be pointing to a BCT_STARTS tag
void vm_run_starts(VM* vm) {    
    Foci new_foci;
    foci_init(&new_foci);
    for (unsigned long i = 0; i < vm->foci.cnt; i++) {
        Span focus = vm->foci.s[i];
        foci_append(&new_foci, focus.start, focus.start);
    }
    foci_free(vm->foci);
    vm->foci = new_foci;
}

// expects vm->ip to be pointing to a BCT_ENDS tag
void vm_run_ends(VM* vm) {    
    Foci new_foci;
    foci_init(&new_foci);
    for (unsigned long i = 0; i < vm->foci.cnt; i++) {
        Span focus = vm->foci.s[i];
        foci_append(&new_foci, focus.end, focus.end);
    }
    foci_free(vm->foci);
    vm->foci = new_foci;
}

// expects vm->ip to be pointing to a BCT_COMPLEMENT tag
void vm_run_complement(VM* vm) {    
    Foci new_foci;
    foci_init(&new_foci);
    char* done = vm->text;
    for (unsigned long i = 0; i < vm->foci.cnt; i++) {
        Span focus = vm->foci.s[i];
        foci_append(&new_foci, done, focus.start);
        done = focus.end;
    }
    foci_append(&new_foci, done, strchr(vm->text, '\0')); 
    foci_free(vm->foci);
    vm->foci = new_foci;
}


void print_vm(VM vm) {
    printf("Text: %s\n", vm.text);
    printf("Bytecode:\n");
    print_bytecode(vm.bc);
    printf("IP: %ld\n", vm.ip);
    printf("Foci:\n");
    print_foci(vm.foci);
    puts("");
    puts("");
}

// returns pointer to mutated text on success, NULL on error
char* vm_run(VM vm) {
    #ifdef DEBUG
    printf("Initial state:\n");
    print_vm(vm);
    #endif
    for (; vm.ip < vm.bc.cnt; vm.ip++) {
        switch (vm.bc.tag[vm.ip]) {
            case BCT_DRILL:
                vm_run_drill(&vm);
                // TODO: figure out if merging overlapping foci is needed here
                // TODO: can test with a drill like /ss/ and a string like "sss"
                // vm_merge_overlapping_foci(&vm);
                break;
            case BCT_SUBSTITUTE:
                vm_run_substitute(&vm);
                break;
            case BCT_AROUND:
                vm_run_around(&vm);
                break;
            case BCT_STARTS:
                vm_run_starts(&vm);
                break;
            case BCT_ENDS:
                vm_run_ends(&vm);
                break;
            case BCT_COMPLEMENT:
                vm_run_complement(&vm);
                break;
            default:
                printf("Got unimplemented bytecode ");
                print_single_bytecode(vm.bc, vm.ip);
        }
        #ifdef DEBUG
        print_vm(vm);
        #endif
    }
    return vm.text;
}



/* ~~~~~~~~~~~ Parser ~~~~~~~~~~~ */
#define PAD_SIZE 1024
typedef struct ParseState {
    Bytecode out; // Did this parser produce output? "No" represented by a zero-length Bytecode
    char* altout; // Some parsers produce strings instead
    char* rest; // The rest of the input stream
} ParseState;

void parsestate_init(ParseState* uninitialized, char* unparsed) {
    Bytecode bc;
    bytecode_init(&bc);
    *uninitialized = (ParseState){ .out = bc, .altout = NULL, .rest = unparsed };
}

void parsestate_free(ParseState ps) {
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
    if (subparse.out.cnt > 0) {
        bytecode_concat(&p->out, subparse.out);
    }
    p->rest = subparse.rest;
}

// parses regexp/, so like a regexp missing the leading slash & ending on a slash, places it in altout
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

// parses quote", so like a string missing the leading quote & ending on a quote, places it in altout
ParseState parse_quote(char* in) {
    ParseState o = { .altout = "", .rest = in };
    char pad[PAD_SIZE] = "";
    unsigned int padc = 0;
    while (padc < PAD_SIZE) {
        char c = *o.rest;
        switch (c) {
            case '\0':
                die("EOF while parsing quote");
                break;
            case '"':
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

// parses s/regexp/
ParseState parse_substitute(char* in) {
    ParseState o;
    parsestate_init(&o, in);
    if (*(o.rest++) != 's') {
        die("Expected s at start of substitution");
    }
    if (*(o.rest++) != '/') {
        die("Expected s/ at start of substitution");
    }
    ParseState sub = parse_regexp(o.rest);
    consume_subparse(&o, sub);
    bytecode_append(&o.out, BCT_SUBSTITUTE, strdup(sub.altout));
    parsestate_free(sub);
    if (*(o.rest++) != '/') {
        die("Expected / at the end of substitution");
    }
    return o;
}

// parses /regexp/
ParseState parse_drill(char* in) {
    ParseState o;
    parsestate_init(&o, in);
    if (*(o.rest++) != '/') {
        die("Expected / at start of drill");
    }
    ParseState sub = parse_regexp(o.rest);
    consume_subparse(&o, sub);
    bytecode_append(&o.out, BCT_DRILL, strdup(sub.altout));
    parsestate_free(sub);
    if (*(o.rest++)!= '/') {
        die("Expected / at end of drill");
    }

    return o;
}


ParseState parse_root(char* in) {
    ParseState o;
    parsestate_init(&o, in);
    while (1) {
        char c = *o.rest;
        ParseState sub;
        switch (c) {
            case '\0':
                goto done;
            case 's':
                sub = parse_substitute(o.rest);
                consume_subparse(&o, sub);
                parsestate_free(sub);
                break;
            case '/':
                sub = parse_drill(o.rest);
                consume_subparse(&o, sub);
                parsestate_free(sub);
                break;
            case '^':
                o.rest++;
                bytecode_append(&o.out, BCT_STARTS, NULL);
                break;
            case '$':
                o.rest++;
                bytecode_append(&o.out, BCT_ENDS, NULL);
                break;
            case '%':
                o.rest++;
                bytecode_append(&o.out, BCT_AROUND, NULL);
                break;
            case '@':
                o.rest++;
                bytecode_append(&o.out, BCT_COMPLEMENT, NULL);
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
    return parse_root(in).out;
}

// requires a heap-allocated text
char* run(char* progstr, char* text) {
    Bytecode prog = parse(progstr);
    #ifdef DEBUG
    print_bytecode(prog);
    puts("");
    #endif
    VM vm;
    vm_init(&vm, text, prog);
    return vm_run(vm);
}

int main(int argc, char** argv) {
    printf("%s\n", run("/hello,/ s// s/I'm ready to conquer the/", strdup("hello, world!")));
    printf("%s\n", run("^ s/This text is at the start /", strdup("hello, world!")));
    printf("%s\n", run("$ s/ This text is at the end/", strdup("hello, world!")));
    printf("%s\n", run("% s/ This text is at the start and the end /", strdup("hello, world!")));
    printf("%s\n", run("/l/ % s/ol/", strdup("hello, world!")));
    printf("%s\n", run("/,/ @ s/lol/", strdup("hello,,,  world! i love to sing, dance, and play video games!")));
    return 0;
}

// two men walk abreast
// into the dark night sky ablaze with
// ten thousand dying stars