#pragma once

#include <assert.h>
#include <stddef.h>

typedef struct spla_avl_node spla_avl_node;
struct spla_avl_node {
    spla_avl_node *left;
    spla_avl_node *right;
    spla_avl_node *next;
    spla_avl_node *prev;
    spla_avl_node *parent;
    size_t height;
};

spla_avl_node *spla_avl_insert(spla_avl_node **root, spla_avl_node *node, unsigned blck_align);
void spla_avl_remove_leaf(spla_avl_node *leaf);
spla_avl_node *spla_avl_remove_root(spla_avl_node **root);
void spla_avl_remove_first(spla_avl_node *first);
void spla_avl_remove_last(spla_avl_node *last);

typedef struct spla_avl_tree spla_avl_tree;
struct spla_avl_tree {
    spla_avl_node root;
    spla_avl_node *first;
};

static inline spla_avl_node *spla_avl_tree_insert(spla_avl_tree **tree, spla_avl_node *node,
                                                  unsigned blck_align) {
    assert(tree != NULL);
    assert(node != NULL);

    spla_avl_node *first = *tree == NULL ? NULL : (*tree)->first;

    spla_avl_node *compacted = spla_avl_insert((spla_avl_node **)tree, node, blck_align);

    if ((*tree)->root.parent != NULL) {
        *tree = (spla_avl_tree *)(*tree)->root.parent;
    }
    assert((*tree)->root.parent == NULL);

    if (first == NULL) {
        (*tree)->first = &(*tree)->root;
    } else if (first->prev != NULL) {
        (*tree)->first = first->prev;
    }
    assert((*tree)->first->prev == NULL);

    return compacted;
}

static inline spla_avl_node *spla_avl_tree_pop(spla_avl_tree **tree) {
    assert(tree != NULL);

    if (*tree == NULL) {
        return NULL;
    }

    spla_avl_node *first = (*tree)->first;
    assert(first != NULL);

    spla_avl_node *next_first = first->next;

    if (&(*tree)->root == first) {
        spla_avl_remove_root((spla_avl_node **)tree);

    } else {
        spla_avl_remove_first(first);

        if ((*tree)->root.parent != NULL) {
            *tree = (spla_avl_tree *)(*tree)->root.parent;
        }
        assert((*tree)->root.parent == NULL);
    }

    if (*tree != NULL) {
        (*tree)->first = next_first;
    }
    return first;
}