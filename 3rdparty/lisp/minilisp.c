// This software is in the public domain.

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sys/mman.h>

#include "sqlite3.h"

extern void lisp_write_svg(char *txt, int len);

static const unsigned char *code_line;
static int code_ix;
static char out[256];
static int outlen;
static char sqlcmd[512];
static int sqlcmdlen;
static int print_stdout = 0;
static int print_sqlcmd = 0;

static int GETCHAR(void) {
    int r = code_line[code_ix];
    if (!r) return EOF;
    code_ix++;
    return r;
}

static int UNGETC(int c, FILE *stream) {
    if (code_ix == 0) return EOF;
    code_ix--;
    return c;
}

#define printf(_fmt...) do {						\
	if (print_sqlcmd) sqlcmdlen += snprintf(sqlcmd+sqlcmdlen, sizeof(sqlcmd)-sqlcmdlen, _fmt); \
	else if (print_stdout) printf(_fmt);				\
	else {								\
	    outlen = snprintf(out, sizeof(out), _fmt);			\
	    lisp_write_svg(out, outlen);				\
	}								\
    } while (0)

#define getchar GETCHAR
#define ungetc UNGETC

static int READ_NUMBER(void) {
    char *endp = NULL;
    int val = strtol((char *)&code_line[code_ix], &endp, 0);
    while ((char *)(&code_line[code_ix]) < endp) code_ix++;
    return val;
}

static __attribute((noreturn)) void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

//======================================================================
// Lisp objects
//======================================================================

// The Lisp object type
enum {
    // Regular objects visible from the user
    TINT = 1,
    TCELL,
    TSYMBOL,
    TPRIMITIVE,
    TFUNCTION,
    TMACRO,
    TENV,
    TSTRING,
    // The marker that indicates the object has been moved to other location by GC. The new location
    // can be found at the forwarding pointer. Only the functions to do garbage collection set and
    // handle the object of this type. Other functions will never see the object of this type.
    TMOVED,
    // Const objects. They are statically allocated and will never be managed by GC.
    TTRUE,
    TNIL,
    TDOT,
    TCPAREN,
};

// Typedef for the primitive function
struct Obj;
typedef struct Obj *Primitive(void *root, struct Obj **env, struct Obj **args);

// The object type
typedef struct Obj {
    // The first word of the object represents the type of the object. Any code that handles object
    // needs to check its type first, then access the following union members.
    int type;

    // The total size of the object, including "type" field, this field, the contents, and the
    // padding at the end of the object.
    int size;

    // Object values.
    union {
        // Int
        int value;
        // Cell
        struct {
            struct Obj *car;
            struct Obj *cdr;
        };
        // Symbol
        char name[1];
        // Primitive
        Primitive *fn;
        // Function or Macro
        struct {
            struct Obj *params;
            struct Obj *body;
            struct Obj *env;
        };
        // Environment frame. This is a linked list of association lists
        // containing the mapping from symbols to their value.
        struct {
            struct Obj *vars;
            struct Obj *up;
        };
        // Forwarding pointer
        void *moved;
    };
} Obj;

// Constants
static Obj *True = &(Obj){ TTRUE };
static Obj *Nil = &(Obj){ TNIL };
static Obj *Dot = &(Obj){ TDOT };
static Obj *Cparen = &(Obj){ TCPAREN };

// Strings variables
typedef struct Obj2 {
    int type;
    int size;
    char name[32];
} Obj2;

#define DEF_STR(_name) static Obj *_name##_ = (Obj *)(&(Obj2){ TSTRING })
#define DEF_INT(_name) static Obj *_name##_ = &(Obj){ TINT }
#define NAME2OBJ(_a) { #_a, &_a##_ }

#define INTCPY(_what) do {						\
	Obj *_o = _what##_;						\
	_o->value = _what;						\
    } while (0)


#define STRCPY(_what) do {						\
	Obj2 *_o = (Obj2 *) _what##_;					\
	strncpy(_o->name, _what, 31);					\
	_o->name[31] = 0;						\
    } while (0)

#define STRCPY_(_who, _what) do {					\
	Obj2 *_o = (Obj2 *) _what##_##_who##_;				\
	strncpy(_o->name, _what, 31);					\
	_o->name[31] = 0;						\
    } while (0)

static struct Obj *get_judoka(void *root, struct Obj **env, struct Obj **args);
static Obj *judoka_ = &(Obj){ .type = TFUNCTION, .fn = get_judoka };

DEF_INT(compix_1);
DEF_STR(first_1);
DEF_STR(last_1);
DEF_STR(club_1);
DEF_STR(country_1);

DEF_INT(compix_2);
DEF_STR(first_2);
DEF_STR(last_2);
DEF_STR(club_2);
DEF_STR(country_2);

DEF_INT(compix);
DEF_STR(first);
DEF_STR(last);
DEF_STR(club);
DEF_STR(country);
DEF_STR(category);
DEF_STR(regcategory);
DEF_STR(compid);
DEF_STR(coachid);
DEF_INT(birthyear);
DEF_INT(grade);
DEF_INT(weight);
DEF_INT(seeding);
DEF_INT(clubseeding);
DEF_INT(gender);
DEF_STR(comment);

DEF_INT(wins_1);
DEF_INT(res_1);

DEF_INT(catix);
DEF_INT(number);
DEF_INT(score_1);
DEF_INT(score_2);
DEF_INT(points_1);
DEF_INT(points_2);
DEF_INT(match_time);
DEF_INT(order);
DEF_INT(tatami);
DEF_INT(group);
DEF_INT(flags);
DEF_INT(forcedtatami);
DEF_INT(forcednumber);
DEF_INT(date);
DEF_INT(legend);
DEF_INT(roundnum);

static struct {
    char *name;
    Obj **obj;
} judoshiai_objs[] = {
    NAME2OBJ(compix_1),
    NAME2OBJ(first_1),
    NAME2OBJ(last_1),
    NAME2OBJ(club_1),
    NAME2OBJ(country_1),

    NAME2OBJ(wins_1),
    NAME2OBJ(res_1),

    NAME2OBJ(compix_2),
    NAME2OBJ(first_2),
    NAME2OBJ(last_2),
    NAME2OBJ(club_2),
    NAME2OBJ(country_2),

    NAME2OBJ(compix),
    NAME2OBJ(first),
    NAME2OBJ(last),
    NAME2OBJ(club),
    NAME2OBJ(country),
    NAME2OBJ(category),
    NAME2OBJ(regcategory),
    NAME2OBJ(compid),
    NAME2OBJ(coachid),
    NAME2OBJ(comment),
    NAME2OBJ(birthyear),
    NAME2OBJ(grade),
    NAME2OBJ(weight),
    NAME2OBJ(flags),
    NAME2OBJ(seeding),
    NAME2OBJ(clubseeding),
    NAME2OBJ(gender),

    NAME2OBJ(catix),
    NAME2OBJ(number),
    NAME2OBJ(score_1),
    NAME2OBJ(score_2),
    NAME2OBJ(points_1),
    NAME2OBJ(points_2),
    NAME2OBJ(match_time),
    NAME2OBJ(order),
    NAME2OBJ(tatami),
    NAME2OBJ(group),
    NAME2OBJ(flags),
    NAME2OBJ(forcedtatami),
    NAME2OBJ(forcednumber),
    NAME2OBJ(date),
    NAME2OBJ(legend),
    NAME2OBJ(roundnum),

    NAME2OBJ(judoka),
    {NULL, NULL}
};

void lisp_set_match(int catix, int number, int compix_1, int compix_2,
		    int score_1, int score_2, int points_1, int points_2,
		    int match_time, int order, int tatami, int group, int flags,
		    int forcedtatami, int forcednumber, int date, int legend, int roundnum)
{
    INTCPY(catix);
    INTCPY(number);
    INTCPY(compix_1);
    INTCPY(compix_2);
    INTCPY(score_1);
    INTCPY(score_2);
    INTCPY(points_1);
    INTCPY(points_2);
    INTCPY(match_time);
    INTCPY(order);
    INTCPY(tatami);
    INTCPY(group);
    INTCPY(flags);
    INTCPY(forcedtatami);
    INTCPY(forcednumber);
    INTCPY(date);
    INTCPY(legend);
    INTCPY(roundnum);
}

void lisp_set_competitor(int who, int compix,
			 const char *first, const char *last,
			 const char *club, const char *country,
			 const char *regcategory, const char *category,
			 const char *compid,
			 const char *comment, const char *coachid,
			 int birthyear, int grade,
			 int weight, int seeding, int clubseeding, int gender)
{
    if (who == 1) {
	STRCPY_(1, first);
	STRCPY_(1, last);
	STRCPY_(1, club);
	STRCPY_(1, country);
    } else if (who == 2) {
	STRCPY_(2, first);
	STRCPY_(2, last);
	STRCPY_(2, club);
	STRCPY_(2, country);
    } else {
	STRCPY(first);
	STRCPY(last);
	STRCPY(club);
	STRCPY(country);
	STRCPY(comment);
	STRCPY(compid);
	STRCPY(coachid);
	INTCPY(compix);
	INTCPY(birthyear);
	INTCPY(grade);
	STRCPY(regcategory);
	STRCPY(category);
	INTCPY(weight);
	INTCPY(seeding);
	INTCPY(clubseeding);
	INTCPY(gender);
    }
}

void lisp_set_comp_points(int ix, int wins, int pts, int res)
{
    compix_1_->value = ix;
    wins_1_->value = wins;
    points_1_->value = pts;
    res_1_->value = res;
}

#define SYMBOL_MAX_LEN 200

// The list containing all symbols. Such data structure is traditionally called the "obarray", but I
// avoid using it as a variable name as this is not an array but a list.
static Obj *Symbols;

// Strings
static Obj *Strings;

//======================================================================
// Memory management
//======================================================================

// The size of the heap in byte
static size_t memory_size = 65536*2;
#define MEMORY_SIZE memory_size

// The pointer pointing to the beginning of the current heap
static void *memory;

// The pointer pointing to the beginning of the old heap
static void *from_space;

// The number of bytes allocated from the heap
static size_t mem_nused = 0;

// Flags to debug GC
static bool gc_running = false;
static bool debug_gc = !false;
static bool always_gc = false;

static void gc(void *root);

// Currently we are using Cheney's copying GC algorithm, with which the available memory is split
// into two halves and all objects are moved from one half to another every time GC is invoked. That
// means the address of the object keeps changing. If you take the address of an object and keep it
// in a C variable, dereferencing it could cause SEGV because the address becomes invalid after GC
// runs.
//
// In order to deal with that, all access from C to Lisp objects will go through two levels of
// pointer dereferences. The C local variable is pointing to a pointer on the C stack, and the
// pointer is pointing to the Lisp object. GC is aware of the pointers in the stack and updates
// their contents with the objects' new addresses when GC happens.
//
// The following is a macro to reserve the area in the C stack for the pointers. The contents of
// this area are considered to be GC root.
//
// Be careful not to bypass the two levels of pointer indirections. If you create a direct pointer
// to an object, it'll cause a subtle bug. Such code would work in most cases but fails with SEGV if
// GC happens during the execution of the code. Any code that allocates memory may invoke GC.

#define ROOT_END ((void *)-1)

#define ADD_ROOT(size)                          \
    void *root_ADD_ROOT_[size + 2];             \
    root_ADD_ROOT_[0] = root;                   \
    for (int i = 1; i <= size; i++)             \
        root_ADD_ROOT_[i] = NULL;               \
    root_ADD_ROOT_[size + 1] = ROOT_END;        \
    root = root_ADD_ROOT_

#define DEFINE1(var1)                           \
    ADD_ROOT(1);                                \
    Obj **var1 = (Obj **)(root_ADD_ROOT_ + 1)

#define DEFINE2(var1, var2)                     \
    ADD_ROOT(2);                                \
    Obj **var1 = (Obj **)(root_ADD_ROOT_ + 1);  \
    Obj **var2 = (Obj **)(root_ADD_ROOT_ + 2)

#define DEFINE3(var1, var2, var3)               \
    ADD_ROOT(3);                                \
    Obj **var1 = (Obj **)(root_ADD_ROOT_ + 1);  \
    Obj **var2 = (Obj **)(root_ADD_ROOT_ + 2);  \
    Obj **var3 = (Obj **)(root_ADD_ROOT_ + 3)

#define DEFINE4(var1, var2, var3, var4)         \
    ADD_ROOT(4);                                \
    Obj **var1 = (Obj **)(root_ADD_ROOT_ + 1);  \
    Obj **var2 = (Obj **)(root_ADD_ROOT_ + 2);  \
    Obj **var3 = (Obj **)(root_ADD_ROOT_ + 3);  \
    Obj **var4 = (Obj **)(root_ADD_ROOT_ + 4)

// Round up the given value to a multiple of size. Size must be a power of 2. It adds size - 1
// first, then zero-ing the least significant bits to make the result a multiple of size. I know
// these bit operations may look a little bit tricky, but it's efficient and thus frequently used.
static inline size_t roundup(size_t var, size_t size) {
    return (var + size - 1) & ~(size - 1);
}

// Allocates memory block. This may start GC if we don't have enough memory.
static Obj *alloc(void *root, int type, size_t size) {
    // The object must be large enough to contain a pointer for the forwarding pointer. Make it
    // larger if it's smaller than that.
    size = roundup(size, sizeof(void *));

    // Add the size of the type tag and size fields.
    size += offsetof(Obj, value);

    // Round up the object size to the nearest alignment boundary, so that the next object will be
    // allocated at the proper alignment boundary. Currently we align the object at the same
    // boundary as the pointer.
    size = roundup(size, sizeof(void *));

    // If the debug flag is on, allocate a new memory space to force all the existing objects to
    // move to new addresses, to invalidate the old addresses. By doing this the GC behavior becomes
    // more predictable and repeatable. If there's a memory bug that the C variable has a direct
    // reference to a Lisp object, the pointer will become invalid by this GC call. Dereferencing
    // that will immediately cause SEGV.
    if (always_gc && !gc_running)
        gc(root);

    // Otherwise, run GC only when the available memory is not large enough.
    if (!always_gc && MEMORY_SIZE < mem_nused + size) {
	fprintf(stderr, "call gb: memsize=%ld, nused=%ld size=%ld\n",
		MEMORY_SIZE, mem_nused, size);
        gc(root);
	if (mem_nused > MEMORY_SIZE/2) {
	    if (debug_gc)
		fprintf(stderr, "mem_nused=%ld MEMORY_SIZE=%ld: double mem\n",
			mem_nused, MEMORY_SIZE);
	    memory_size *= 2;
	    gc(root);
	}
    }

    // Terminate the program if we couldn't satisfy the memory request. This can happen if the
    // requested size was too large or the from-space was filled with too many live objects.
    if (MEMORY_SIZE < mem_nused + size)
        error("Memory exhausted");

    // Allocate the object.
    Obj *obj = memory + mem_nused;
    obj->type = type;
    obj->size = size;
    mem_nused += size;
    return obj;
}

//======================================================================
// Garbage collector
//======================================================================

// Cheney's algorithm uses two pointers to keep track of GC status. At first both pointers point to
// the beginning of the to-space. As GC progresses, they are moved towards the end of the
// to-space. The objects before "scan1" are the objects that are fully copied. The objects between
// "scan1" and "scan2" have already been copied, but may contain pointers to the from-space. "scan2"
// points to the beginning of the free space.
static Obj *scan1;
static Obj *scan2;

// Moves one object from the from-space to the to-space. Returns the object's new address. If the
// object has already been moved, does nothing but just returns the new address.
static inline Obj *forward(Obj *obj) {
    // If the object's address is not in the from-space, the object is not managed by GC nor it
    // has already been moved to the to-space.
    ptrdiff_t offset = (uint8_t *)obj - (uint8_t *)from_space;
    if (offset < 0 || MEMORY_SIZE <= offset)
        return obj;

    // The pointer is pointing to the from-space, but the object there was a tombstone. Follow the
    // forwarding pointer to find the new location of the object.
    if (obj->type == TMOVED)
        return obj->moved;

    // Otherwise, the object has not been moved yet. Move it.
    Obj *newloc = scan2;
    memcpy(newloc, obj, obj->size);
    scan2 = (Obj *)((uint8_t *)scan2 + obj->size);

    // Put a tombstone at the location where the object used to occupy, so that the following call
    // of forward() can find the object's new location.
    obj->type = TMOVED;
    obj->moved = newloc;
    return newloc;
}

static void *alloc_semispace() {
    return malloc(MEMORY_SIZE);
    //return mmap(NULL, MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
}

// Copies the root objects.
static void forward_root_objects(void *root) {
    Symbols = forward(Symbols);
    Strings = forward(Strings);
    for (void **frame = root; frame; frame = *(void ***)frame) {
        for (int i = 1; frame[i] != ROOT_END; i++)
            if (frame[i])
                frame[i] = forward(frame[i]);
    }
}

// Implements Cheney's copying garbage collection algorithm.
// http://en.wikipedia.org/wiki/Cheney%27s_algorithm
static void gc(void *root) {
    assert(!gc_running);
    gc_running = true;

    // Allocate a new semi-space.
    from_space = memory;
    memory = alloc_semispace();

    // Initialize the two pointers for GC. Initially they point to the beginning of the to-space.
    scan1 = scan2 = memory;

    // Copy the GC root objects first. This moves the pointer scan2.
    forward_root_objects(root);

    // Copy the objects referenced by the GC root objects located between scan1 and scan2. Once it's
    // finished, all live objects (i.e. objects reachable from the root) will have been copied to
    // the to-space.
    while (scan1 < scan2) {
        switch (scan1->type) {
        case TINT:
        case TSYMBOL:
        case TPRIMITIVE:
	case TSTRING:
		// Any of the above types does not contain a pointer to a GC-managed object.
            break;
        case TCELL:
            scan1->car = forward(scan1->car);
            scan1->cdr = forward(scan1->cdr);
            break;
        case TFUNCTION:
        case TMACRO:
            scan1->params = forward(scan1->params);
            scan1->body = forward(scan1->body);
            scan1->env = forward(scan1->env);
            break;
        case TENV:
            scan1->vars = forward(scan1->vars);
            scan1->up = forward(scan1->up);
            break;
        default:
            error("Bug: copy: unknown type %d", scan1->type);
        }
        scan1 = (Obj *)((uint8_t *)scan1 + scan1->size);
    }

    // Finish up GC.
    free(from_space);
    //munmap(from_space, MEMORY_SIZE);
    size_t old_nused = mem_nused;
    mem_nused = (size_t)((uint8_t *)scan1 - (uint8_t *)memory);
    if (debug_gc)
        fprintf(stderr, "GC: %zu bytes out of %zu bytes copied.\n", mem_nused, old_nused);
    gc_running = false;
}

//======================================================================
// Constructors
//======================================================================

static Obj *make_int(void *root, int value) {
    Obj *r = alloc(root, TINT, sizeof(int));
    r->value = value;
    return r;
}

static Obj *cons(void *root, Obj **car, Obj **cdr) {
    Obj *cell = alloc(root, TCELL, sizeof(Obj *) * 2);
    cell->car = *car;
    cell->cdr = *cdr;
    return cell;
}

static Obj *make_symbol(void *root, char *name) {
    Obj *sym = alloc(root, TSYMBOL, strlen(name) + 1);
    strcpy(sym->name, name);
    return sym;
}

static Obj *make_primitive(void *root, Primitive *fn) {
    Obj *r = alloc(root, TPRIMITIVE, sizeof(Primitive *));
    r->fn = fn;
    return r;
}

static Obj *make_function(void *root, Obj **env, int type, Obj **params, Obj **body) {
    assert(type == TFUNCTION || type == TMACRO);
    Obj *r = alloc(root, type, sizeof(Obj *) * 3);
    r->params = *params;
    r->body = *body;
    r->env = *env;
    return r;
}

struct Obj *make_env(void *root, Obj **vars, Obj **up) {
    Obj *r = alloc(root, TENV, sizeof(Obj *) * 2);
    r->vars = *vars;
    r->up = *up;
    return r;
}

static Obj *make_string(void *root, char *name) {
    for (Obj *p = Strings; p != Nil; p = p->cdr)
        if (p && strcmp(name, p->car->name) == 0) {
            return p->car;
	}
    DEFINE1(sym);
    *sym = alloc(root, TSTRING, strlen(name) + 1);
    strcpy((*sym)->name, name);
    Strings = cons(root, sym, &Strings);
    return *sym;
}

static Obj *make_string_copy(void *root, char *name) {
    DEFINE1(sym);
    *sym = alloc(root, TSTRING, strlen(name) + 1);
    strcpy((*sym)->name, name);
    Strings = cons(root, sym, &Strings);
    return *sym;
}

// Returns ((x . y) . a)
static Obj *acons(void *root, Obj **x, Obj **y, Obj **a) {
    DEFINE1(cell);
    *cell = cons(root, x, y);
    return cons(root, cell, a);
}

//======================================================================
// Parser
//
// This is a hand-written recursive-descendent parser.
//======================================================================

const char symbol_chars[] = "~!@#$%^&*-_=+:/?<>";

static Obj *read_expr(void *root);

static int peek(void) {
    int c = getchar();
    ungetc(c, stdin);
    return c;
}

// Destructively reverses the given list.
static Obj *reverse(Obj *p) {
    Obj *ret = Nil;
    while (p != Nil) {
        Obj *head = p;
        p = p->cdr;
        head->cdr = ret;
        ret = head;
    }
    return ret;
}

// Skips the input until newline is found. Newline is one of \r, \r\n or \n.
static void skip_line(void) {
    for (;;) {
        int c = getchar();
        if (c == EOF || c == '\n')
            return;
        if (c == '\r') {
            if (peek() == '\n')
                getchar();
            return;
        }
    }
}

// Reads a list. Note that '(' has already been read.
static Obj *read_list(void *root) {
    DEFINE3(obj, head, last);
    *head = Nil;
    for (;;) {
        *obj = read_expr(root);
        if (!*obj)
            error("Unclosed parenthesis");
        if (*obj == Cparen)
            return reverse(*head);
        if (*obj == Dot) {
            *last = read_expr(root);
            if (read_expr(root) != Cparen)
                error("Closed parenthesis expected after dot");
            Obj *ret = reverse(*head);
            (*head)->cdr = *last;
            return ret;
        }
        *head = cons(root, obj, head);
    }
}

// May create a new symbol. If there's a symbol with the same name, it will not create a new symbol
// but return the existing one.
static Obj *intern(void *root, char *name) {
    for (Obj *p = Symbols; p != Nil; p = p->cdr)
        if (strcmp(name, p->car->name) == 0)
            return p->car;
    DEFINE1(sym);
    *sym = make_symbol(root, name);
    Symbols = cons(root, sym, &Symbols);
    return *sym;
}

// Reader marcro ' (single quote). It reads an expression and returns (quote <expr>).
static Obj *read_quote(void *root) {
    DEFINE2(sym, tmp);
    *sym = intern(root, "quote");
    *tmp = read_expr(root);
    *tmp = cons(root, tmp, &Nil);
    *tmp = cons(root, sym, tmp);
    return *tmp;
}

#if 0
static int read_number(int val) {
    while (isdigit(peek()))
        val = val * 10 + (getchar() - '0');
    return val;
}
#endif

static Obj *read_symbol(void *root, char c) {
    char buf[SYMBOL_MAX_LEN + 1];
    buf[0] = c;
    int len = 1;
    while (isalnum(peek()) || strchr(symbol_chars, peek())) {
        if (SYMBOL_MAX_LEN <= len)
            error("Symbol name too long");
        buf[len++] = getchar();
    }
    buf[len] = '\0';
    return intern(root, buf);
}

static Obj *read_string(void *root, char c) {
    char buf[SYMBOL_MAX_LEN + 1];
    int len = 0;
    int ch = getchar();

    while (ch != c && len < SYMBOL_MAX_LEN) {
	if (ch == '\\') {
	    ch = getchar();
	    if (ch == 'n') ch = '\n';
	    else if (ch == 'r') ch = '\r';
	    else if (ch == 't') ch = '\t';
	}
	buf[len++] = ch;
	ch = getchar();
    }

    buf[len] = 0;

    DEFINE1(sym);
    *sym = make_string(root, buf);
    return *sym;
}

static int is_js_name(int c) {
    return isalnum(c) || c == '_';
}

static Obj *read_expr(void *root) {
    for (;;) {
        int c = getchar();
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
            continue;
        if (c == EOF || c == 0)
            return NULL;
        if (c == ';') {
            skip_line();
            continue;
        }
        if (c == '(')
            return read_list(root);
        if (c == ')')
            return Cparen;
        if (c == '.')
            return Dot;
        if (c == '\'')
            return read_quote(root);
        if (isdigit(c)) {
#if 1
	    ungetc(c, stdin);
            return make_int(root, READ_NUMBER());
#else
            return make_int(root, read_number(c - '0'));
#endif
	}
        if (c == '-' && isdigit(peek())) {
#if 1
            return make_int(root, -READ_NUMBER());
#else
            return make_int(root, -read_number(0));
#endif
	}
	if (c == '%' && is_js_name(peek())) {
	    // potentially judoshiai variable
	    char name[32];
	    int i = 0;
	    while (is_js_name(peek())) {
		c = getchar();
		name[i] = c;
		if (i < 31) i++;
	    }
	    name[i] = 0;
	    for (i = 0; judoshiai_objs[i].name; i++) {
		if (!strcmp(judoshiai_objs[i].name, name))
		    return *judoshiai_objs[i].obj;
	    }
	    error("Cannot find JudoShiai object %s", name);
	}
        if (isalpha(c) || strchr(symbol_chars, c))
            return read_symbol(root, c);
	if (c == '"')
		return read_string(root, c);
        error("Don't know how to handle %c", c);
    }
}

// Prints the given object.
static void print(Obj *obj) {
    switch (obj->type) {
    case TCELL:
        printf("(");
        for (;;) {
            print(obj->car);
            if (obj->cdr == Nil)
                break;
            if (obj->cdr->type != TCELL) {
                printf(" . ");
                print(obj->cdr);
                break;
            }
            printf(" ");
            obj = obj->cdr;
        }
        printf(")");
        return;

#define CASE(type, ...)                         \
    case type:                                  \
        printf(__VA_ARGS__);                    \
        return
    CASE(TINT, "%d", obj->value);
    CASE(TSYMBOL, "%s", obj->name);
    CASE(TSTRING, "%s", obj->name);
    CASE(TPRIMITIVE, "<primitive>");
    CASE(TFUNCTION, "<function>");
    CASE(TMACRO, "<macro>");
    CASE(TMOVED, "<moved>");
    CASE(TTRUE, "t");
    CASE(TNIL, "()");
#undef CASE
    default:
        error("Bug: print: Unknown tag type: %d", obj->type);
    }
}

static void printdbg(Obj *obj) {
    print_stdout = 1;
    print(obj);
    print_stdout = 0;
}

// Returns the length of the given list. -1 if it's not a proper list.
static int length(Obj *list) {
    int len = 0;
    for (; list->type == TCELL; list = list->cdr)
        len++;
    return list == Nil ? len : -1;
}

//======================================================================
// Evaluator
//======================================================================

static Obj *eval(void *root, Obj **env, Obj **obj);

static void add_variable(void *root, Obj **env, Obj **sym, Obj **val) {
    DEFINE2(vars, tmp);
    *vars = (*env)->vars;
    *tmp = acons(root, sym, val, vars);
    (*env)->vars = *tmp;
}

// Returns a newly created environment frame.
static Obj *push_env(void *root, Obj **env, Obj **vars, Obj **vals) {
    DEFINE3(map, sym, val);
    *map = Nil;
    for (; (*vars)->type == TCELL; *vars = (*vars)->cdr, *vals = (*vals)->cdr) {
        if ((*vals)->type != TCELL)
            error("Cannot apply function: number of argument does not match");
        *sym = (*vars)->car;
        *val = (*vals)->car;
        *map = acons(root, sym, val, map);
    }
    if (*vars != Nil)
        *map = acons(root, vars, vals, map);
    return make_env(root, map, env);
}

// Evaluates the list elements from head and returns the last return value.
static Obj *progn(void *root, Obj **env, Obj **list) {
    DEFINE2(lp, r);
    for (*lp = *list; *lp != Nil; *lp = (*lp)->cdr) {
        *r = (*lp)->car;
        *r = eval(root, env, r);
    }
    return *r;
}

// Evaluates all the list elements and returns their return values as a new list.
static Obj *eval_list(void *root, Obj **env, Obj **list) {
    DEFINE4(head, lp, expr, result);
    *head = Nil;
    for (lp = list; *lp != Nil; *lp = (*lp)->cdr) {
        *expr = (*lp)->car;
        *result = eval(root, env, expr);
        *head = cons(root, result, head);
    }
    return reverse(*head);
}

static bool is_list(Obj *obj) {
    return obj == Nil || obj->type == TCELL;
}

static Obj *apply_func(void *root, Obj **env, Obj **fn, Obj **args) {
    DEFINE3(params, newenv, body);
    *params = (*fn)->params;
    *newenv = (*fn)->env;
    *newenv = push_env(root, newenv, params, args);
    *body = (*fn)->body;
    return progn(root, newenv, body);
}

// Apply fn with args.
static Obj *apply(void *root, Obj **env, Obj **fn, Obj **args) {
    if (!is_list(*args))
        error("argument must be a list");
    if ((*fn)->type == TPRIMITIVE)
        return (*fn)->fn(root, env, args);
    if ((*fn)->type == TFUNCTION) {
        DEFINE1(eargs);
        *eargs = eval_list(root, env, args);
        return apply_func(root, env, fn, eargs);
    }
    error("not supported");
}

// Searches for a variable by symbol. Returns null if not found.
static Obj *find(Obj **env, Obj *sym) {
    for (Obj *p = *env; p != Nil; p = p->up) {
        for (Obj *cell = p->vars; cell != Nil; cell = cell->cdr) {
            Obj *bind = cell->car;
            if (sym == bind->car)
                return bind;
        }
    }
    return NULL;
}

// Expands the given macro application form.
static Obj *macroexpand(void *root, Obj **env, Obj **obj) {
    if ((*obj)->type != TCELL || (*obj)->car->type != TSYMBOL)
        return *obj;
    DEFINE3(bind, macro, args);
    *bind = find(env, (*obj)->car);
    if (!*bind || (*bind)->cdr->type != TMACRO)
        return *obj;
    *macro = (*bind)->cdr;
    *args = (*obj)->cdr;
    return apply_func(root, env, macro, args);
}

// Evaluates the S expression.
static Obj *eval(void *root, Obj **env, Obj **obj) {
    //out[0] = 0; outlen = 0;

    switch ((*obj)->type) {
    case TINT:
    case TPRIMITIVE:
    case TFUNCTION:
    case TTRUE:
    case TNIL:
    case TSTRING:
	    // Self-evaluating objects
        return *obj;
    case TSYMBOL: {
        // Variable
        Obj *bind = find(env, *obj);
        if (!bind)
            error("Undefined symbol: %s", (*obj)->name);
        return bind->cdr;
    }
    case TCELL: {
        // Function application form
        DEFINE3(fn, expanded, args);
        *expanded = macroexpand(root, env, obj);
        if (*expanded != *obj)
            return eval(root, env, expanded);
        *fn = (*obj)->car;
        *fn = eval(root, env, fn);
        *args = (*obj)->cdr;
        if ((*fn)->type != TPRIMITIVE && (*fn)->type != TFUNCTION) {
            error("The head of a list must be a function (%s)", out);
	}
        return apply(root, env, fn, args);
    }
    default:
        error("Bug: eval: Unknown tag type: %d", (*obj)->type);
    }
}

//======================================================================
// Primitive functions and special forms
//======================================================================

// 'expr
static Obj *prim_quote(void *root, Obj **env, Obj **list) {
    if (length(*list) != 1)
        error("Malformed quote");
    return (*list)->car;
}

// (cons expr expr)
static Obj *prim_cons(void *root, Obj **env, Obj **list) {
    if (length(*list) != 2)
        error("Malformed cons");
    Obj *cell = eval_list(root, env, list);
    cell->cdr = cell->cdr->car;
    return cell;
}

// (car <cell>)
static Obj *prim_car(void *root, Obj **env, Obj **list) {
    Obj *args = eval_list(root, env, list);
    if (args->car->type != TCELL || args->cdr != Nil)
        error("Malformed car");
    return args->car->car;
}

// (cdr <cell>)
static Obj *prim_cdr(void *root, Obj **env, Obj **list) {
    Obj *args = eval_list(root, env, list);
    if (args->car->type != TCELL || args->cdr != Nil)
        error("Malformed cdr");
    return args->car->cdr;
}

// (setq <symbol> expr)
static Obj *prim_setq(void *root, Obj **env, Obj **list) {
    if (length(*list) != 2 || (*list)->car->type != TSYMBOL)
        error("Malformed setq, name=%s, len=%d, type=%d",
	      (*list)->car->name, length(*list), (*list)->car->type);
    DEFINE2(bind, value);
    *bind = find(env, (*list)->car);
    if (!*bind)
        error("Unbound variable %s", (*list)->car->name);
    *value = (*list)->cdr->car;
    *value = eval(root, env, value);
    (*bind)->cdr = *value;
    return *value;
}

// (setcar <cell> expr)
static Obj *prim_setcar(void *root, Obj **env, Obj **list) {
    DEFINE1(args);
    *args = eval_list(root, env, list);
    if (length(*args) != 2 || (*args)->car->type != TCELL)
        error("Malformed setcar");
    (*args)->car->car = (*args)->cdr->car;
    return (*args)->car;
}

// (while cond expr ...)
static Obj *prim_while(void *root, Obj **env, Obj **list) {
    if (length(*list) < 2)
        error("Malformed while");
    DEFINE2(cond, exprs);
    *cond = (*list)->car;
    while (eval(root, env, cond) != Nil) {
        *exprs = (*list)->cdr;
        eval_list(root, env, exprs);
    }
    return Nil;
}

// (gensym)
static Obj *prim_gensym(void *root, Obj **env, Obj **list) {
  static int count = 0;
  char buf[10];
  snprintf(buf, sizeof(buf), "G__%d", count++);
  return make_symbol(root, buf);
}

// (+ <integer> ...)
static Obj *prim_plus(void *root, Obj **env, Obj **list) {
    int sum = 0;
    for (Obj *args = eval_list(root, env, list); args != Nil; args = args->cdr) {
        if (args->car->type != TINT)
            error("+ takes only numbers");
        sum += args->car->value;
    }
    return make_int(root, sum);
}

// (- <integer> ...)
static Obj *prim_minus(void *root, Obj **env, Obj **list) {
    Obj *args = eval_list(root, env, list);
    for (Obj *p = args; p != Nil; p = p->cdr)
        if (p->car->type != TINT)
            error("- takes only numbers");
    if (args->cdr == Nil)
        return make_int(root, -args->car->value);
    int r = args->car->value;
    for (Obj *p = args->cdr; p != Nil; p = p->cdr)
        r -= p->car->value;
    return make_int(root, r);
}

// (* <integer> ...)
static Obj *prim_mul(void *root, Obj **env, Obj **list) {
    int mul = 1;
    for (Obj *args = eval_list(root, env, list); args != Nil; args = args->cdr) {
        if (args->car->type != TINT)
            error("* takes only numbers");
        mul *= args->car->value;
    }
    return make_int(root, mul);
}

// (/ <integer> ...)
static Obj *prim_div(void *root, Obj **env, Obj **list) {
    Obj *args = eval_list(root, env, list);
    for (Obj *p = args; p != Nil; p = p->cdr)
        if (p->car->type != TINT)
            error("/ takes only numbers");
    int r = args->car->value;
    for (Obj *p = args->cdr; p != Nil; p = p->cdr)
        r /= p->car->value;
    return make_int(root, r);
}

// (% <integer> <integer>)
static Obj *prim_mod(void *root, Obj **env, Obj **list) {
    Obj *args = eval_list(root, env, list);
    if (length(args) != 2)
        error("malformed %");
    Obj *x = args->car;
    Obj *y = args->cdr->car;
    if (x->type != TINT || y->type != TINT)
        error("% takes only numbers");
    return make_int(root, x->value % y->value);
}

/* Bitwise Operations on Numbers */

// (logand <integer> ...)
static Obj *prim_bit_and(void *root, Obj **env, Obj **list) {
    Obj *args = eval_list(root, env, list);
    for (Obj *p = args; p != Nil; p = p->cdr)
        if (p->car->type != TINT)
            error("logand takes only numbers");
    int r = args->car->value;
    for (Obj *p = args->cdr; p != Nil; p = p->cdr)
        r &= p->car->value;
    return make_int(root, r);
}

// (logior <integer> ...)
static Obj *prim_bit_ior(void *root, Obj **env, Obj **list) {
    Obj *args = eval_list(root, env, list);
    for (Obj *p = args; p != Nil; p = p->cdr)
        if (p->car->type != TINT)
            error("logior takes only numbers");
    int r = args->car->value;
    for (Obj *p = args->cdr; p != Nil; p = p->cdr)
        r |= p->car->value;
    return make_int(root, r);
}

// (logxor <integer> ...)
static Obj *prim_bit_xor(void *root, Obj **env, Obj **list) {
    Obj *args = eval_list(root, env, list);
    for (Obj *p = args; p != Nil; p = p->cdr)
        if (p->car->type != TINT)
            error("logior takes only numbers");
    int r = args->car->value;
    for (Obj *p = args->cdr; p != Nil; p = p->cdr)
        r ^= p->car->value;
    return make_int(root, r);
}

// (lognot <integer>)
static Obj *prim_bit_not(void *root, Obj **env, Obj **list) {
    Obj *args = eval_list(root, env, list);
    if (args->car->type != TINT)
        error("lognot takes only number");
    return make_int(root, ~args->car->value);
}

/* Comparison Operations */

#define PRIM_COMP(_name, _prim, _cond) \
    static Obj *prim_num_##_name (void *root, Obj **env, Obj **list) {	\
	Obj *args = eval_list(root, env, list);				\
	if (length(args) != 2) error("malformed %s", _prim);		\
	Obj *x = args->car;						\
	Obj *y = args->cdr->car;					\
	if (x->type != TINT || y->type != TINT)				\
	    error("%s takes only numbers", _prim);			\
	return (_cond) ? True : Nil;					\
    }

PRIM_COMP(lt, "<", (x->value < y->value))
PRIM_COMP(gt, ">", (x->value > y->value))
PRIM_COMP(le, "<=", (x->value <= y->value))
PRIM_COMP(ge, ">=", (x->value >= y->value))
PRIM_COMP(eq, "=", (x->value == y->value))
PRIM_COMP(ne, "/=", (x->value != y->value))

/* Logical Operations on Boolean Values */

// (and ...)
static Obj *prim_and(void *root, Obj **env, Obj **list) {
    Obj *args = eval_list(root, env, list);
    Obj *res = Nil;
    for (Obj *p = args; p != Nil; p = p->cdr) {
        if (p->car == Nil)
	    return Nil;
	res = p->car;
    }
    return res;
}

// (or ...)
static Obj *prim_or(void *root, Obj **env, Obj **list) {
    Obj *args = eval_list(root, env, list);
    for (Obj *p = args; p != Nil; p = p->cdr)
        if (p->car != Nil)
	    return p->car;
    return Nil;
}

// (not arg)
static Obj *prim_not(void *root, Obj **env, Obj **list) {
    DEFINE1(arg);
    *arg = (*list)->car;
    *arg = eval(root, env, arg);
    if (*arg == Nil) return True;
    return Nil;
}

/* judoshiai specific functions */

static struct Obj *get_judoka(void *root, struct Obj **env, struct Obj **args)
{
    int val;
    DEFINE1(arg);
    *arg = (*args)->car;
    *arg = eval(root, env, arg);

    if ((*arg)->type == TINT) val = (*arg)->value;
    else if ((*arg)->type == TSTRING) val = atoi((*arg)->name);
    else return Nil;

    extern int lisp_get_data(int);
    if (lisp_get_data(val))
	return Nil;
    return True;
}

/********************************************/

static Obj *handle_function(void *root, Obj **env, Obj **list, int type) {
    if ((*list)->type != TCELL || !is_list((*list)->car) || (*list)->cdr->type != TCELL)
        error("Malformed lambda");
    Obj *p = (*list)->car;
    for (; p->type == TCELL; p = p->cdr)
        if (p->car->type != TSYMBOL)
            error("Parameter must be a symbol");
    if (p != Nil && p->type != TSYMBOL)
        error("Parameter must be a symbol");
    DEFINE2(params, body);
    *params = (*list)->car;
    *body = (*list)->cdr;
    return make_function(root, env, type, params, body);
}

// (lambda (<symbol> ...) expr ...)
static Obj *prim_lambda(void *root, Obj **env, Obj **list) {
    return handle_function(root, env, list, TFUNCTION);
}

static Obj *handle_defun(void *root, Obj **env, Obj **list, int type) {
    if ((*list)->car->type != TSYMBOL || (*list)->cdr->type != TCELL)
        error("Malformed defun");
    DEFINE3(fn, sym, rest);
    *sym = (*list)->car;
    *rest = (*list)->cdr;
    *fn = handle_function(root, env, rest, type);
    add_variable(root, env, sym, fn);
    return *fn;
}

// (defun <symbol> (<symbol> ...) expr ...)
static Obj *prim_defun(void *root, Obj **env, Obj **list) {
    return handle_defun(root, env, list, TFUNCTION);
}

// (define <symbol> expr)
static Obj *prim_define(void *root, Obj **env, Obj **list) {
    if (length(*list) != 2 || (*list)->car->type != TSYMBOL)
        error("Malformed define");
    DEFINE2(sym, value);
    *sym = (*list)->car;
    *value = (*list)->cdr->car;
    *value = eval(root, env, value);
    add_variable(root, env, sym, value);
    return *value;
}

// (defmacro <symbol> (<symbol> ...) expr ...)
static Obj *prim_defmacro(void *root, Obj **env, Obj **list) {
    return handle_defun(root, env, list, TMACRO);
}

// (macroexpand expr)
static Obj *prim_macroexpand(void *root, Obj **env, Obj **list) {
    if (length(*list) != 1)
        error("Malformed macroexpand");
    DEFINE1(body);
    *body = (*list)->car;
    return macroexpand(root, env, body);
}

// (write expr...)
static Obj *prim_write(void *root, Obj **env, Obj **list) {
    for (Obj *args = eval_list(root, env, list); args != Nil; args = args->cdr) {
        print(args->car);
    }
    return Nil;
}

// (print expr...)
static Obj *prim_print(void *root, Obj **env, Obj **list) {
    print_stdout = 1;
    prim_write(root, env, list);
    print_stdout = 0;
    return Nil;
}

// (println expr...)
static Obj *prim_println(void *root, Obj **env, Obj **list) {
    print_stdout = 1;
    prim_write(root, env, list);
    printf("\n");
    print_stdout = 0;
    return Nil;
}

// (if expr expr expr ...)
static Obj *prim_if(void *root, Obj **env, Obj **list) {
    if (length(*list) < 2)
        error("Malformed if");
    DEFINE3(cond, then, els);
    *cond = (*list)->car;
    *cond = eval(root, env, cond);
    if (*cond != Nil) {
        *then = (*list)->cdr->car;
        return eval(root, env, then);
    }
    *els = (*list)->cdr->cdr;
    return *els == Nil ? Nil : progn(root, env, els);
}

// (eq expr expr)
static Obj *prim_eq(void *root, Obj **env, Obj **list) {
    if (length(*list) != 2)
        error("Malformed eq");
    Obj *values = eval_list(root, env, list);
    return values->car == values->cdr->car ? True : Nil;
}

static Obj *prim_progn(void *root, Obj **env, Obj **list) {
    DEFINE3(lp, expr, result);
    for (lp = list; *lp != Nil; *lp = (*lp)->cdr) {
        *expr = (*lp)->car;
        *result = eval(root, env, expr);
    }
    return *result;
}

static Obj *prim_copy(void *root, Obj **env, Obj **list) {
    if ((*list)->car && (*list)->car->type == TSTRING)
	return make_string_copy(root, (*list)->car->name);
    if ((*list)->car && (*list)->car->type == TINT)
	return make_int(root, (*list)->car->value);
    return Nil;
}

// (sql expr...)
static Obj *prim_sql(void *root, Obj **env, Obj **list) {
    extern const char *db_name;
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc, row, col;
    char **tablep = NULL;
    int tablerows, tablecols;

    DEFINE4(fn, data, str, cblist);
    *fn = (*list)->car;
    *data = (*list)->cdr;

    sqlcmdlen = 0;
    print_sqlcmd = 1;
    prim_write(root, env, data);
    print_sqlcmd = 0;
    fprintf(stderr, "sqlcmd=%s\n", sqlcmd);
    rc = sqlite3_open(db_name, &db);
    if (rc)
	return Nil;
    rc = sqlite3_get_table(db, sqlcmd, &tablep, &tablerows, &tablecols, &zErrMsg);
    if (rc != SQLITE_OK && zErrMsg) {
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
	return Nil;
    }
    sqlite3_close(db);

    outlen = 0;
    out[0] = 0;
    for (row = 1; row <= tablerows; row++) {
	*cblist = Nil;
	for (col = tablecols-1; col >= 0; col--) {
	    *str = make_string(root, tablep[row*tablecols + col]);
	    *cblist = cons(root, str, cblist);
	}
	*str = intern(root, "quote");
	*cblist = cons(root, cblist, &Nil);
	*cblist = cons(root, str, cblist);
	*cblist = cons(root, cblist, &Nil);
	*cblist = cons(root, fn, cblist);
	eval(root, env, cblist);
    }
    sqlite3_free_table(tablep);
    return True;
}

static void add_primitive(void *root, Obj **env, char *name, Primitive *fn) {
    DEFINE2(sym, prim);
    *sym = intern(root, name);
    *prim = make_primitive(root, fn);
    add_variable(root, env, sym, prim);
}

static void define_constants(void *root, Obj **env) {
    DEFINE1(sym);
    *sym = intern(root, "t");
    add_variable(root, env, sym, &True);
}

static void define_primitives(void *root, Obj **env) {
    add_primitive(root, env, "quote", prim_quote);
    add_primitive(root, env, "cons", prim_cons);
    add_primitive(root, env, "car", prim_car);
    add_primitive(root, env, "cdr", prim_cdr);
    add_primitive(root, env, "setq", prim_setq);
    add_primitive(root, env, "setcar", prim_setcar);
    add_primitive(root, env, "while", prim_while);
    add_primitive(root, env, "gensym", prim_gensym);
    add_primitive(root, env, "+", prim_plus);
    add_primitive(root, env, "-", prim_minus);
    add_primitive(root, env, "*", prim_mul);
    add_primitive(root, env, "/", prim_div);
    add_primitive(root, env, "mod", prim_mod);
    add_primitive(root, env, "logand", prim_bit_and);
    add_primitive(root, env, "logior", prim_bit_ior);
    add_primitive(root, env, "logxor", prim_bit_xor);
    add_primitive(root, env, "lognot", prim_bit_not);
    add_primitive(root, env, "and", prim_and);
    add_primitive(root, env, "or", prim_or);
    add_primitive(root, env, "not", prim_not);
    add_primitive(root, env, "=", prim_num_eq);
    add_primitive(root, env, "/=", prim_num_ne);
    add_primitive(root, env, "<", prim_num_lt);
    add_primitive(root, env, ">", prim_num_gt);
    add_primitive(root, env, "<=", prim_num_le);
    add_primitive(root, env, ">=", prim_num_ge);
    add_primitive(root, env, "define", prim_define);
    add_primitive(root, env, "defun", prim_defun);
    add_primitive(root, env, "defmacro", prim_defmacro);
    add_primitive(root, env, "macroexpand", prim_macroexpand);
    add_primitive(root, env, "lambda", prim_lambda);
    add_primitive(root, env, "if", prim_if);
    add_primitive(root, env, "eq", prim_eq);
    add_primitive(root, env, "progn", prim_progn);
    add_primitive(root, env, "write", prim_write);
    add_primitive(root, env, "print", prim_print);
    add_primitive(root, env, "println", prim_println);
    add_primitive(root, env, "sql", prim_sql);
    add_primitive(root, env, "get-data-by-ix", get_judoka);
    add_primitive(root, env, "copy", prim_copy);
}

//======================================================================
// Entry point
//======================================================================

// Returns true if the environment variable is defined and not the empty string.
static bool getEnvFlag(char *name) {
    char *val = getenv(name);
    return val && val[0];
}

static void *root[4];
static Obj **env, **expr;

int lisp_init(int argc, char **argv)
{
    // Debug flags
    debug_gc = getEnvFlag("MINILISP_DEBUG_GC");
    always_gc = getEnvFlag("MINILISP_ALWAYS_GC");

    // Memory allocation
    if (memory) free(memory);
    memory = alloc_semispace();

    // Constants and primitives
    Symbols = Nil;
    Strings = Nil;
    root[0] = NULL;
    env = (Obj **)(root + 1);
    expr = (Obj **)(root + 2);
    root[3] = ROOT_END;

    *env = make_env(root, &Nil, &Nil);
    define_constants(root, env);
    define_primitives(root, env);

    //lisp_exe("(print (+ 2 3))");

    return 0;
}

char *lisp_exe(char *code)
{
    //fprintf(stderr, "\n--- LISP executing\n%s\n", code);

    code_line = (unsigned char *)code;
    code_ix = 0;
    out[0] = 0;
    outlen = 0;

    while (1) {
	*expr = read_expr(root);
	if (!*expr) {
	    return out;
	}
	if (*expr == Cparen)
	    error("Stray close parenthesis");
	if (*expr == Dot)
	    error("Stray dot");
	eval(root, env, expr);
    }
    //print(eval(root, env, expr));

    return out;
}

#ifndef SHIAI_VERSION
int lisp_get_data(int ix) { return -1; }

int main(int argc, char *argv[])
{
    if (argc < 2) return 1;
    lisp_init(argc, argv);
    char *r = lisp_exe(argv[1]);
    fprintf(stdout, "==%s==\n", r);
    return 0;
}
#endif
