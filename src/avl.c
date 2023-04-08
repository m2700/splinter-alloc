#include "avl.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "align.h"
#include "debug.h"

#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))

#define TREE_HEIGHT(node) ((node) == NULL ? 0 : (node)->height)
#define UPDATE_TREE_HEIGHT(node)                                                                   \
    ((node)->height = MAX(TREE_HEIGHT((node)->left), TREE_HEIGHT((node)->right)) + 1)

#define PARENT_CHILD_PTR(child)                                                                    \
    ((child)->parent->left == (child) ? &(child)->parent->left : ({                                \
        assert((child)->parent->right == child && "parent not connected to child");                \
        &(child)->parent->right;                                                                   \
    }))

inline static void list_insert_before(spla_avl_node *list_node, spla_avl_node *new_node) {
    assert(list_node != NULL);
    assert(new_node != NULL);
    assert(new_node->next == NULL);
    assert(new_node->prev == NULL);
    assert(new_node < list_node);
    assert(list_node->prev == NULL || list_node->prev < new_node);

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
    assert(list_node < new_node);
    assert(list_node->next == NULL || new_node < list_node->next);

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
    if (right_left != NULL) {
        right_left->parent = root;
    }

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
    if (left_right != NULL) {
        left_right->parent = root;
    }

    left->right = root;
    root->parent = left;

    UPDATE_TREE_HEIGHT(root);
    UPDATE_TREE_HEIGHT(left);

    *node = left;
}

inline static void rebalance(spla_avl_node **node) {
    assert(node != NULL);
    assert(*node != NULL);
    assert((*node)->parent == NULL || PARENT_CHILD_PTR(*node) == node);

    spla_avl_node **left = &(*node)->left;
    spla_avl_node **right = &(*node)->right;

    size_t left_height = TREE_HEIGHT(*left);
    size_t right_height = TREE_HEIGHT(*right);

    if (left_height > right_height + 1) {
        assert(left_height - right_height == 2);

        spla_avl_node *left_left = (*left)->left;
        spla_avl_node *left_right = (*left)->right;

        size_t left_left_height = TREE_HEIGHT(left_left);
        size_t left_right_height = TREE_HEIGHT(left_right);

        if (left_left_height < left_right_height) {
            rotate_left(left);
        }
        rotate_right(node);

        spla_avl_check_tree_only(*node);
    } else if (right_height > left_height + 1) {
        assert(right_height - left_height == 2);

        spla_avl_node *right_left = (*right)->left;
        spla_avl_node *right_right = (*right)->right;

        size_t right_left_height = TREE_HEIGHT(right_left);
        size_t right_right_height = TREE_HEIGHT(right_right);

        if (right_right_height < right_left_height) {
            rotate_right(right);
        }
        rotate_left(node);

        spla_avl_check_tree_only(*node);
    } else {
        UPDATE_TREE_HEIGHT(*node);

        spla_avl_check_tree_only(*node);
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

    assert(parent == (*node_ptr)->parent);

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

    spla_avl_node **node_ptr = PARENT_CHILD_PTR(node);
    *node_ptr = node->right;
    if (*node_ptr != NULL) {
        (*node_ptr)->parent = parent;
    }

    rebalance_up(parent);
}

inline static void tree_no_right_node_remove(spla_avl_node *node) {
    assert(node != NULL);
    assert(node->right == NULL);

    spla_avl_node *parent = node->parent;
    assert(parent != NULL);

    spla_avl_node **node_ptr = PARENT_CHILD_PTR(node);
    *node_ptr = node->left;
    if (*node_ptr != NULL) {
        (*node_ptr)->parent = parent;
    }

    rebalance_up(parent);
}

/// root must still be in list
inline static spla_avl_node *tree_root_remove(spla_avl_node **root) {
    assert(root != NULL);
    assert(*root != NULL);
    assert((*root)->parent == NULL || PARENT_CHILD_PTR(*root) == root);

    spla_avl_node *node = *root;

    if (node->left == NULL && node->right == NULL) {
        *root = NULL;
    } else if (node->left == NULL) {
        *root = node->right;
        (*root)->parent = node->parent;

        spla_avl_check_tree_only(*root);
    } else if (node->right == NULL) {
        *root = node->left;
        (*root)->parent = node->parent;

        spla_avl_check_tree_only(*root);
    } else if (node->left->height >= node->right->height) {
        spla_avl_node *prev = node->prev;
        assert(prev != NULL);
        assert(prev->right == NULL);

        if (node->left == prev) {
            node->left = node->left->left;
            if (node->left != NULL) {
                node->left->parent = node;
            }
        } else {
            // remove child parent to stop rebalancing of root.
            node->left->parent = NULL;

            tree_no_right_node_remove(prev);
            // left might have changed

            assert(node == *root);

            if (node->left->parent != NULL) {
                // a rotation happened
                node->left = node->left->parent;
            }
            // restore child parent
            assert(node->left->parent == NULL);
            node->left->parent = node;
        }

        *root = prev;
        (*root)->parent = node->parent;
        (*root)->left = node->left;
        (*root)->right = node->right;
        if ((*root)->left != NULL) {
            (*root)->left->parent = *root;
        }
        (*root)->right->parent = *root;

        rebalance(root);

        spla_avl_check_tree_only(*root);
    } else {
        spla_avl_node *next = node->next;
        assert(next != NULL);
        assert(next->left == NULL);

        if (node->right == next) {
            node->right = node->right->right;
            if (node->right != NULL) {
                node->right->parent = node;
            }
        } else {
            // remove child parent to stop rebalancing of root.
            node->right->parent = NULL;

            tree_no_left_node_remove(next);
            // right might have changed

            assert(node == *root);

            if (node->right->parent != NULL) {
                // a rotation happened
                node->right = node->right->parent;
            }
            // restore child parent
            assert(node->right->parent == NULL);
            node->right->parent = node;
        }

        *root = next;
        (*root)->parent = node->parent;
        (*root)->left = node->left;
        (*root)->right = node->right;
        (*root)->left->parent = *root;
        if ((*root)->right != NULL) {
            (*root)->right->parent = *root;
        }

        rebalance(root);

        spla_avl_check_tree_only(*root);
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
    node->height = 1;

    if (*root == NULL) {
        *root = node;
        (*root)->height = 1;

        spla_avl_check(*root);
        return NULL;
    } else {
        assert((*root)->parent == NULL || PARENT_CHILD_PTR(*root) == root);

        bool compact_first = is_compact_first(node, blck_align);
        size_t blck_size = (size_t)1 << blck_align;

        assert(node != *root);
        if (node < *root) {
            if (compact_first && can_compact(node, *root, blck_size)) {
                DBG_FN2(block_compaction, ("0x%012lx..", (size_t)node),
                        ("0x%012lx..", (size_t)*root));
                list_remove(tree_root_remove(root));

                spla_avl_check(*root);
                return node;
            } else if ((*root)->left == NULL) {
                assert(TREE_HEIGHT((*root)->right) <= 1);

                (*root)->left = node;
                node->parent = *root;
                list_insert_before(*root, node);
                (*root)->height = 2;

                spla_avl_check(*root);
                return NULL;
            } else {
                size_t left_height = (*root)->left->height;
                spla_avl_node *compact_node = spla_avl_insert(&(*root)->left, node, blck_align);
                if (TREE_HEIGHT((*root)->left) != left_height) {
                    rebalance(root);
                }

                spla_avl_check(*root);
                return compact_node;
            }
        } else {
            if (!compact_first && can_compact(*root, node, blck_size)) {
                DBG_FN2(block_compaction, ("0x%012lx..", (size_t)*root),
                        ("0x%012lx..", (size_t)node));
                spla_avl_node *compact_node = tree_root_remove(root);
                list_remove(compact_node);

                spla_avl_check(*root);
                return compact_node;
            } else if ((*root)->right == NULL) {
                assert(TREE_HEIGHT((*root)->left) <= 1);

                (*root)->right = node;
                node->parent = *root;
                list_insert_after(*root, node);
                (*root)->height = 2;

                spla_avl_check(*root);
                return NULL;
            } else {
                size_t right_height = (*root)->right->height;
                spla_avl_node *compact_node = spla_avl_insert(&(*root)->right, node, blck_align);
                if (TREE_HEIGHT((*root)->right) != right_height) {
                    rebalance(root); // sets height
                }

                spla_avl_check(*root);
                return compact_node;
            }
        }
    }
}

spla_avl_node *spla_avl_remove_root(spla_avl_node **root) {
    assert(root != NULL);
    assert(*root != NULL);
    assert((*root)->parent == NULL || PARENT_CHILD_PTR(*root) == root);

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

    tree_no_left_node_remove(first);
    list_remove(first);
}
void spla_avl_remove_last(spla_avl_node *last) {
    assert(last != NULL);
    assert(last->right == NULL);

    tree_no_right_node_remove(last);
    list_remove(last);
}

#if SPLA_TESTING

static void dump(spla_avl_node *root) {
    if (root == NULL) {
        printf("NULL\n");
        return;
    }

    printf("AVL Block: 0x%012lx\n", (size_t)root);
    printf("  height=%lu\n", root->height);
    printf("  parent=0x%012lx\n", (size_t)root->parent);
    printf("  left=0x%012lx\n", (size_t)root->left);
    printf("  right=0x%012lx\n", (size_t)root->right);
    printf("  next=0x%012lx\n", (size_t)root->next);
    printf("  prev=0x%012lx\n", (size_t)root->prev);
}

#define assert_dump(expr)                                                                          \
    if (!(expr)) {                                                                                 \
        dump(root);                                                                                \
        assert(0 && #expr);                                                                        \
    }

static void node_parent_check(spla_avl_node *root) {
    assert_dump(root == NULL || root->parent == NULL || root->parent->left == root
                || root->parent->right == root);
}

static void node_tree_only_constrained_check(spla_avl_node *root, spla_avl_node *limit_low,
                                             spla_avl_node *limit_high) {
    if (root == NULL) {
        return;
    }

    assert_dump(root->height == 1 + MAX(TREE_HEIGHT(root->left), TREE_HEIGHT(root->right)));
    assert_dump(MAX(TREE_HEIGHT(root->left), TREE_HEIGHT(root->right))
                    - MIN(TREE_HEIGHT(root->left), TREE_HEIGHT(root->right))
                <= 1);

    assert_dump(root->left == NULL || root->left->parent == root);
    assert_dump(root->right == NULL || root->right->parent == root);

    assert_dump(limit_low == NULL || root > limit_low);
    assert_dump(limit_high == NULL || root < limit_high);
}

static void spla_avl_constrained_check_tree_only(spla_avl_node *root, spla_avl_node *limit_low,
                                                 spla_avl_node *limit_high) {
    if (root == NULL) {
        return;
    }

    node_tree_only_constrained_check(root, limit_low, limit_high);

    spla_avl_constrained_check_tree_only(root->left, limit_low, root);
    spla_avl_constrained_check_tree_only(root->right, root, limit_high);
}

void spla_avl_check_tree_only(spla_avl_node *root) {
    node_parent_check(root);
    spla_avl_constrained_check_tree_only(root, NULL, NULL);
}

static void node_list_only_check(spla_avl_node *root) {
    if (root == NULL) {
        return;
    }

    assert_dump(root->next == NULL || root->next->prev == root);
    assert_dump(root->prev == NULL || root->prev->next == root);

    assert_dump(root->prev == NULL || root->prev < root);
    assert_dump(root->next == NULL || root->next > root);
}

static void go_prev_list_only_check(spla_avl_node *root) {
    while (root != NULL) {
        node_list_only_check(root);
        root = root->prev;
    }
}
static void go_next_list_only_check(spla_avl_node *root) {
    while (root != NULL) {
        node_list_only_check(root);
        root = root->next;
    }
}

void spla_avl_check_list_only(spla_avl_node *root) {
    if (root == NULL) {
        return;
    }

    node_list_only_check(root);

    go_prev_list_only_check(root->prev);
    go_next_list_only_check(root->next);
}

static void spla_avl_constrained_check(spla_avl_node *root, spla_avl_node *limit_low,
                                       spla_avl_node *limit_high) {
    if (root == NULL) {
        return;
    }

    assert_dump((root->prev != NULL) || (root->left == NULL));
    assert_dump((root->next != NULL) || (root->right == NULL));

    assert_dump(root->left == NULL || root->left <= root->prev);
    assert_dump(root->right == NULL || root->right >= root->next);

    node_tree_only_constrained_check(root, limit_low, limit_high);
    node_list_only_check(root);

    spla_avl_constrained_check(root->left, limit_low, root);
    spla_avl_constrained_check(root->right, root, limit_high);
}

#undef assert_dump

void spla_avl_check(spla_avl_node *root) {
    node_parent_check(root);
    spla_avl_constrained_check(root, NULL, NULL);
}

#endif // SPLA_TESTING