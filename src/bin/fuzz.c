#include <assert.h>
#include <errno.h>
#include <splinter-alloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <time.h>

#define SWAP(x, y)                                                                                 \
    ({                                                                                             \
        typeof(x) tmp = x;                                                                         \
        x = y;                                                                                     \
        y = tmp;                                                                                   \
    })

static inline unsigned randint(unsigned min, unsigned max) {
    assert(max >= min);
    if (min == max) {
        return min;
    }
    unsigned r;
    do {
        r = random();
    } while (r >= ~0u - (~0u % (max - min)));
    r %= max - min;
    return r + min;
}

static inline void random_split(size_t amount, size_t split_size, size_t split_array[]) {
    assert(amount >= split_size);
    for (size_t i = 0; i < split_size; i++) {
        split_array[i] = 1;
        amount--;
    }
    for (size_t i = 0, step = randint(1, MIN(1 + (amount / (split_size * split_size)), amount - i));
         i < amount; i += step)
    {
        split_array[randint(0, split_size - 1)] += step;
    }
}

#define PAGE_PTR_CACHE_CAP 8
typedef struct page_allocator {
    size_t free_size;
    size_t ptrs_len;
    void *ptrs[PAGE_PTR_CACHE_CAP];
} page_allocator;

static inline void page_allocator_init(page_allocator *palloc, size_t size) {
    assert(size % (1ul << 12) == 0);
    palloc->free_size = size;
    palloc->ptrs_len = 0;
}
static inline page_allocator *page_allocator_new(size_t size) {
    page_allocator *palloc = malloc(sizeof(page_allocator));
    page_allocator_init(palloc, size);
    return palloc;
}
static inline void page_allocator_free(page_allocator *palloc) {
    for (size_t i = 0; i < palloc->ptrs_len; i++) {
        free(palloc->ptrs[i]);
    }
    free(palloc);
}

static inline void page_allocator_push_ptr(page_allocator *palloc, void *ptr) {
    assert(palloc->ptrs_len < PAGE_PTR_CACHE_CAP);
    palloc->ptrs[palloc->ptrs_len] = ptr;
    palloc->ptrs_len++;
}
static inline void *page_allocator_remove_ptr_idx(page_allocator *palloc, size_t idx) {
    assert(idx < palloc->ptrs_len);
    palloc->ptrs_len--;
    void *ptr = palloc->ptrs[idx];
    palloc->ptrs[idx] = palloc->ptrs[palloc->ptrs_len];
    return ptr;
}
static inline char page_allocator_remove_ptr(page_allocator *palloc, void *ptr) {
    for (size_t i = 0; i < palloc->ptrs_len; i++) {
        if (palloc->ptrs[i] == ptr) {
            page_allocator_remove_ptr_idx(palloc, i);
            return 1;
        }
    }
    return 0;
}

typedef struct range {
    size_t start; // inclusive
    size_t end;   // exclusive
} range;

typedef struct range_list range_list;
struct range_list {
    range item;
    range_list *next;
};

static inline void range_list_push_front_elem(range_list **list, range_list *elem) {
    assert(elem->item.start < elem->item.end);
    elem->next = *list;
    *list = elem;
}
static inline void range_list_push_front(range_list **list, range item) {
    range_list *elem = malloc(sizeof(range_list));
    elem->item = item;
    range_list_push_front_elem(list, elem);
}

static inline range_list *range_list_pop_front_elem(range_list **list) {
    assert(*list != NULL);
    range_list *elem = *list;
    *list = (*list)->next;
    return elem;
}
static inline range range_list_pop_front(range_list **list) {
    range_list *elem = range_list_pop_front_elem(list);
    range r = elem->item;
    free(elem);
    return r;
}

static void range_list_clear(range_list **list) {
    while (*list != NULL) {
        range_list_pop_front(list);
    }
}
static void range_list_insert_distinct_sorted(range_list **list, range item) {
    assert(item.start < item.end);

    while (*list != NULL && item.start >= (*list)->item.end) {
        list = &(*list)->next;
    }
    assert((*list == NULL || item.end <= (*list)->item.start) && "overlapping ranges");

    range_list_push_front(list, item);
}
static void range_list_shuffle(range_list **list) {
    const size_t tmp_list_count = 16;
    range_list *tmp_lists[tmp_list_count];
    for (size_t li = 0; li < tmp_list_count; li++) {
        tmp_lists[li] = NULL;
    }

    for (size_t i = 0; i < 8; i++) {
        while (*list != NULL) {
            range_list_push_front_elem(&tmp_lists[randint(0, tmp_list_count - 1)],
                                       range_list_pop_front_elem(list));
        }
        size_t non_empty_tmp_l_count = tmp_list_count;
        while (non_empty_tmp_l_count > 0) {
            size_t tmp_ri = randint(0, non_empty_tmp_l_count - 1);
            if (tmp_lists[tmp_ri] == NULL) {
                if (tmp_ri < non_empty_tmp_l_count - 1) {
                    SWAP(tmp_lists[tmp_ri], tmp_lists[non_empty_tmp_l_count - 1]);
                }
                non_empty_tmp_l_count--;
            } else {
                range_list_push_front_elem(list, range_list_pop_front_elem(&tmp_lists[tmp_ri]));
            }
        }
    }
}

static void single_alloc_test(splinter_alloc *spla_alloc, page_allocator *palloc) {
    printf("single_alloc_test:\n");
    size_t block_size = randint(1, palloc->free_size - 8);
    void *ptr = spla_malloc(spla_alloc, block_size);
    assert(ptr != NULL && "out of memory has been reached");
}

static void use_half_test(splinter_alloc *spla_alloc, page_allocator *palloc,
                          unsigned block_count) {
    printf("use_half_test: block_count: %u\n", block_count);
    size_t split_array[block_count];

    size_t alloc_amount = (palloc->free_size) / 2;
    assert(alloc_amount > block_count * 8);
    alloc_amount -= block_count * 8; // metadata

    random_split(alloc_amount, block_count, split_array);
    for (size_t bli = 0; bli < block_count; bli++) {
        size_t block_size = split_array[bli];
        void *ptr = spla_malloc(spla_alloc, block_size);
        assert(ptr != NULL && "out of memory has been reached");
    }
}

static void out_of_mem_test(splinter_alloc *spla_alloc, page_allocator *palloc,
                            unsigned block_count) {
    printf("out_of_mem_test: block_count: %u\n", block_count);
    size_t split_array[block_count];
    size_t alloc_amount = palloc->free_size + (1ul << 12);
    random_split(alloc_amount, block_count, split_array);
    for (size_t bli = 0; bli < block_count; bli++) {
        size_t block_size = split_array[bli];
        void *ptr = spla_malloc(spla_alloc, block_size);
        if (ptr == NULL) {
            return;
        }
    }
    assert(0 && "out of memory has not been reached");
}

static void no_overlap_test(splinter_alloc *spla_alloc, page_allocator *palloc,
                            unsigned block_count) {
    printf("no_overlap_test: block_count: %u\n", block_count);
    range_list *list = NULL;
    size_t split_array[block_count];
    size_t alloc_amount = palloc->free_size + (1ul << 12);
    random_split(alloc_amount, block_count, split_array);
    for (size_t bli = 0; bli < block_count; bli++) {
        size_t block_size = split_array[bli];
        void *ptr = spla_malloc(spla_alloc, block_size);
        if (ptr != NULL) {
            range r = {.start = (size_t)ptr, .end = (size_t)ptr + block_size};
            range_list_insert_distinct_sorted(&list, r); // checks for overlaps
        }
    }
    range_list_clear(&list);
}

static void reclaim_test(splinter_alloc *spla_alloc, page_allocator *palloc, unsigned block_count) {
    printf("reclaim_test: block_count: %u\n", block_count);
    size_t initial_free_size = palloc->free_size;
    range_list *list = NULL;
    size_t split_array[block_count];
    size_t alloc_amount = randint(block_count, palloc->free_size + (1ul << 12));
    random_split(alloc_amount, block_count, split_array);
    for (size_t bli = 0; bli < block_count; bli++) {
        size_t block_size = split_array[bli];
        void *ptr = spla_malloc(spla_alloc, block_size);
        if (ptr != NULL) {
            range r = {.start = (size_t)ptr, .end = (size_t)ptr + block_size};
            range_list_push_front(&list, r);
        }
    }

    range_list_shuffle(&list);
    while (list != NULL) {
        range r = range_list_pop_front(&list);
        spla_free(spla_alloc, (void *)r.start);
    }

    void *ptr = spla_malloc(spla_alloc, initial_free_size - 8);
    if (ptr == NULL) {
        printf("Warning: reclaim_test: Allocating all space did not work as expected. "
               "This can happen when free-list sorting is not set to infinity.\n");
    }

    range_list_clear(&list);
}

void *palloc(void *pa, size_t num_pages) {
    assert(num_pages >= 1);

#if SPLA_DEBUG
    printf("palloc:\n  num_pages:\t%lu\n", num_pages);
#endif

    page_allocator *pallocator = pa;

    if (num_pages * (1ul << 12) > pallocator->free_size) {
#if SPLA_DEBUG
        printf("  free_size:\t0x%lx\n", pallocator->free_size);
        printf("  req-size:\t0x%lx\n", num_pages * (1ul << 12));
#endif
        return NULL;
    }
    void *ptr;
    int res = posix_memalign(&ptr, 1ul << 12, num_pages * (1ul << 12));
    assert(res == 0);
    page_allocator_push_ptr(pallocator, ptr);
    pallocator->free_size -= num_pages * (1ul << 12);
    return ptr;
}

void pfree(void *pa, void *ptr, size_t num_pages) {
#if SPLA_DEBUG
    printf("pfree:\n  ptr:\t0x%012lx\n  num_pages:\t%lu\n", (size_t)ptr, num_pages);
#endif
    page_allocator *pallocator = pa;
    pallocator->free_size += num_pages * (1ul << 12);
}

#define TEST_ITERATIONS 1000
#define spla_clear(pallocator, spla_alloc, palloc_size)                                            \
    {                                                                                              \
        page_allocator_free(pallocator);                                                           \
        pallocator = page_allocator_new(palloc_size);                                              \
        spla_alloc = spla_init_alloc(pallocator, 0, palloc, pfree);                                \
    }

int main(int argc, char const *argv[]) {
    // random seed
    srandom(time(NULL));

    for (size_t test_i = 0; test_i < TEST_ITERATIONS; test_i++) {
        size_t palloc_page_num = randint(2, PAGE_PTR_CACHE_CAP);
        size_t palloc_size = palloc_page_num << 12;
        page_allocator *pallocator = page_allocator_new(palloc_size);
        splinter_alloc *spla_alloc = spla_init_alloc(pallocator, 0, palloc, pfree);

        // unsigned block_count = randint(1, pallocator->free_size / 24);
        unsigned block_count_half = randint(1, pallocator->free_size / 24 / 2);
        unsigned block_count_p1 = randint(1, pallocator->free_size + (1ul << 12) / 24);

        single_alloc_test(spla_alloc, pallocator);
        spla_clear(pallocator, spla_alloc, palloc_size);

        use_half_test(spla_alloc, pallocator, block_count_half);
        spla_clear(pallocator, spla_alloc, palloc_size);

        out_of_mem_test(spla_alloc, pallocator, block_count_p1);
        spla_clear(pallocator, spla_alloc, palloc_size);

        no_overlap_test(spla_alloc, pallocator, block_count_p1);
        spla_clear(pallocator, spla_alloc, palloc_size);

        reclaim_test(spla_alloc, pallocator, block_count_p1);
        spla_clear(pallocator, spla_alloc, palloc_size);
    }

    return 0;
}
