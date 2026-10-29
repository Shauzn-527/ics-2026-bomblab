#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

static uint32_t rotl32(uint32_t x, unsigned n) {
    return (x << n) | (x >> (32u - n));
}

static uint32_t rotr32(uint32_t x, unsigned n) {
    return (x >> n) | (x << (32u - n));
}

/* ---- Phase 1: 函数调用链 (p1_compute) ---- */
static uint32_t p1_step2(uint32_t a, uint32_t b) {
    return (a + 8u * b) ^ UINT32_C(0x46f4);
}

static uint32_t p1_step0(uint32_t a, uint32_t b) {
    return (b ^ UINT32_C(0x9049)) + 2u * a;
}

static uint32_t p1_step1(uint32_t a, uint32_t b) {
    return (3u * a) ^ (b + UINT32_C(0x7460));
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t low = (uint32_t)challenge;
    uint32_t high = (uint32_t)(challenge >> 32);
    uint32_t mid = (uint32_t)(challenge >> 16);
    uint32_t step2 = p1_step2(high, low);
    uint32_t step0 = p1_step0(low, mid);
    values[0] = p1_step1(step0, step2);
    return write_answer(out, cap, values, 1);
}

/* ---- Phase 2: 数组与递推 (phase_02) ---- */
int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    values[0] = 3u * ((uint32_t)(challenge >> 30) & UINT32_C(0xff));
    for (unsigned i = 1; i < 6; ++i) {
        uint32_t term = ((uint32_t)(challenge >> (8 + 5 * i)) & UINT32_C(0x3f)) + i;
        values[i] = 5u * (term ^ values[i - 1]);
    }
    return write_answer(out, cap, values, 6);
}

/* ---- Phase 3: switch 跳转表 (p3_dispatch) ---- */
static uint32_t p3_dispatch(uint32_t index, uint32_t value) {
    switch (index) {
        case 0: return 5u * value + UINT32_C(0x2491);            /* case_1 */
        case 1: return 5u * value + UINT32_C(0x4fee);            /* case_4 */
        case 2: return 5u * value + UINT32_C(0x14fdc);           /* case_5 */
        case 3: return 5u * value + UINT32_C(0x32f4b);           /* case_2 */
        case 4: return (2u * value) ^ UINT32_C(0x4710);          /* case_0 */
        case 5: return (value ^ UINT32_C(0xfbd5)) + (value >> 2);/* case_3 */
        case 6: return (value ^ UINT32_C(0x11f)) + (value >> 2); /* case_7 */
        case 7: return (2u * value) ^ UINT32_C(0xe630);          /* case_6 */
        default: return 0;
    }
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t index = (uint32_t)challenge & 7u;
    uint32_t value = (uint32_t)(challenge >> 3) ^ (uint32_t)(challenge >> 32);
    values[0] = index;
    values[1] = p3_dispatch(index, value);
    return write_answer(out, cap, values, 2);
}

/* ---- Phase 4: 递归二分查找 (p4_walk) ---- */
static uint32_t p4_walk(uint32_t target, uint32_t low, uint32_t high, uint32_t salt) {
    uint32_t middle = low + ((high - low) >> 1);
    if (target == middle) return (middle + UINT32_C(0x6e)) ^ salt;
    if (target < middle)
        return ((5u * p4_walk(target, low, middle - 1u, salt)) ^ (middle + salt)) ^ UINT32_C(0x0a);
    return ((5u * p4_walk(target, middle + 1u, high, salt)) ^ (middle + salt)) ^ UINT32_C(0x11);
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t target = (uint32_t)challenge & UINT32_C(0x7f);
    uint32_t salt = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    values[0] = p4_walk(target, 0u, UINT32_C(0x7f), salt);
    return write_answer(out, cap, values, 1);
}

/* ---- Phase 5: 链表遍历 (p5_traverse) ---- */
struct p5_node {
    int next;     /* -1 表示 NULL */
    uint32_t tag;
    uint32_t value;
};

static const struct p5_node p5_nodes[] = {
    /*  0 */ { 1,  UINT32_C(0x34e), UINT32_C(0xe64d) },
    /*  1 */ { 9,  UINT32_C(0x03d), UINT32_C(0xa96d) },
    /*  2 */ { 8,  UINT32_C(0x62e), UINT32_C(0x666b) },
    /*  3 */ { 10, UINT32_C(0x56a), UINT32_C(0x1146) },
    /*  4 */ { 7,  UINT32_C(0x451), UINT32_C(0xc651) },
    /*  5 */ { 11, UINT32_C(0x726), UINT32_C(0x7be4) },
    /*  6 */ { 4,  UINT32_C(0x954), UINT32_C(0x212a) },
    /*  7 */ { 3,  UINT32_C(0x86d), UINT32_C(0xdbe7) },
    /*  8 */ { 0,  UINT32_C(0x20b), UINT32_C(0x19b6) },
    /*  9 */ { 5,  UINT32_C(0xa15), UINT32_C(0x6521) },
    /* 10 */ { 2,  UINT32_C(0x128), UINT32_C(0xc7ce) },
    /* 11 */ { -1, UINT32_C(0xb46), UINT32_C(0x2d05) },
};

static const int p5_index[8] = {6, 4, 7, 3, 10, 2, 8, 0};

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    int node = p5_index[(uint32_t)challenge & 7u];
    uint32_t fold = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t out_tag = p5_nodes[node].tag;
    fold ^= p5_nodes[node].value;
    fold ^= UINT32_C(0x91c6);
    fold += out_tag;
    uint32_t steps = (uint32_t)(challenge >> 3) & 3u;
    for (uint32_t i = 0; i < steps; ++i) {
        node = p5_nodes[node].next;
        fold ^= p5_nodes[node].value;
        fold += p5_nodes[node].tag;
        out_tag = p5_nodes[node].tag;
    }
    values[0] = fold;
    values[1] = out_tag;
    return write_answer(out, cap, values, 2);
}

/* ---- Phase 6: 函数指针表间接调用 (p6_dispatch) ---- */
static uint32_t p6_op(uint32_t index, uint32_t value) {
    switch (index) {
        case 0: return rotr32(value, 5);                                /* op_0 */
        case 1: return (value ^ UINT32_C(0xf5c9)) + 8u * value;         /* op_1 */
        case 2: return rotl32(value, 5) ^ UINT32_C(0x4b58);             /* op_2 */
        case 3: return (value + UINT32_C(0x4dc3)) ^ (value >> 3);       /* op_3 */
        case 4: return (8u * value + UINT32_C(0xc255)) ^ (7u * value);  /* op_4 */
        case 5: return (value ^ (value >> 7)) + UINT32_C(0x14e5);       /* op_5 */
        default: return 0;
    }
}

static uint32_t p6_op_table(uint32_t index, uint32_t value) {
    static const uint8_t table[8] = {5, 4, 1, 5, 3, 0, 2, 4};
    return p6_op(table[index & 7u], value);
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t value = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t first = p6_op_table((uint32_t)challenge & 7u, value) ^ UINT32_C(0xf4bd);
    uint32_t second = p6_op_table(((uint32_t)challenge >> 3) & 7u, first);
    values[0] = second + UINT32_C(0x3fd7);
    return write_answer(out, cap, values, 1);
}

/* ---- Secret (Honor): secret_gate + secret_phase ---- */
static uint32_t secret_mix(uint32_t value) {
    return (rotl32(value, 3) + UINT32_C(0x9a30)) ^ UINT32_C(0xebf3);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t chal_lo = (uint32_t)challenge;
    uint32_t chal_hi = (uint32_t)(challenge >> 32);
    uint32_t ctx_lo = (uint32_t)context;
    uint32_t ctx_hi = (uint32_t)(context >> 32);
    uint32_t ctx16 = (uint32_t)(context >> 16);

    uint32_t k = ctx_lo & 3u;
    uint32_t x = chal_lo ^ chal_hi;
    uint32_t y = ctx_lo ^ ctx_hi;

    uint32_t a = k;
    uint32_t b = secret_mix(x ^ y) ^ ((k * UINT32_C(0x9a30)) ^ ctx16);
    uint32_t c = b ^ ((y + k + UINT32_C(0xebf3)) + (x >> 3));

    /* secret_gate 要求第 32..35 字节为 "OPEN"，其后跟三个无符号整数。 */
    int size = snprintf(out, cap,
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOPEN %" PRIu32 " %" PRIu32 " %" PRIu32,
                        a, b, c);
    return size < 0 || (size_t)size >= cap;
}
