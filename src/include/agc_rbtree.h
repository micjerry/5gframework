#ifndef AGC_RBTREE_H
#define AGC_RBTREE_H

/*
nginx rbtree
*/

typedef uint32_t  agc_rbtree_key_t;
typedef int   agc_rbtree_key_int_t;

typedef struct agc_rbtree_node_s  agc_rbtree_node_t;

struct agc_rbtree_node_s {
    agc_rbtree_key_t       key;
    agc_rbtree_node_t     *left;
    agc_rbtree_node_t     *right;
    agc_rbtree_node_t     *parent;
    char                 color;
    char                 data;
};

typedef struct agc_rbtree_s  agc_rbtree_t;

typedef void (*agc_rbtree_insert_func) (agc_rbtree_node_t *root,
    agc_rbtree_node_t *node, agc_rbtree_node_t *sentinel);

struct agc_rbtree_s {
    agc_rbtree_node_t     *root;
    agc_rbtree_node_t     *sentinel;
    agc_rbtree_insert_func   insert;
};

#define agc_rbtree_init(tree, s, i)                                           \
    agc_rbtree_sentinel_init(s);                                              \
    (tree)->root = s;                                                         \
    (tree)->sentinel = s;                                                     \
    (tree)->insert = i


void agc_rbtree_insert(agc_rbtree_t *tree, agc_rbtree_node_t *node);
void agc_rbtree_delete(agc_rbtree_t *tree, agc_rbtree_node_t *node);
void agc_rbtree_insert_value(agc_rbtree_node_t *root, agc_rbtree_node_t *node,
    agc_rbtree_node_t *sentinel);
void agc_rbtree_insert_timer_value(agc_rbtree_node_t *root,
    agc_rbtree_node_t *node, agc_rbtree_node_t *sentinel);
agc_rbtree_node_t *agc_rbtree_next(agc_rbtree_t *tree, agc_rbtree_node_t *node);


#define agc_rbt_red(node)               ((node)->color = 1)
#define agc_rbt_black(node)             ((node)->color = 0)
#define agc_rbt_is_red(node)            ((node)->color)
#define agc_rbt_is_black(node)          (!agc_rbt_is_red(node))
#define agc_rbt_copy_color(n1, n2)      (n1->color = n2->color)

#define agc_rbtree_sentinel_init(node)  agc_rbt_black(node)


static inline agc_rbtree_node_t *
agc_rbtree_min(agc_rbtree_node_t *node, agc_rbtree_node_t *sentinel)
{
    while (node->left != sentinel) {
        node = node->left;
    }

    return node;
}


#endif