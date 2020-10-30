#ifdef T
#include <cassert>
#include <cstring>
#include "templates.hpp"
#include "StackLib.hpp"
#if DEBUG_LEVEL >= DUMP_LEVEL
#include "logs.hpp"
#endif


struct TEMPLATE(stack,T) {
#if DEBUG_LEVEL >= CANARY_LEVEL
    canary_t cuckoo_low;
#endif
    int64_t size;
    int64_t capacity;
    T * ptr;
#if DEBUG_LEVEL >= DUMP_LEVEL
    const char * var_name;
    const char * file_name;
    const char * func_name;
    int64_t line;
#endif
#if DEBUG_LEVEL >= HASH_LEVEL
    hash_t hash;
#endif
#if DEBUG_LEVEL >= CANARY_LEVEL
    canary_t cuckoo_high;
#endif
};

char *find_alloc_address(T *ptr) {
    char *p = (char *)ptr;
#if DEBUG_LEVEL >= HASH_LEVEL
    p -= sizeof(hash_t);
#endif
#if DEBUG_LEVEL >= CANARY_LEVEL
    p -= sizeof(canary_t);
#endif
    return p;
}

#if DEBUG_LEVEL >= HASH_LEVEL
hash_t calc_stack_hash(struct TEMPLATE(stack,T) *stack) {
    hash_t old_hash = stack->hash;
    stack->hash = 0;
    hash_t curr_hash = calc_hash((char *)stack + sizeof(stack->cuckoo_low),
                                 (char *)stack + sizeof(*stack) - sizeof(stack->cuckoo_high));
    stack->hash = old_hash;
    return curr_hash;
}

hash_t calc_buf_hash(struct TEMPLATE(stack,T) *stack) {
    return calc_hash((char *)stack->ptr, (char *)(stack->ptr + stack->capacity));
}

hash_t get_buf_hash(struct TEMPLATE(stack,T) *stack) {
    return *((hash_t *)stack->ptr - 1);
}
#endif

#if DEBUG_LEVEL >= CANARY_LEVEL
canary_t get_buf_cuckoo_low(struct TEMPLATE(stack,T) *stack) {
    char * ptr = find_alloc_address(stack->ptr);
    return *(canary_t *)ptr;
}

canary_t get_buf_cuckoo_high(struct TEMPLATE(stack,T) *stack) {
    T *ptr = stack->ptr + stack->capacity;
    return *(canary_t *)ptr;
}
#endif



///-----------------------------------------------------------------------------------------
/// Returns 0 if stack is in valid state.
/// Returns error code elsewhere.
///-----------------------------------------------------------------------------------------
int stack_fail(struct TEMPLATE(stack,T) *stack) {
    if (stack == NULL) {
        return 1;
    }
#if DEBUG_LEVEL >= CANARY_LEVEL
    if (stack->cuckoo_low != cuckoo_standard) {
        return 2;
    }
    if (stack->cuckoo_high != cuckoo_standard) {
        return 3;
    }
#endif
#if DEBUG_LEVEL >= HASH_LEVEL
    if (calc_stack_hash(stack) != stack->hash) {
        return 4;
    }
#endif

    if (stack->size < 0) {
        return 5;
    }
    if (stack->capacity < 0) {
        return 6;
    }
    if (stack->size > stack->capacity) {
        return 7;
    }
    if (pointer_poison_name(stack->ptr) != 0) {
        return 8;
    }
    if (stack->ptr == NULL) {
        return 0;
    }
   
#if DEBUG_LEVEL >= CANARY_LEVEL
    if (get_buf_cuckoo_low(stack) != cuckoo_standard) {
        return 9;
    }
    if (get_buf_cuckoo_high(stack) != cuckoo_standard) {
        return 10;
    }
#endif

#if DEBUG_LEVEL >= HASH_LEVEL
    if (calc_buf_hash(stack) != get_buf_hash(stack)) {
        return 11;
    }
#endif
    return 0;
}

///-----------------------------------------------------------------------------------------
/// Checks whether stack is in valid state.
///-----------------------------------------------------------------------------------------
inline bool stack_ok(struct TEMPLATE(stack, T) *stack) {
    return !stack_fail(stack);
}
#if DEBUG_LEVEL >= DUMP_LEVEL
///-----------------------------------------------------------------------------------------
/// Prints a thorough dump of struct stack_T
///-----------------------------------------------------------------------------------------
void stack_dump(struct TEMPLATE(stack,T) *stack) {
    log_printf("stack_%s (%s) ", STR(T), stack_fail_code + stack_fail(stack));
    log_printf("[%p] ", stack);
    log_printf("\"%s\" [%s() %s(%lld)] {\n", stack->var_name, stack->func_name, stack->file_name, stack->line);

#if DEBUG_LEVEL >= CANARY_LEVEL
    log_printf("    cuckoo_low = %llx (standard = %llx)\n", stack->cuckoo_low, cuckoo_standard);
#endif

    log_printf("    size = %lld\n    capacity = %lld\n", stack->size, stack->capacity);
    log_printf("    ptr = %p (%s)\n", stack->ptr, ptr_poisons + pointer_poison_name(stack->ptr));
#if DEBUG_LEVEL >= HASH_LEVEL
    log_printf("    hash = %llu\n", stack->hash);
#endif

#if DEBUG_LEVEL >= CANARY_LEVEL
    log_printf("    cuckoo_high = %llx (standard = %llx)\n", stack->cuckoo_high, cuckoo_standard);
#endif
    if (stack->ptr == NULL) {
        log_printf("    Buffer not allocated (empty stack)\n");
        return;
    }
    log_printf("    data:\n");
#if DEBUG_LEVEL >= CANARY_LEVEL
    log_printf("        cuckoo_low = %llx (standard = %llx)\n", get_buf_cuckoo_low(stack), cuckoo_standard);
#endif
#if DEBUG_LEVEL >= HASH_LEVEL
    log_printf("        hash = %lld\n", get_buf_hash(stack));
#endif
    for (int i = 0; i < stack->capacity; i++) {
        if (i < stack->size) {
            log_printf("       *");
        } else {
            log_printf("        ");
        }
        log_printf("[%d] = ", i);
        char buf[50];
        to_str(stack->ptr[i], buf);
        log_printf("%s\n", buf);
    }
#if DEBUG_LEVEL >= CANARY_LEVEL
    log_printf("        cuckoo_high = %llx (standard = %llx)\n", get_buf_cuckoo_high(stack), cuckoo_standard);
    log_printf("}\n");
#endif
}
#endif

///---------------------------------------------------------------------------------------------------
/// Allocates stack buffer as follows
/// |      canary      |     hash     | stack[0]| stack[1]|...|     canary     |
/// | sizeof(canary_t) |sizeof(hash_t)|sizeof(T)| ........... |sizeof(canary_t)|
///                       stack->ptr = ^
/// canary and hash fields are optional
///---------------------------------------------------------------------------------------------------
void allocate(struct TEMPLATE(stack,T) *stack, size_t capacity) {
#if DEBUG_LEVEL == CANARY_LEVEL
    size_t reserve = capacity * sizeof(T) + 2 * sizeof(canary_t);
    char *ptr = (char *)calloc(reserve, sizeof(*ptr));
    *(canary_t *)ptr = cuckoo_standard; //канарейка в начале
    *(canary_t *)(ptr + reserve - sizeof(canary_t)) = cuckoo_standard;//канарейка в конце
    stack->ptr = (T *)(ptr + sizeof(canary_t));
#elif DEBUG_LEVEL == HASH_LEVEL
    size_t reserve = capacity * sizeof(T) + 2 * sizeof(canary_t) + sizeof(hash_t);
    char *ptr = (char *)calloc(reserve, sizeof(*ptr));
    *(canary_t *)ptr = cuckoo_standard; //канарейка в начале
    *(canary_t *)(ptr + reserve - sizeof(canary_t)) = cuckoo_standard; //канарейка в конце
    
    stack->ptr = (T *)(ptr + sizeof(canary_t) + sizeof(hash_t));
    
    *(hash_t *)(ptr + sizeof(canary_t)) = calc_buf_hash(stack);//хеш, без канареек
#else
    stack->ptr = (T *)calloc(capacity, sizeof(T));
#endif
}

void reallocate(struct TEMPLATE(stack, T) *stack, size_t capacity) {    
    if (stack->ptr == NULL) {
        stack->capacity = capacity;
        allocate(stack, capacity);
        return;
    }
    char *old_ptr      = (char *)stack->ptr;
    size_t old_reserve = stack->capacity * sizeof(T);
    size_t new_reserve = capacity * sizeof(T);
#if DEBUG_LEVEL >= HASH_LEVEL
    old_ptr     -= sizeof(hash_t);
    old_reserve += sizeof(hash_t);
    new_reserve += sizeof(hash_t);
#endif
#if DEBUG_LEVEL >= CANARY_LEVEL
    old_ptr     -= sizeof(canary_t);
    old_reserve += sizeof(canary_t);  //Эта переменная не учитывают канарейку на конце, намеренно.
    new_reserve += sizeof(canary_t) * 2; 
#endif

    char *new_ptr = (char*)realloc(old_ptr, new_reserve);
    memset(new_ptr + old_reserve, 0, new_reserve - old_reserve);
    stack->capacity = capacity;

#if DEBUG_LEVEL >= CANARY_LEVEL
    new_reserve -= sizeof(canary_t);
    *(canary_t *)(new_ptr + new_reserve) = cuckoo_standard;
    new_ptr += sizeof(canary_t);
#endif
#if DEBUG_LEVEL >= HASH_LEVEL
    new_ptr += sizeof(hash_t);
    stack->ptr = (T *)new_ptr;
    *(hash_t *)(new_ptr - sizeof(hash_t)) = calc_buf_hash(stack);
#else
    stack->ptr = (T *)new_ptr;
#endif
}

#if DEBUG_LEVEL >= DUMP_LEVEL
#define stack_init(stack, capacity)  \
    (stack)->var_name = #stack;     \
    (stack)->line = __LINE__;       \
    (stack)->file_name = __FILE__;  \
    (stack)->func_name = __FUNCTION__;\
    stack_init_internal(stack, capacity);
#else
#define stack_init(stack, capacity)  \
    stack_init_internal(stack, capacity);
#endif

void stack_init_internal(struct TEMPLATE(stack,T) *stack, size_t capacity = 100) {
    assert(stack != NULL);
#if DEBUG_LEVEL >= CANARY_LEVEL
    stack->cuckoo_low = stack->cuckoo_high = cuckoo_standard;
#endif
    stack->size = 0;
    stack->capacity = capacity;
    if (stack->capacity != 0) {
        allocate(stack, capacity);
    } else {
        stack->ptr = NULL;
    }

#if DEBUG_LEVEL >= HASH_LEVEL
    stack->hash = calc_stack_hash(stack);
#endif
}

void stack_destruct(struct TEMPLATE(stack,T) *stack) {
#if DEBUG_LEVEL >= DUMP_LEVEL
    if (!stack_ok(stack)) {
        stack_dump(stack);
        assert(!"OK before destruction");
    }
#endif
    if (stack->ptr != NULL) {
        free(find_alloc_address(stack->ptr));
    }
    stack->size = -1;
    stack->capacity = -1;
    stack->ptr = (T *)1;
}

void stack_push(struct TEMPLATE(stack, T) *stack, T elem) {
#if DEBUG_LEVEL >= DUMP_LEVEL
    if (!stack_ok(stack)) {
        stack_dump(stack);
        assert(!"OK (before stack_push())");
    }
#endif
    if (stack->size == stack->capacity) { //если меньше, реаллок не нужен, если больше стек не был валиден.
        reallocate(stack, (stack->capacity + 1) * 1.5);        
    }
    stack->ptr[stack->size++] = elem;
#if DEBUG_LEVEL >= HASH_LEVEL
    *((hash_t *)stack->ptr - 1) = calc_buf_hash(stack);
    stack->hash = calc_stack_hash(stack);
#endif
#if DEBUG_LEVEL >= DUMP_LEVEL
    if (!stack_ok(stack)) {
        stack_dump(stack);
        assert(!"OK (after stack_push())");
    }
#endif
}

T stack_pop(struct TEMPLATE(stack,T) *stack) {
#if DEBUG_LEVEL >= DUMP_LEVEL
    if (!stack_ok(stack)) {
        stack_dump(stack);
        assert(!"OK (before stack_pop())");
    }
#endif
    if (stack->size == 0) {
        log_printf("Trying to pop from empty stack.\n");
        assert(!"OK pop from empty stack");
        return 0;//плохое число, но мне лень делать полноценную систему ядов для всех типов. Может быть позже.
    }
    T elem = stack->ptr[--stack->size];
#if DEBUG_LEVEL >= HASH_LEVEL
    stack->hash = calc_stack_hash(stack);
#endif
#if DEBUG_LEVEL >= DUMP_LEVEL
    if (!stack_ok(stack)) {
        stack_dump(stack);
        assert(!"OK (after stack_pop())");
    }
#endif
    return elem;
}

int64_t stack_size(struct TEMPLATE(stack,T) *stack) {
#if DEBUG_LEVEL >= DUMP_LEVEL
    if (!stack_ok(stack)) {
        stack_dump(stack);
        assert(!"OK (before stack_size())");
    }
#endif
    return stack->size;
}

#endif
