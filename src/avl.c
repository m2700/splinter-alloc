#include "avl.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "align.h"
#include "debug.h"

#define MAX(a, b) ((a) >= (b) ? (a) : (b))

#define TREE_HEIGHT(node) ((node) == NULL ? 0 : (node)->height)
#define UPDATE_TREE_HEIGHT(node)                                                                   \
    ((node)->height = MAX(TREE_HEIGHT((node)->left), TREE_HEIGHT((node)->right)) + 1)

#define PARENT_CHILD_PTR(child)                                                                    \
    ((child)->parent->left == (child) ? &(child)->parent->left : ({                                \
        assert((child)->parent->right == child);                                                   \
        &(child)->parent->right;                                                                   \
    }))

inline static void list_insert_before(spla_avl_node *list_node, spla_avl_node *new_node) {
    assert(list_node != NULL);
    assert(new_node != NULL);
    assert(new_node->next == NULL);
    assert(new_node->prev == NULL);

    new_node->next = list_node;
    new_node->prev = list_node->prev;
    list_node->prev = new_node;
    if (new_node->prev != NULL) {
        new_node->prev->next = new_node;
    }
}

inline static void list_insert_after(spla_avl_node *list_node, spla_avl_node *new_node) {
    assert(list_node != NULL);
    assert(new_node != NULL);
    assert(new_node->next == NULL);
    assert(new_node->prev == NULL);

    new_node->prev = list_node;
    new_node->next = list_node->next;
    list_node->next = new_node;
    if (new_node->next != NULL) {
        new_node->next->prev = new_node;
    }
}

inline static void list_remove(spla_avl_node *list_node) {
    assert(list_node != NULL);

    if (list_node->prev != NULL) {
        list_node->prev->next = list_node->next;
    }
    if (list_node->next != NULL) {
        list_node->next->prev = list_node->prev;
    }
    list_node->next = NULL;
    list_node->prev = NULL;
}

inline static bool is_compact_first(spla_avl_node *node, unsigned blck_align) {
    return MAX_ALIGN_OF(node) > blck_align;
}

inline static bool can_compact(spla_avl_node *left_node, spla_avl_node *right_node,
                               size_t blck_size) {
    assert(is_compact_first(left_node, MAX_ALIGN_OF(blck_size))
           || !is_compact_first(right_node, MAX_ALIGN_OF(blck_size)));
    return (char *)left_node + blck_size == (char *)right_node;
}

inline static void rotate_left(spla_avl_node **node) {
    assert(node != NULL);
    assert(*node != NULL);
    assert((*node)->right != NULL);

    spla_avl_node *root = *node;
    spla_avl_node *right = root->right;
    spla_avl_node *right_left = right->left;

    right->parent = root->parent;

    root->right = right_left;
    right_left->parent = root;

    right->left = root;
    root->parent = right;

    UPDATE_TREE_HEIGHT(root);
    UPDATE_TREE_HEIGHT(right);

    *node = right;
}

inline static void rotate_right(spla_avl_node **node) {
    assert(node != NULL);
    assert(*node != NULL);
    assert((*node)->left != NULL);

    spla_avl_node *root = *node;
    spla_avl_node *left = root->left;
    spla_avl_node *left_right = left->right;

    left->parent = root->parent;

    root->left = left_right;
    left_right->parent = root;

    left->right = root;
    root->parent = left;

    UPDATE_TREE_HEIGHT(root);
    UPDATE_TREE_HEIGHT(left);

    *node = left;
}

inline static void rebalance(spla_avl_node **node) {
    assert(node != NULL);

    spla_avl_node **left = &(*node)->left;
    spla_avl_node **right = &(*node)->right;

    size_t left_height = TREE_HEIGHT(*left);
    size_t right_height = TREE_HEIGHT(*right);

    if (left_height > right_height + 1) {
        spla_avl_node *left_left = (*left)->left;
        spla_avl_node *left_right = (*left)->right;

        size_t left_left_height = TREE_HEIGHT(left_left);
        size_t left_right_height = TREE_HEIGHT(left_right);

        if (left_left_height < left_right_height) {
            rotate_left(left);
        }
        rotate_right(node);
    } else if (right_height > left_height + 1) {
        spla_avl_node *right_left = (*right)->left;
        spla_avl_node *right_right = (*right)->right;

        size_t right_left_height = TREE_HEIGHT(right_left);
        size_t right_right_height = TREE_HEIGHT(right_right);

        if (right_right_height < right_left_height) {
            rotate_right(right);
        }
        rotate_left(node);
    } else {
        UPDATE_TREE_HEIGHT(*node);
    }
}

static inline void rebalance_up(spla_avl_node *node) {
    assert(node != NULL);

    size_t height = node->height;
    spla_avl_node *parent = node->parent;

    spla_avl_node **node_ptr;
    if (parent != NULL) {
        node_ptr = PARENT_CHILD_PTR(node);
    } else {
        node_ptr = &node; // change still visible due to added parent in node
    }
    rebalance(node_ptr);

    if (parent != NULL && height != (*node_ptr)->height) {
        rebalance_up(parent);
    }
}

inline static void tree_leaf_remove(spla_avl_node *leaf) {
    assert(leaf != NULL);
    assert(leaf->left == NULL);
    assert(leaf->right == NULL);
    assert(leaf->height == 1);

    spla_avl_node *parent = leaf->parent;
    assert(parent != NULL);

    *PARENT_CHILD_PTR(leaf) = NULL;

    rebalance_up(parent);
}

inline static void tree_no_left_node_remove(spla_avl_node *node) {
    assert(node != NULL);
    assert(node->left == NULL);

    spla_avl_node *parent = node->parent;
    assert(parent != NULL);

    node->right->parent = parent;
    *PARENT_CHILD_PTR(node) = node->right;

    rebalance_up(parent);
}

inline static void tree_no_right_node_remove(spla_avl_node *node) {
    assert(node != NULL);
    assert(node->right == NULL);

    spla_avl_node *parent = node->parent;
    assert(parent != NULL);

    node->left->parent = parent;
    *PARENT_CHILD_PTR(node) = node->left;

    rebalance_up(parent);
}

/// root must still be in list
inline static spla_avl_node *tree_root_remove(spla_avl_node **root) {
    assert(root != NULL);
    assert(*root != NULL);

    spla_avl_node *node = *root;
    spla_avl_node *left = node->left;
    spla_avl_node *right = node->right;

    if (left == NULL && right == NULL) {
        *root = NULL;
    } else if (left == NULL) {
        *root = right;
        right->parent = node->parent;
    } else if (right == NULL) {
        *root = left;
        left->parent = node->parent;
    } else if (left->height >= right->height) {
        spla_avl_node *prev = node->prev;
        assert(prev != NULL);

        tree_no_right_node_remove(prev);
        *root = prev;
        prev->parent = node->parent;
        prev->left = left;
        prev->right = right;
        left->parent = prev;
        right->parent = prev;

        rebalance(root);
    } else {
        spla_avl_node *next = node->next;
        assert(next != NULL);

        tree_no_left_node_remove(next);
        *root = next;
        next->parent = node->parent;
        next->left = left;
        next->right = right;
        left->parent = next;
        right->parent = next;

        rebalance(root);
    }

    return node;
}

spla_avl_node *spla_avl_insert(spla_avl_node **root, spla_avl_node *node, unsigned blck_align) {
    assert(root != NULL);
    assert(node != NULL);

    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    node->prev = NULL;
    node->parent = NULL;
    node->height = 0;

    if (*root == NULL) {
        *root = node;
        (*root)->height = 1;
        return NULL;
    } else {
        bool compact_first = is_compact_first(node, blck_align);
        size_t blck_size = (size_t)1 << blck_align;

        assert(node != *root);
        if (node < *root) {
            if (compact_first && can_compact(node, *root, blck_size)) {
                DBG_FN2(block_compaction, ("0x%012lx..", (size_t)node),
                        ("0x%012lx..", (size_t)*root));
                list_remove(tree_root_remove(root));
                return node;
            } else if ((*root)->left == NULL) {
                assert(TREE_HEIGHT((*root)->right) <= 1);

                (*root)->left = node;
                node->parent = *root;
                list_insert_before(*root, node);
                (*root)->height = 2;
                return NULL;
            } else {
                size_t left_height = (*root)->left->height;
                spla_avl_node *compact_node = spla_avl_insert(&(*root)->left, node, blck_align);
                if (TREE_HEIGHT((*root)->left) != left_height) {
                    rebalance(root);
                }
                return compact_node;
            }
        } else {
            if (!compact_first && can_compact(*root, node, blck_size)) {
                DBG_FN2(block_compaction, ("0x%012lx..", (size_t)*root),
                        ("0x%012lx..", (size_t)node));
                spla_avl_node *compact_node = tree_root_remove(root);
                list_remove(compact_node);
                return compact_node;
            } else if ((*root)->right == NULL) {
                assert(TREE_HEIGHT((*root)->left) <= 1);

                (*root)->right = node;
                node->parent = *root;
                list_insert_after(*root, node);
                (*root)->height = 2;
                return NULL;
            } else {
                size_t right_height = (*root)->right->height;
                spla_avl_node *compact_node = spla_avl_insert(&(*root)->right, node, blck_align);
                if (TREE_HEIGHT((*root)->right) != right_height) {
                    rebalance(root); // sets height
                }
                return compact_node;
            }
        }
    }
}

spla_avl_node *spla_avl_remove_root(spla_avl_node **root) {
    assert(root != NULL);
    assert(*root != NULL);

    spla_avl_node *removed_node = tree_root_remove(root);
    list_remove(removed_node);

    return removed_node;
}

void spla_avl_remove_leaf(spla_avl_node *leaf) {
    assert(leaf != NULL);
    assert(leaf->left == NULL);
    assert(leaf->right == NULL);
    assert(leaf->height == 1);

    tree_leaf_remove(leaf);
    list_remove(leaf);
}
void spla_avl_remove_first(spla_avl_node *first) {
    assert(first != NULL);
    assert(first->left == NULL);

    tree_no_right_node_remove(first);
    list_remove(first);
}
void spla_avl_remove_last(spla_avl_node *last) {
    assert(last != NULL);
    assert(last->right == NULL);

    tree_no_left_node_remove(last);
    list_remove(last);
}