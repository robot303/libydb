#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include <yaml.h>

#include "yalloc.h"
#include "ytree.h"
#include "ylist.h"

#include "ydb.h"
#include "ynode.h"

struct _yhook
{
    ynode *node;
    yhook_func func;
    void *user;
    unsigned int flags;
};
typedef struct _yhook yhook;

// ynode flags
#define YNODE_FLAG_KEY 0x1
#define YNODE_FLAG_ITER 0x2

struct _ynode
{
    union {
        char *key;
        ylist_iter *iter;
    };
    union {
        ylist *list;
        ytree *dict;
        char *value;
        void *data;
    };
    unsigned char flags;
    unsigned char rsvd1;
    unsigned char rsvd2;
    unsigned char type;
    struct _ynode *parent;
    struct _yhook *hook;
};

#define IS_LAEF(x) ((x)->type == YNODE_TYPE_VAL)
#define SET_FLAG(flag, v) ((flag) = ((flag) | (v)))
#define UNSET_FLAG(flag, v) ((flag) = ((flag) & (~v)))
#define IS_SET(flag, v) ((flag) & (v))

static char *ystr_convert(char *str)
{
    int slen = 0;
    unsigned int spacectrl = 0;
    unsigned int space = 0;
    unsigned int ctrl = 0;

    for (slen = 0; str[slen]; slen++)
    {
        if (!isgraph(str[slen]))
        {
            if (isspace(str[slen]))
            {
                if (str[slen] == ' ')
                    space++;
                else
                    spacectrl++;
            }
            else
                ctrl++;
        }
    }

    if (space == 0 && ctrl == 0 && spacectrl == 0)
        return NULL;
    else
    {
        int len = 0;
        char *newstr = malloc((slen + spacectrl + (ctrl * 3) + 4));
        if (!newstr)
            return NULL;
        newstr[len] = '"';
        len++;
        for (slen = 0; str[slen]; slen++)
        {
            if (isprint(str[slen]))
            {
                newstr[len] = str[slen];
                len++;
            }
            else if (isspace(str[slen]))
            {
                int n;
                char c;
                switch (str[slen])
                {
                case 0x09:
                    c = 't';
                    break;
                case 0x0A:
                    c = 'n';
                    break;
                case 0x0B:
                    c = 'v';
                    break;
                case 0x0C:
                    c = 'f';
                    break;
                case 0x0D:
                    c = 'r';
                    break;
                default:
                    c = ' ';
                    break;
                }
                n = sprintf(newstr + len, "\\%c", c);
                if (n <= 0)
                    break;
                len = len + n;
            }
            else
            {
                int n = sprintf(newstr + len, "\\x%02X", str[slen]);
                if (n <= 0)
                    break;
                len = len + n;
            }
        }
        newstr[len] = '"';
        len++;
        newstr[len] = 0;
        return newstr;
    }
}

static void yhook_delete(ynode *cur);

// delete ynode regardless of the detachment of the parent
static void ynode_free(ynode *node)
{
    if (!node)
        return;
    switch (node->type)
    {
    case YNODE_TYPE_VAL:
        if (node->value)
            yfree(node->value);
        break;
    case YNODE_TYPE_DICT:
        ytree_destroy_custom(node->dict, (user_free)ynode_free);
        break;
    case YNODE_TYPE_LIST:
        ylist_destroy_custom(node->list, (user_free)ynode_free);
        break;
    default:
        assert(!YDB_E_TYPE_ERR);
    }
    yhook_delete(node);
    free(node);
}

// create ynode
static ynode *ynode_new(unsigned char type, char *value)
{
    ynode *node = malloc(sizeof(ynode));
    if (!node)
        return NULL;
    memset(node, 0x0, sizeof(ynode));
    switch (type)
    {
    case YNODE_TYPE_VAL:
        node->value = ystrdup(value);
        if (!node->value)
            goto _error;
        break;
    case YNODE_TYPE_DICT:
        node->dict = ytree_create((ytree_cmp)strcmp, (user_free)yfree);
        if (!node->dict)
            goto _error;
        break;
    case YNODE_TYPE_LIST:
        node->list = ylist_create();
        if (!node->list)
            goto _error;
        break;
    default:
        assert(!YDB_E_TYPE_ERR);
    }
    node->type = type;
    return node;
_error:
    free(node);
    return NULL;
}

// return parent after remove the node from the parent node.
static ynode *ynode_detach(ynode *node)
{
    ynode *parent;
    ynode *searched_node;
    if (!node)
        return NULL;
    if (!node->parent)
        return NULL;
    parent = node->parent;
    node->parent = NULL;
    switch (parent->type)
    {
    case YNODE_TYPE_DICT:
        assert(node->key && YDB_E_NO_ENTRY);
        searched_node = ytree_delete(parent->dict, node->key);
        UNSET_FLAG(node->flags, YNODE_FLAG_KEY);
        node->key = NULL;
        assert(searched_node && YDB_E_NO_ENTRY);
        assert(searched_node == node && YDB_E_INVALID_PARENT);
        break;
    case YNODE_TYPE_LIST:
        assert(node->iter && YDB_E_NO_ENTRY);
        ylist_erase(node->iter, NULL);
        UNSET_FLAG(node->flags, YNODE_FLAG_ITER);
        node->iter = NULL;
        break;
    case YNODE_TYPE_VAL:
    default:
        assert(!YDB_E_TYPE_ERR);
    }
    return parent;
}

// insert the ynode to the parent, the key is used for dict type.
// return old ynode that was being attached to the parent.
static ynode *ynode_attach(ynode *node, ynode *parent, char *key)
{
    ynode *old = NULL;
    if (!node || !parent)
        return NULL;
    if (parent->type == YNODE_TYPE_VAL)
        assert(!YDB_E_INVALID_PARENT);
    if (node->parent)
        ynode_detach(node);
    switch (parent->type)
    {
    case YNODE_TYPE_DICT:
        SET_FLAG(node->flags, YNODE_FLAG_KEY);
        node->key = ystrdup(key);
        old = ytree_insert(parent->dict, node->key, node);
        break;
    case YNODE_TYPE_LIST:
        // ignore key.
        SET_FLAG(node->flags, YNODE_FLAG_ITER);
        node->iter = ylist_push_back(parent->list, node);
        break;
    case YNODE_TYPE_VAL:
    default:
        assert(!YDB_E_TYPE_ERR);
    }
    node->parent = parent;
    return old;
}

// register the hook func to the target ynode.
ydb_res yhook_register(ynode *node, unsigned int flags, yhook_func func, void *user)
{
    yhook *hook;
    if (!node || !func)
        return YDB_E_NO_ARGS;
    if (node->hook)
        hook = node->hook;
    else
    {
        hook = malloc(sizeof(yhook));
        memset(hook, 0x0, sizeof(yhook));
    }
    if (!hook)
        return YDB_E_MEM;
    if (IS_SET(flags, YNODE_LEAF_FIRST))
        SET_FLAG(hook->flags, YNODE_LEAF_FIRST);
    else
        UNSET_FLAG(hook->flags, YNODE_LEAF_FIRST);
    hook->func = func;
    hook->user = user;
    hook->node = node;
    node->hook = hook;
    return YDB_OK;
}

// unregister the hook func from the target ynode.
void yhook_unregister(ynode *node)
{
    if (!node || !node->hook)
        return;
    free(node->hook);
    node->hook = NULL;
}

static int yhook_pre_run_for_delete(ynode *cur);
static int yhook_pre_run_for_delete_dict(void *key, void *data, void *addition)
{
    ynode *cur = data;
    key = (void *)key;
    addition = (void *)addition;
    return yhook_pre_run_for_delete(cur);
}

static int yhook_pre_run_for_delete_list(void *data, void *addition)
{
    ynode *cur = data;
    addition = (void *)addition;
    return yhook_pre_run_for_delete(cur);
}

// call the pre / post hook for deleting cur ynode.
static int yhook_pre_run_for_delete(ynode *cur)
{
    ydb_res res;
    ynode *node = cur;
    if (!cur)
        return YDB_E_NO_ENTRY;
    while (node)
    {
        yhook *hook = node->hook;
        if (hook && !(IS_SET(hook->flags, YNODE_LEAF_FIRST)))
        {
            if (cur->type == YNODE_TYPE_VAL)
                hook->func(YHOOK_OP_DELETE, cur, NULL, hook->user);
            else if (!IS_SET(hook->flags, YNODE_LEAF_ONLY))
                hook->func(YHOOK_OP_DELETE, cur, NULL, hook->user);
            break;
        }
        node = node->parent;
    }
    switch (cur->type)
    {
    case YNODE_TYPE_DICT:
        res = ytree_traverse(cur->dict, yhook_pre_run_for_delete_dict, NULL);
        break;
    case YNODE_TYPE_LIST:
        res = ylist_traverse(cur->list, yhook_pre_run_for_delete_list, NULL);
        break;
    case YNODE_TYPE_VAL:
        res = YDB_OK;
        break;
    default:
        assert(!YDB_E_TYPE_ERR);
    }
    node = cur;
    while (node)
    {
        yhook *hook = node->hook;
        if (hook && IS_SET(hook->flags, YNODE_LEAF_FIRST))
        {
            if (cur->type == YNODE_TYPE_VAL)
                hook->func(YHOOK_OP_DELETE, cur, NULL, hook->user);
            else if (!IS_SET(hook->flags, YNODE_LEAF_ONLY))
                hook->func(YHOOK_OP_DELETE, cur, NULL, hook->user);
            break;
        }
        node = node->parent;
    }
    return res;
}

// return ynode is changed or not.
static void yhook_pre_run(yhook_op_type op, ynode *parent, ynode *cur, ynode *new)
{
    yhook *hook;
    if (cur)
    {
        hook = cur->hook;
        if (hook && !(IS_SET(hook->flags, YNODE_LEAF_FIRST)))
        {
            if (cur->type == YNODE_TYPE_VAL)
                hook->func(op, cur, new, hook->user);
            else if (new && new->type == YNODE_TYPE_VAL)
                hook->func(op, cur, new, hook->user);
            else if (!IS_SET(hook->flags, YNODE_LEAF_ONLY))
                hook->func(op, cur, new, hook->user);
        }
    }
    while (parent)
    {
        hook = parent->hook;
        if (hook && !(IS_SET(hook->flags, YNODE_LEAF_FIRST)))
        {
            if (cur && cur->type == YNODE_TYPE_VAL)
                hook->func(op, cur, new, hook->user);
            else if (new && new->type == YNODE_TYPE_VAL)
                hook->func(op, cur, new, hook->user);
            else if (!IS_SET(hook->flags, YNODE_LEAF_ONLY))
                hook->func(op, cur, new, hook->user);
            break;
        }
        parent = parent->parent;
    }
}

static void yhook_post_run(yhook_op_type op, ynode *parent, ynode *cur, ynode *new)
{
    yhook *hook;
    if (cur)
    {
        hook = cur->hook;
        if (hook && IS_SET(hook->flags, YNODE_LEAF_FIRST))
        {
            if (cur->type == YNODE_TYPE_VAL)
                hook->func(op, cur, new, hook->user);
            else if (new && new->type == YNODE_TYPE_VAL)
                hook->func(op, cur, new, hook->user);
            else if (!IS_SET(hook->flags, YNODE_LEAF_ONLY))
                hook->func(op, cur, new, hook->user);
        }
    }
    while (parent)
    {
        hook = parent->hook;
        if (hook && IS_SET(hook->flags, YNODE_LEAF_FIRST))
        {
            if (cur && cur->type == YNODE_TYPE_VAL)
                hook->func(op, cur, new, hook->user);
            else if (new && new->type == YNODE_TYPE_VAL)
                hook->func(op, cur, new, hook->user);
            else if (!IS_SET(hook->flags, YNODE_LEAF_ONLY))
                hook->func(op, cur, new, hook->user);
            break;
        }
        parent = parent->parent;
    }
}

static void yhook_copy(ynode *dest, ynode *src)
{
    yhook *hook;
    if (!dest || !src)
        return;
    if (dest->hook || !src->hook)
        return;
    hook = malloc(sizeof(yhook));
    hook->func = src->hook->func;
    hook->user = src->hook->user;
    hook->node = dest;
    dest->hook = hook;
}

static void yhook_delete(ynode *cur)
{
    if (!cur || !cur->hook)
        return;
    free(cur->hook);
    cur->hook = NULL;
}

struct dump_ctrl
{
    enum
    {
        DUMP_TYPE_FP,
        DUMP_TYPE_FD,
        DUMP_TYPE_STR,
    } type;
    FILE *fp;
    int fd;
    char *buf;
    int buflen;
    int len;
#define DUMP_FLAG_DEBUG 0x1
    unsigned int flags;
    int end_level;
    int start_level;
    int level;
    int indent;
};

#define S10 "          "
static char *space = S10 S10 S10 S10 S10 S10 S10 S10 S10 S10;

static struct dump_ctrl *dump_ctrl_new(FILE *fp, int fd, char *buf, int buflen, int start_level, int end_level)
{
    struct dump_ctrl *cb;
    cb = malloc(sizeof(struct dump_ctrl));
    if (!cb)
        return NULL;
    if (fp)
        cb->type = DUMP_TYPE_FP;
    else if (fd)
        cb->type = DUMP_TYPE_FD;
    else if (buf && buflen > 0)
        cb->type = DUMP_TYPE_STR;
    else
    {
        cb->type = DUMP_TYPE_FP;
        fp = stdout;
    }
    cb->fp = fp;
    cb->fd = fd;
    cb->buf = buf;
    cb->buflen = buflen;
    cb->len = 0;
    cb->flags = 0x0;
    cb->end_level = end_level;
    cb->start_level = start_level;
    cb->level = 0;
    cb->indent = 0;
    return cb;
}

static struct dump_ctrl *dump_ctrl_new_debug(FILE *fp, int fd, char *buf, int buflen, int start_level, int end_level)
{
    struct dump_ctrl *cb;
    cb = dump_ctrl_new(fp, fd, buf, buflen, start_level, end_level);
    if (!cb)
        return NULL;
    cb->flags = DUMP_FLAG_DEBUG;
    return cb;
}

void dump_ctrl_free(struct dump_ctrl *cb)
{
    if (cb)
        free(cb);
}

int dump_ctrl_print(struct dump_ctrl *dump, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    switch (dump->type)
    {
    case DUMP_TYPE_FP:
        dump->len += vfprintf(dump->fp, format, args);
        break;
    case DUMP_TYPE_FD:
        dump->len += vdprintf(dump->fd, format, args);
        break;
    case DUMP_TYPE_STR:
        dump->len += vsnprintf((dump->buf + dump->len), (dump->buflen - dump->len), format, args);
        if (dump->buflen <= dump->len)
        {
            dump->buf[dump->buflen - 1] = 0;
            return YDB_E_FULL_BUF;
        }
        break;
    default:
        assert(!YDB_E_DUMP_CB);
    }
    va_end(args);
    return YDB_OK;
}

static int dump_ctrl_debug_ynode(struct dump_ctrl *dump, ynode *node)
{
    ydb_res res;
    int indent = dump->indent;
    if (indent < 0)
        return YDB_OK;

    // print indent
    res = dump_ctrl_print(dump, "%.*s", indent, space);
    if (res)
        return res;

    res = dump_ctrl_print(dump, "%p ", node);
    if (res)
        return res;

    // print key
    if (IS_SET(node->flags, YNODE_FLAG_KEY))
    {
        char *key = ystr_convert(node->key);
        res = dump_ctrl_print(dump, "{key: %s,", key ? key : node->key);
        if (key)
            free(key);
    }
    else if (IS_SET(node->flags, YNODE_FLAG_ITER))
        res = dump_ctrl_print(dump, "{key: %p,", ylist_data(node->iter));
    else
        res = dump_ctrl_print(dump, "{key: none,");
    if (res)
        return res;

    // print flags
    switch (node->type)
    {
    case YNODE_TYPE_VAL:
    {
        char *value = ystr_convert(node->value);
        res = dump_ctrl_print(dump, " value: %s,", value ? value : node->value);
        if (value)
            free(value);
        break;
    }
    case YNODE_TYPE_DICT:
    {
        res = dump_ctrl_print(dump, " dict: num=%d,", ytree_size(node->dict));
        break;
    }
    case YNODE_TYPE_LIST:
    {
        res = dump_ctrl_print(dump, " list: %s,", ylist_empty(node->list) ? "empty" : "not-empty");
        break;
    }
    default:
        assert(!YDB_E_TYPE_ERR);
    }
    if (res)
        return res;
    res = dump_ctrl_print(dump, " parent: %p}\n", node->parent);
    return res;
}

static int dump_ctrl_print_ynode(struct dump_ctrl *dump, ynode *node)
{
    int only_val = 0;
    ydb_res res;
    int indent = dump->indent;
    if (indent < 0)
        return YDB_OK;

    // print indent
    res = dump_ctrl_print(dump, "%.*s", indent, space);
    if (res)
        return res;

    // print key
    if (IS_SET(node->flags, YNODE_FLAG_KEY))
    {
        char *key = ystr_convert(node->key);
        res = dump_ctrl_print(dump, "%s:", key ? key : node->key);
        if (key)
            free(key);
    }
    else if (IS_SET(node->flags, YNODE_FLAG_ITER))
    {
        res = dump_ctrl_print(dump, "-");
    }
    else
    {
        only_val = 1;
    }

    if (res)
        return res;

    // print value
    if (node->type == YNODE_TYPE_VAL)
    {
        char *value = ystr_convert(node->value);
        res = dump_ctrl_print(dump, "%s%s\n",
                              only_val ? "" : " ",
                              value ? value : node->value);
        if (value)
            free(value);
    }
    else
    {
        res = dump_ctrl_print(dump, "\n");
    }
    return res;
}

static int dump_ctrl_dump_childen(struct dump_ctrl *dump, ynode *node);
static int dump_ctrl_traverse_dict(void *key, void *data, void *addition)
{
    struct dump_ctrl *dump = addition;
    ynode *node = data;
    key = (void *)key;
    return dump_ctrl_dump_childen(dump, node);
}

static int dump_ctrl_traverse_list(void *data, void *addition)
{
    struct dump_ctrl *dump = addition;
    ynode *node = data;
    return dump_ctrl_dump_childen(dump, node);
}

static int dump_ctrl_dump_parent(struct dump_ctrl *dump, ynode *node)
{
    ydb_res res = YDB_OK;
    ylist *parents = ylist_create();
    int start_level = dump->start_level;
    int end_level = (dump->end_level < 0) ? dump->end_level : 0;
    node = node->parent;
    start_level++;
    while (node && start_level <= end_level)
    {
        ylist_push_back(parents, node);
        node = node->parent;
        start_level++;
    }

    while (!ylist_empty(parents))
    {
        node = ylist_pop_back(parents);
        if (IS_SET(dump->flags, DUMP_FLAG_DEBUG))
            res = dump_ctrl_debug_ynode(dump, node);
        else
            res = dump_ctrl_print_ynode(dump, node);
        // printf("\ndump len=%d\n", dump->len);
        if (res)
            break;
        dump->indent++;
    }
    dump->start_level = (dump->start_level < 0) ? 0 : dump->start_level;
    ylist_destroy(parents);
    return res;
}

static int dump_ctrl_dump_childen(struct dump_ctrl *dump, ynode *node)
{
    ydb_res res = YDB_OK;
    if (dump->end_level < 0)
        return res;
    dump->end_level--;
    // printf("dump->start_level %d, dump->level %d\n", dump->start_level, dump->level);
    if (dump->start_level <= dump->level)
    {
        if (IS_SET(dump->flags, DUMP_FLAG_DEBUG))
            res = dump_ctrl_debug_ynode(dump, node);
        else
            res = dump_ctrl_print_ynode(dump, node);
        if (res)
            return res;
        dump->indent++;
    }

    dump->level++;
    switch (node->type)
    {
    case YNODE_TYPE_DICT:
        res = ytree_traverse(node->dict, dump_ctrl_traverse_dict, dump);
        break;
    case YNODE_TYPE_LIST:
        res = ylist_traverse(node->list, dump_ctrl_traverse_list, dump);
        break;
    case YNODE_TYPE_VAL:
        break;
    default:
        assert(!YDB_E_TYPE_ERR);
    }
    dump->level--;
    if (dump->start_level <= dump->level)
        dump->indent--;
    dump->end_level++;
    return res;
}

void ynode_dump_node(FILE *fp, int fd, char *buf, int buflen, ynode *node, int start_level, int end_level)
{
    struct dump_ctrl *dump;
    if (!node)
        return;
    if (start_level > end_level)
        return;
    dump = dump_ctrl_new_debug(fp, fd, buf, buflen, start_level, end_level);
    if (!dump)
        return;
    if (start_level < end_level)
        dump_ctrl_print(dump, "\n[dump (start_level=%d, end_level=%d)]\n", start_level, end_level);
    dump_ctrl_dump_parent(dump, node);
    dump_ctrl_dump_childen(dump, node);
    if (start_level < end_level)
        dump_ctrl_print(dump, "[dump (len=%d)]\n", dump->len);
    dump_ctrl_free(dump);
}

int ynode_snprintf(char *buf, int buflen, ynode *node, int start_level, int end_level)
{
    int len = -1;
    struct dump_ctrl *dump;
    if (!node)
        return -1;
    if (start_level > end_level)
        return 0;
    dump = dump_ctrl_new(NULL, 0, buf, buflen, start_level, end_level);
    if (!dump)
        return -1;
    dump_ctrl_dump_parent(dump, node);
    dump_ctrl_dump_childen(dump, node);
    len = dump->len;
    dump_ctrl_free(dump);
    return len;
}

int ynode_fprintf(FILE *fp, ynode *node, int start_level, int end_level)
{
    int len = -1;
    struct dump_ctrl *dump;
    if (!node)
        return -1;
    if (start_level > end_level)
        return 0;
    dump = dump_ctrl_new(fp, 0, NULL, 0, start_level, end_level);
    if (!dump)
        return -1;
    dump_ctrl_dump_parent(dump, node);
    dump_ctrl_dump_childen(dump, node);
    len = dump->len;
    dump_ctrl_free(dump);
    return len;
}

int ynode_write(int fd, ynode *node, int start_level, int end_level)
{
    int len = -1;
    struct dump_ctrl *dump;
    if (!node)
        return -1;
    if (start_level > end_level)
        return 0;
    dump = dump_ctrl_new(NULL, fd, NULL, 0, start_level, end_level);
    if (!dump)
        return -1;
    dump_ctrl_dump_parent(dump, node);
    dump_ctrl_dump_childen(dump, node);
    len = dump->len;
    dump_ctrl_free(dump);
    return len;
}

int ynode_printf(ynode *node, int start_level, int end_level)
{
    return ynode_fprintf(NULL, node, start_level, end_level);
}

int ydb_log_err_yaml(yaml_parser_t *parser)
{
    /* Display a parser error message. */
    switch (parser->error)
    {
    case YAML_MEMORY_ERROR:
        ydb_log_error("mem err: not enough memory for parsing\n");
        break;

    case YAML_READER_ERROR:
        if (parser->problem_value != -1)
        {
            ydb_log_error("reader error: %s: #%X at %zd\n", parser->problem,
                          parser->problem_value, parser->problem_offset);
        }
        else
        {
            ydb_log_error("reader error: %s at %zu\n", parser->problem,
                          parser->problem_offset);
        }
        break;

    case YAML_SCANNER_ERROR:
        if (parser->context)
        {
            ydb_log_error("scanner error: %s at line %zu, column %zu\n",
                          parser->context,
                          parser->context_mark.line + 1, parser->context_mark.column + 1);
            ydb_log_error("%s at line %zu, column %zu\n",
                          parser->problem, parser->problem_mark.line + 1,
                          parser->problem_mark.column + 1);
        }
        else
        {
            ydb_log_error("scanner error: %s at line %zu, column %zu\n",
                          parser->problem, parser->problem_mark.line + 1,
                          parser->problem_mark.column + 1);
        }
        break;

    case YAML_PARSER_ERROR:
        if (parser->context)
        {
            ydb_log_error("parser error: %s at line %zu, column %zu\n",
                          parser->context,
                          parser->context_mark.line + 1, parser->context_mark.column + 1);
            ydb_log_error("%s at line %zu, column %zu\n",
                          parser->problem, parser->problem_mark.line + 1,
                          parser->problem_mark.column + 1);
        }
        else
        {
            ydb_log_error("parser error: %s at line %zu, column %zu\n",
                          parser->problem, parser->problem_mark.line + 1,
                          parser->problem_mark.column + 1);
        }
        break;

    case YAML_COMPOSER_ERROR:
        if (parser->context)
        {
            ydb_log_error("composer error: %s at line %zu, column %zu\n",
                          parser->context,
                          parser->context_mark.line + 1, parser->context_mark.column + 1);
            ydb_log_error("%s at line %zu, column %zu\n",
                          parser->problem, parser->problem_mark.line + 1,
                          parser->problem_mark.column + 1);
            ydb_log_error("\n");
        }
        else
        {
            ydb_log_error("composer error: %s at line %zu, column %zu\n",
                          parser->problem, parser->problem_mark.line + 1,
                          parser->problem_mark.column + 1);
        }
        break;

    default:
        /* Couldn't happen. */
        ydb_log_error("internal error\n");
        break;
    }
    return 0;
}

char *yaml_token_str[] = {
    YDB_VNAME(YAML_NO_TOKEN),

    YDB_VNAME(YAML_STREAM_START_TOKEN),
    YDB_VNAME(YAML_STREAM_END_TOKEN),

    YDB_VNAME(YAML_VERSION_DIRECTIVE_TOKEN),
    YDB_VNAME(YAML_TAG_DIRECTIVE_TOKEN),
    YDB_VNAME(YAML_DOCUMENT_START_TOKEN),
    YDB_VNAME(YAML_DOCUMENT_END_TOKEN),

    YDB_VNAME(YAML_BLOCK_SEQUENCE_START_TOKEN),
    YDB_VNAME(YAML_BLOCK_MAPPING_START_TOKEN),
    YDB_VNAME(YAML_BLOCK_END_TOKEN),

    YDB_VNAME(YAML_FLOW_SEQUENCE_START_TOKEN),
    YDB_VNAME(YAML_FLOW_SEQUENCE_END_TOKEN),
    YDB_VNAME(YAML_FLOW_MAPPING_START_TOKEN),
    YDB_VNAME(YAML_FLOW_MAPPING_END_TOKEN),

    YDB_VNAME(YAML_BLOCK_ENTRY_TOKEN),
    YDB_VNAME(YAML_FLOW_ENTRY_TOKEN),
    YDB_VNAME(YAML_KEY_TOKEN),
    YDB_VNAME(YAML_VALUE_TOKEN),

    YDB_VNAME(YAML_ALIAS_TOKEN),
    YDB_VNAME(YAML_ANCHOR_TOKEN),
    YDB_VNAME(YAML_TAG_TOKEN),
    YDB_VNAME(YAML_SCALAR_TOKEN),
};

static char *ynode_strpbrk(char *src, char *src_end, char *stop)
{
    char *delimiter;
    if (strlen(stop) == 1)
    {
        for (; src != src_end; src++)
            if (*src == stop[0])
                return src;
        return NULL;
    }

    for (; src != src_end; src++)
    {
        for (delimiter = stop; *delimiter != 0; delimiter++)
        {
            if (*src == *delimiter)
                return src;
        }
    }
    return NULL;
}

struct ynod_query_data {
    yaml_parser_t *parser;
    int num_of_query;
};

static int ynod_yaml_query_handler(void *data, unsigned char *buffer, size_t size,
                             size_t *size_read)
{
    int len;
    char *start, *cur, *end, *buf;
    struct ynod_query_data *qd = data;
    yaml_parser_t *parser = (yaml_parser_t *)qd->parser;

    if (parser->input.string.current == parser->input.string.end)
    {
        *size_read = 0;
        return 1;
    }
    start = (char *)parser->input.string.start;
    cur = (char *)parser->input.string.current;
    end = (char *)parser->input.string.end;
    buf = (char *)buffer;

    char *specifier = ynode_strpbrk(cur, end, "%");
    while (specifier)
    {
        char *pre = specifier - 1;
        len = specifier - cur;
        if (size <= (size_t)len)
            break;
        memcpy(buf, cur, len);
        cur += len;
        size -= len;
        buf += len;

        if (specifier == start || isspace(*pre))
        {
            // add ""
            char *s_end = ynode_strpbrk(specifier, end, " \n:,}]\r\t\f\v");
            len = s_end - cur;
            if (size <= (len + 3) || len <= 0)
                goto _done;
            // copy
            sprintf(buf, "+%03d", qd->num_of_query);
            buf += 4;
            size -= 4;
            qd->num_of_query++;
            // *buf = '"';
            // buf++;
            // size--;

            memcpy(buf, cur, len);
            cur += len;
            buf += len;
            size -= len;

            // *buf = '"';
            // buf++;
            // size--;
        }
        else
        {
            if (size <= 1)
                break;
            *buf = *cur;
            cur++;
            buf++;
            size--;
        }

        specifier = ynode_strpbrk(cur, end, "%");
    }

    len = end - cur;
    if (len <= 0)
        goto _done;
    if (size <= (size_t)len)
        len = size;
    memcpy(buf, cur, len);
    cur += len;
    buf += len;
    size -= len;

_done:
    // printf("buff='%s'\n", buffer);
    *size_read = buf - ((char *)buffer);
    parser->input.string.current = (const unsigned char *)cur;
    return 1;
}

ydb_res ynode_scan(FILE *fp, char *buf, int buflen, ynode **n, int *queryform)
{
    ydb_res res = YDB_OK;
    int level = 0;
    yaml_parser_t parser;
    yaml_token_t token; /* new variable */
    ylist *stack;
    ynode *top = NULL;
    ynode *node = NULL;
    ynode *old = NULL;
    char *key = NULL;
    char *value = NULL;
    struct ynod_query_data qdata;
    enum YAML_NEXT_TOKEN
    {
        YAML_NEXT_NONE,
        YAML_NEXT_MAPPING_KEY_SCALAR,
        YAML_NEXT_MAPPING_VAL_SCALAR,
        YAML_NEXT_SEQUENCE_ENTRY_SCALAR,
    } next = YAML_NEXT_NONE;

    if ((!fp && !buf) || !n)
    {
        res = YDB_E_NO_ARGS;
        ydb_log_res(res);
        return res;
    }

    stack = ylist_create();
    if (!stack)
    {
        res = YDB_E_MEM;
        ydb_log_res(res);
        return res;
    }

    /* Initialize parser */
    if (!yaml_parser_initialize(&parser))
    {
        ydb_log_err_yaml(&parser);
        yaml_parser_delete(&parser);
        res = YDB_E_YAML_INIT;
        ydb_log_res(res);
        return res;
    }

    if (queryform)
    {
        /* Set input buf and handler */
        parser.input.string.start = (const unsigned char *)buf;
        parser.input.string.current = (const unsigned char *)buf;
        parser.input.string.end = (const unsigned char *)buf + buflen;
        qdata.parser = &parser;
        qdata.num_of_query = 0;
        yaml_parser_set_input(&parser, ynod_yaml_query_handler, &qdata);
    }
    else
    {
        if (fp)
            yaml_parser_set_input_file(&parser, fp);
        else
            yaml_parser_set_input_string(&parser,
                                         (const unsigned char *)buf, (size_t)buflen);
    }

    /* BEGIN new code */
    do
    {
        yaml_parser_scan(&parser, &token);
        if (!token.type)
        {
            ydb_log_err_yaml(&parser);
            res = YDB_E_YAML_EMPTY_TOKEN;
            break;
        }
        if (YDB_LOGGING_DEBUG)
        {
            if (token.type == YAML_BLOCK_END_TOKEN ||
                token.type == YAML_FLOW_MAPPING_END_TOKEN ||
                token.type == YAML_FLOW_SEQUENCE_END_TOKEN)
                level--;
            ydb_log(YDB_LOG_DBG, "%.*s%s\n", level * 2, space, yaml_token_str[token.type]);
            if (token.type == YAML_BLOCK_SEQUENCE_START_TOKEN ||
                token.type == YAML_BLOCK_MAPPING_START_TOKEN ||
                token.type == YAML_FLOW_SEQUENCE_START_TOKEN ||
                token.type == YAML_FLOW_MAPPING_START_TOKEN)
                level++;
        }

        switch (token.type)
        {
        case YAML_KEY_TOKEN:
            node = ylist_back(stack);
            if (node && node->type != YNODE_TYPE_DICT)
            {
                res = YDB_E_INVALID_YAML_KEY;
                break;
            }
            if (key)
            {
                ydb_log(YDB_LOG_DBG, "!!empty\n");
                node = ynode_new(YNODE_TYPE_VAL, NULL);
                old = ynode_attach(node, ylist_back(stack), key);
                ynode_free(old);
                yfree(key);
                key = NULL;
            }
            next = YAML_NEXT_MAPPING_KEY_SCALAR;
            break;
        case YAML_VALUE_TOKEN:
            next = YAML_NEXT_MAPPING_VAL_SCALAR;
            break;
            /* Block delimeters */
        case YAML_BLOCK_SEQUENCE_START_TOKEN:
        case YAML_BLOCK_MAPPING_START_TOKEN:
        case YAML_FLOW_SEQUENCE_START_TOKEN:
        case YAML_FLOW_MAPPING_START_TOKEN:
        {
            int node_type;
            if (token.type == YAML_BLOCK_SEQUENCE_START_TOKEN ||
                token.type == YAML_FLOW_SEQUENCE_START_TOKEN)
                node_type = YNODE_TYPE_LIST;
            else
                node_type = YNODE_TYPE_DICT;
            if (ylist_empty(stack))
            {
                if (top)
                {
                    if (node_type != top->type)
                    {
                        res = YDB_E_INVALID_YAML_TOP;
                        break;
                    }
                    node = top;
                }
                else
                    node = ynode_new(node_type, NULL);
            }
            else
                node = ynode_new(node_type, NULL);
            if (!node)
            {
                res = YDB_E_MEM;
                break;
            }
            old = ynode_attach(node, ylist_back(stack), key);
            ynode_free(old);
            ylist_push_back(stack, node);
            yfree(key);
            key = NULL;
            ydb_log(YDB_LOG_DBG, "(last stack entry=%p)\n", ylist_back(stack));
            next = YAML_NEXT_NONE;
            break;
        }
        case YAML_BLOCK_ENTRY_TOKEN: // ,
            node = ylist_back(stack);
            if (!node || (node && node->type != YNODE_TYPE_LIST))
                res = YDB_E_INVALID_YAML_ENTRY;
            next = YAML_NEXT_SEQUENCE_ENTRY_SCALAR;
            break;
        case YAML_FLOW_ENTRY_TOKEN: // ,
            node = ylist_back(stack);
            if (!node || (node && node->type != YNODE_TYPE_DICT))
                res = YDB_E_INVALID_YAML_ENTRY;
            next = YAML_NEXT_MAPPING_KEY_SCALAR;
            break;
        case YAML_BLOCK_END_TOKEN:
        case YAML_FLOW_MAPPING_END_TOKEN:
        case YAML_FLOW_SEQUENCE_END_TOKEN:
            if (key)
            {
                ydb_log(YDB_LOG_DBG, "** empty ynode **\n");
                node = ynode_new(YNODE_TYPE_VAL, NULL);
                old = ynode_attach(node, ylist_back(stack), key);
                ynode_free(old);
                yfree(key);
                key = NULL;
            }
            top = ylist_pop_back(stack);
            ydb_log(YDB_LOG_DBG, "(last stack entry=%p)\n", ylist_back(stack));
            next = YAML_NEXT_NONE;
            break;
        case YAML_SCALAR_TOKEN:
            switch (next)
            {
            case YAML_NEXT_MAPPING_KEY_SCALAR:
                value = (char *)token.data.scalar.value;
                ydb_log(YDB_LOG_DBG, "%.*s%s\n", level * 2, space, value);
                key = ystrdup(value);
                break;
            case YAML_NEXT_MAPPING_VAL_SCALAR:
            case YAML_NEXT_SEQUENCE_ENTRY_SCALAR:
            case YAML_NEXT_NONE: // only have a scalar (leaf) node
                value = (char *)token.data.scalar.value;
                ydb_log(YDB_LOG_DBG, "%.*s%s\n", level * 2, space, value);
                node = ynode_new(YNODE_TYPE_VAL, value);
                old = ynode_attach(node, ylist_back(stack), key);
                ynode_free(old);
                if (key)
                    yfree(key);
                key = NULL;
                top = node;
                break;
            default:
                res = YDB_E_INVALID_YAML_INPUT;
                break;
            }
            next = YAML_NEXT_NONE;
            break;
        case YAML_DOCUMENT_START_TOKEN:
            break;
        case YAML_DOCUMENT_END_TOKEN:
            break;
        case YAML_STREAM_START_TOKEN:
            break;
        case YAML_STREAM_END_TOKEN:
            break;
        case YAML_TAG_TOKEN:
            ydb_log(YDB_LOG_DBG, "handle=%s suffix=%s\n", token.data.tag.handle, token.data.tag.suffix);
            break;
        /* Others */
        case YAML_VERSION_DIRECTIVE_TOKEN:
        case YAML_TAG_DIRECTIVE_TOKEN:
        case YAML_ANCHOR_TOKEN:
        case YAML_ALIAS_TOKEN:
        default:
            break;
        }
        if (res)
            break;

        if (token.type != YAML_STREAM_END_TOKEN)
        {
            yaml_token_delete(&token);
        }
    } while (token.type != YAML_STREAM_END_TOKEN);

    yaml_token_delete(&token);
    /* END new code */

    /* Cleanup */
    yaml_parser_delete(&parser);

    if (!ylist_empty(stack) && !res)
        res = YDB_E_INVALID_YAML_INPUT;

    if (res)
    {
        ydb_log_res(res);
        if (!top)
            top = ylist_front(stack);
        while (!ylist_empty(stack))
            ylist_pop_back(stack);
        ynode_free(ynode_top(top));
        top = NULL;
    }
    ylist_destroy(stack);
    if (key)
        yfree(key);
    *n = top;
    if (queryform)
        *queryform = qdata.num_of_query;
    return res;
}

ydb_res ynode_fscanf(FILE *fp, ynode **n)
{
    return ynode_scan(fp, NULL, 0, n, 0);
}

ydb_res ynode_scanf(ynode **n)
{
    return ynode_scan(stdin, NULL, 0, n, 0);
}

ydb_res ynode_read(int fd, ynode **n)
{
    int dup_fd = dup(fd);
    FILE *fp = fdopen(dup_fd, "r");
    ydb_res res = ynode_scan(fp, NULL, 0, n, 0);
    if (fp)
        fclose(fp);
    return res;
}

ydb_res ynode_sscanf(char *buf, int buflen, ynode **n)
{
    // FILE *fp;
    ydb_res res;
    if (!buf || buflen < 0)
        return YDB_E_NO_ARGS;
    // fp = fmemopen(buf, buflen, "r");
    res = ynode_scan(NULL, buf, buflen, n, 0);
    // if (fp)
    //     fclose(fp);
    return res;
}



static ynode *ynode_find_child(ynode *node, char *key)
{
    if (node->type == YNODE_TYPE_DICT)
    {
        return ytree_search(node->dict, key);
    }
    else if (node->type == YNODE_TYPE_LIST)
    {
        int count = 0;
        ylist_iter *iter;
        int index = 0;
        if (strspn(key, "0123456789") != strlen(key))
            return NULL;
        index = atoi(key);
        for (iter = ylist_first(node->list);
             !ylist_done(iter);
             iter = ylist_next(iter))
        {
            if (index == count)
                return ylist_data(iter);
            count++;
        }
        return NULL;
    }
    else
        return NULL;
}

// lookup the ynode in the path
ynode *ynode_search(ynode *node, char *path)
{
    int i, j;
    int failcount;
    char token[512];
    ynode *found;

    if (!path || !node)
        return NULL;
    if (node->type == YNODE_TYPE_VAL)
        return NULL;
    i = 0;
    j = 0;
    failcount = 0;
    if (path[0] == '/') // ignore first '/'
        i = 1;
    for (; path[i]; i++)
    {
        if (path[i] == '/')
        {
            token[j] = 0;
            ydb_log_debug("token: %s\n", token);
            found = ynode_find_child(node, token);
            if (found)
            {
                failcount = 0;
                node = found;
                j = 0;
                continue;
            }
            else
            {
                if (failcount > 0)
                    return NULL;
                failcount++;
            }
        }
        token[j] = path[i];
        j++;
    }
    // lookup the last token of the path.
    if (j > 0)
    {
        token[j] = 0;
        ydb_log_debug("token: %s\n", token);
        found = ynode_find_child(node, token);
        if (found)
            return found;
        else
            return NULL;
    }
    return node;
}

// return ynodes' type
unsigned char ynode_type(ynode *node)
{
    if (node)
        return node->type;
    return 0;
}

// return ynodes' value if that is a leaf.
char *ynode_value(ynode *node)
{
    if (node && node->type == YNODE_TYPE_VAL)
        return node->value;
    return NULL;
}

// return ynodes' key if that has a hash key.
char *ynode_key(ynode *node)
{
    if (!node || !node->parent)
        return NULL;
    if (node->parent->type == YNODE_TYPE_DICT)
        return node->key;
    return NULL;
}

// return ynodes' index if the nodes' parent is a list.
int ynode_index(ynode *node)
{
    if (!node || !node->parent)
        return -1;
    if (node->parent->type == YNODE_TYPE_LIST)
    {
        int index = 0;
        ylist_iter *iter;
        for (iter = ylist_first(node->parent->list);
             !ylist_done(iter);
             iter = ylist_next(iter))
        {
            if (node->iter == iter)
                return index;
            index++;
        }
    }
    return -1;
}

// return the top node of the ynode.
ynode *ynode_top(ynode *node)
{
    while (node)
    {
        if (!node->parent)
            break;
        node = node->parent;
    }
    return node;
}

// return the parent node of the ynode.
ynode *ynode_up(ynode *node)
{
    if (node)
        return node->parent;
    return NULL;
}

// return the first child node of the ynode.
ynode *ynode_down(ynode *node)
{
    if (!node)
        return NULL;
    switch (node->type)
    {
    case YNODE_TYPE_DICT:
        return ytree_data(ytree_first(node->dict));
    case YNODE_TYPE_LIST:
        return ylist_front(node->list);
    case YNODE_TYPE_VAL:
    default:
        break;
    }
    return NULL;
}

// return the previous sibling node of the ynode.
ynode *ynode_prev(ynode *node)
{
    if (!node || !node->parent)
        return NULL;
    switch (node->parent->type)
    {
    case YNODE_TYPE_DICT:
    {
        ytree_iter *iter = ytree_find(node->parent->dict, node->key);
        iter = ytree_prev(node->parent->dict, iter);
        return ytree_data(iter);
    }
    case YNODE_TYPE_LIST:
        return ylist_data(ylist_prev(node->iter));
    case YNODE_TYPE_VAL:
    default:
        break;
    }
    return NULL;
}

// return the next sibling node of the ynode.
ynode *ynode_next(ynode *node)
{
    if (!node || !node->parent)
        return NULL;
    switch (node->parent->type)
    {
    case YNODE_TYPE_DICT:
    {
        ytree_iter *iter = ytree_find(node->parent->dict, node->key);
        iter = ytree_next(node->parent->dict, iter);
        return ytree_data(iter);
    }
    case YNODE_TYPE_LIST:
        return ylist_data(ylist_next(node->iter));
    case YNODE_TYPE_VAL:
    default:
        break;
    }
    return NULL;
}

// return the first sibling node of the ynode.
ynode *ynode_first(ynode *node)
{
    if (!node || !node->parent)
        return NULL;
    switch (node->parent->type)
    {
    case YNODE_TYPE_DICT:
        return ytree_data(ytree_first(node->parent->dict));
    case YNODE_TYPE_LIST:
        return ylist_front(node->parent->list);
    case YNODE_TYPE_VAL:
    default:
        break;
    }
    return NULL;
}

// return the last sibling node of the ynode.
ynode *ynode_last(ynode *node)
{
    if (!node || !node->parent)
        return NULL;
    switch (node->parent->type)
    {
    case YNODE_TYPE_DICT:
        return ytree_data(ytree_last(node->parent->dict));
    case YNODE_TYPE_LIST:
        return ylist_back(node->parent->list);
    case YNODE_TYPE_VAL:
    default:
        break;
    }
    return NULL;
}

int ynode_path_fprintf(FILE *fp, ynode *node, int level)
{
    if (node && level >= 0)
    {
        int len, curlen;
        len = ynode_path_fprintf(fp, node->parent, level - 1);
        if (IS_SET(node->flags, YNODE_FLAG_KEY))
        {
            char *key = ystr_convert(node->key);
            curlen = fprintf(fp, "/%s", key ? key : node->key);
            if (key)
                free(key);
            if (curlen <= 0)
                return len;
            return len + curlen;
        }
        else if (IS_SET(node->flags, YNODE_FLAG_ITER))
        {
            int index = ynode_index(node);
            if (index < 0)
                return len;
            curlen = fprintf(fp, "/%d", index);
            if (curlen <= 0)
                return len;
            return len + curlen;
        }
        else
        {
            return len;
        }
    }
    return 0;
}

char *ynode_path(ynode *node, int level)
{
    char *buf = NULL;
    size_t buflen = 0;
    FILE *fp;
    if (!node)
        return NULL;
    fp = open_memstream(&buf, &buflen);
    ynode_path_fprintf(fp, node, level);
    if (fp)
        fclose(fp);
    if (buf && buflen > 0)
        return buf;
    if (buf)
        free(buf);
    return NULL;
}

char *ynode_path_and_val(ynode *node, int level)
{
    char *buf = NULL;
    size_t buflen = 0;
    FILE *fp;
    if (!node)
        return NULL;
    fp = open_memstream(&buf, &buflen);
    ynode_path_fprintf(fp, node, level);
    if (node->type == YNODE_TYPE_VAL)
    {
        char *value = ystr_convert(node->value);
        fprintf(fp, "=%s", value ? value : node->value);
        if (value)
            free(value);
    }
    if (fp)
        fclose(fp);
    if (buf && buflen > 0)
        return buf;
    if (buf)
        free(buf);
    return NULL;
}

static yhook_op_type ynode_op_get(ynode *cur, ynode *new)
{
    if (!cur && !new)
        return YHOOK_OP_NONE;
    else if (!cur && new)
        return YHOOK_OP_CREATE;
    else if (cur && !new)
        return YHOOK_OP_DELETE;
    else
    {
        if (cur->type == new->type)
        {
            if (cur->type == YNODE_TYPE_VAL)
            {
                if (strcmp(cur->value, new->value) == 0)
                    return YHOOK_OP_NONE;
                return YHOOK_OP_REPLACE;
            }
            return YHOOK_OP_NONE;
        }
        return YHOOK_OP_REPLACE;
    }
}

char *yhook_op_str[] =
    {
        "yhook_op_none",
        "yhook_op_create",
        "yhook_op_replace",
        "yhook_op_delete",
};

static ynode *ynode_control(ynode *cur, ynode *src, ynode *parent, char *key)
{
    ynode *new = NULL;
    yhook_op_type op;
    if (parent)
    {
        if (cur && cur->parent != parent)
            assert(!YDB_E_INVALID_PARENT);
        switch (parent->type)
        {
        case YNODE_TYPE_LIST:
            break;
        case YNODE_TYPE_DICT:
            if (!cur && key)
                cur = ytree_search(parent->dict, key);
            break;
        case YNODE_TYPE_VAL:
            return NULL;
        default:
            assert(!YDB_E_TYPE_ERR);
        }
    }

    op = ynode_op_get(cur, src);

    if (op == YHOOK_OP_CREATE || op == YHOOK_OP_REPLACE)
    {
        new = ynode_new(src->type, src->value);
        if (!new)
            return NULL;
    }
    else if (op == YHOOK_OP_NONE)
        new = cur;

    if (YDB_LOGGING_DEBUG)
    {
        char *path;
        ydb_log_debug("op (%s)\n", yhook_op_str[op]);
        path = ynode_path_and_val(cur, YDB_LEVEL_MAX);
        ydb_log_debug("cur: %s\n", path?path:"null");
        if (path)
            free(path);
        path = ynode_path_and_val(new, YDB_LEVEL_MAX);
        ydb_log_debug("new: %s\n", path?path:"null");
        if (path)
            free(path);
    }

    switch (op)
    {
    case YHOOK_OP_CREATE:
        ynode_attach(new, parent, key);
        yhook_pre_run(op, parent, cur, new);
        break;
    case YHOOK_OP_REPLACE:
        ynode_attach(new, parent, key);
        yhook_pre_run(op, parent, cur, new);
        break;
    case YHOOK_OP_DELETE:
        yhook_pre_run_for_delete(cur);
        break;
    case YHOOK_OP_NONE:
    default:
        break;
    }

    if (src)
    {
        switch (src->type)
        {
        case YNODE_TYPE_DICT:
        {
            ytree_iter *iter = ytree_first(src->dict);
            for (; iter != NULL; iter = ytree_next(src->dict, iter))
            {
                ynode *src_child = ytree_data(iter);
                ynode *cur_child = ynode_find_child(new, src_child->key);
                ynode *new_child = ynode_control(cur_child, src_child, new, src_child->key);
                if (!new_child)
                    ydb_log_error("unable to add child node (src_child->key: %s)\n");
            }
            break;
        }
        case YNODE_TYPE_LIST:
        {
            ylist_iter *iter;
            for (iter = ylist_first(src->list);
                 !ylist_done(iter);
                 iter = ylist_next(iter))
            {
                ynode *src_child = ylist_data(iter);
                ynode *new_child = ynode_control(NULL, src_child, new, NULL);
                if (!new_child)
                    ydb_log_error("unable to add child node (src_child->key: %s)\n");
            }
            break;
        }
        case YNODE_TYPE_VAL:
            break;
        default:
            assert(!YDB_E_TYPE_ERR);
        }
    }

    switch (op)
    {
    case YHOOK_OP_CREATE:
        yhook_post_run(op, parent, cur, new);
        break;
    case YHOOK_OP_REPLACE:
        yhook_post_run(op, parent, cur, new);
        ynode_free(cur);
        break;
    case YHOOK_OP_DELETE:
        ynode_detach(cur);
        ynode_free(cur);
        break;
    case YHOOK_OP_NONE:
    default:
        break;
    }
    return new;
}

// create single ynode and attach to parent
// return created ynode
ynode *ynode_create(unsigned char type, char *key, char *value, ynode *parent)
{
    ynode *cur, *new;
    new = ynode_new(type, value);
    cur = ynode_control(NULL, new, parent, key);
    ynode_free(new);
    return cur;
}

// create new ynodes to parent using src
// return created ynode top
ynode *ynode_create_copy(ynode *src, ynode *parent, char *key)
{
    return ynode_control(NULL, src, parent, key);
}

// create new ynodes using path
// return the last created ynode.
ynode *ynode_create_path(char *path, ynode *parent)
{
    int i, j;
    char *key = NULL;
    ynode *new = NULL;
    ynode *node = NULL;
    char token[512];
    if (!path)
        return NULL;
    i = 0;
    j = 0;
    if (parent)
        new = ynode_new(parent->type, NULL);
    else
        new = ynode_new(YNODE_TYPE_DICT, NULL);
    if (!new)
        return NULL;

    if (path[0] == '/') // ignore first '/'
        i = 1;
    for (; path[i]; i++)
    {
        if (path[i] == '/') // '/' is working as delimiter
        {
            token[j] = 0;
            ydb_log_debug("token: %s\n", token);
            node = ynode_new(YNODE_TYPE_DICT, NULL);
            ynode_attach(node, new, token);
            new = node;
            j = 0;
        }
        else if (path[i] == '=')
        {
            if (key) // '=' is represented twice.
                goto _fail;
            token[j] = 0;
            ydb_log_debug("token: %s\n", token);
            key = ystrdup(token);
            if (!key)
                goto _fail;
            j = 0;
        }
        else
        {
            token[j] = path[i];
            j++;
        }
    }
    // lookup the last token of the path.
    if (j > 0)
    {
        token[j] = 0;
        if (key)
        {
            ydb_log_debug("key: %s, token: %s\n", key, token);
            node = ynode_new(YNODE_TYPE_VAL, token);
            ynode_attach(node, new, key);
            yfree(key);
            key = NULL;
        }
        else
        {
            ydb_log_debug("token: %s\n", token);
            node = ynode_new(YNODE_TYPE_DICT, NULL);
            ynode_attach(node, new, token);
        }
        new = node;
    }
    if (parent)
    {
        new = ynode_top(new);
        ynode_merge(parent, new);
        ynode_free(new);
        if (key)
            yfree(key);
        return ynode_search(parent, path);
    }
    return new;
_fail:
    if (key)
        yfree(key);
    ynode_free(ynode_top(new));
    return NULL;
}

// copy src ynodes (including all sub ynodes)
ynode *ynode_copy(ynode *src)
{
    ynode *dest;
    if (!src)
        return NULL;
    dest = ynode_new(src->type, src->value);
    if (!dest)
        return NULL;

    yhook_copy(dest, src);

    switch (src->type)
    {
    case YNODE_TYPE_DICT:
    {
        ytree_iter *iter = ytree_first(src->dict);
        for (; iter != NULL; iter = ytree_next(src->dict, iter))
        {
            ynode *src_child = ytree_data(iter);
            ynode *dest_child = ynode_copy(src_child);
            if (!dest_child)
                goto _fail;
            ynode_attach(dest_child, dest, src_child->key);
        }
        break;
    }
    case YNODE_TYPE_LIST:
    {
        ylist_iter *iter;
        for (iter = ylist_first(src->list);
             !ylist_done(iter);
             iter = ylist_next(iter))
        {
            ynode *src_child = ylist_data(iter);
            ynode *dest_child = ynode_copy(src_child);
            if (!dest_child)
                goto _fail;
            ynode_attach(dest_child, dest, src_child->key);
        }
        break;
    }
    case YNODE_TYPE_VAL:
        // nothing to do
        break;
    default:
        assert(!YDB_E_TYPE_ERR);
    }
    return dest;
_fail:
    if (dest)
        ynode_free(dest);
    return NULL;
}

// merge src ynode to dest.
// dest is modified by the operation.
// return modified dest.
ynode *ynode_merge(ynode *dest, ynode *src)
{
    ynode *parent;
    if (!dest)
        return ynode_copy(src);
    parent = dest->parent;
    parent = ynode_control(dest, src, parent, dest->key);
    return parent;
}

// merge src ynode to dest.
// dest and src is not modified.
// New ynode is returned.
ynode *ynode_merge_new(ynode *dest, ynode *src)
{
    char *buf = NULL;
    size_t buflen = 0;
    FILE *fp;
    ynode *clone = NULL;
    if (!dest && !src)
        return NULL;
    fp = open_memstream(&buf, &buflen);
    if (dest->type == YNODE_TYPE_VAL)
        fprintf(fp, "%s", dest->value);
    else
        ynode_fprintf(fp, dest, 0, YDB_LEVEL_MAX);
    if (src->type == YNODE_TYPE_VAL)
        fprintf(fp, "%s", src->value);
    else
        ynode_fprintf(fp, src, 0, YDB_LEVEL_MAX);
    if (fp)
        fclose(fp);
    ynode_sscanf(buf, buflen, &clone);
    if (buf)
        free(buf);
    return clone;
}

// deleted cur ynode (including all sub ynodes).
void ynode_delete(ynode *cur)
{
    if (cur)
        ynode_control(cur, NULL, cur->parent, cur->key);
}

struct ynode_traverse_data {
    ynode_callback cb;
    void *addition;
    unsigned int flags;
};

static ydb_res ynode_traverse_sub(ynode *cur, struct ynode_traverse_data *tdata);
static int ynode_traverse_dict(void *key, void *data, void *addition)
{
    ynode *cur = data;
    struct ynode_traverse_data *tdata = addition;
    key = (void *)key;
    return ynode_traverse_sub(cur, tdata);
}

static int ynode_traverse_list(void *data, void *addition)
{
    ynode *cur = data;
    struct ynode_traverse_data *tdata = addition;
    return ynode_traverse_sub(cur, tdata);
}

static ydb_res ynode_traverse_sub(ynode *cur, struct ynode_traverse_data *tdata)
{
    ydb_res res = YDB_OK;
    if (!IS_SET(tdata->flags, YNODE_LEAF_FIRST))
    {
        if (cur->type == YNODE_TYPE_VAL)
        {
            res = tdata->cb(cur, tdata->addition);
        }
        else
        {
            if (!IS_SET(tdata->flags, YNODE_LEAF_ONLY))
                res = tdata->cb(cur, tdata->addition);
        }
    }
    if (res)
        return res;

    switch (cur->type)
    {
    case YNODE_TYPE_DICT:
        res = ytree_traverse(cur->dict, ynode_traverse_dict, tdata);
        break;
    case YNODE_TYPE_LIST:
        res = ylist_traverse(cur->list, ynode_traverse_list, tdata);
        break;
    case YNODE_TYPE_VAL:
        res = YDB_OK;
        break;
    default:
       assert(!YDB_E_TYPE_ERR);
    }
    if (res)
        return res;

    if (IS_SET(tdata->flags, YNODE_LEAF_FIRST))
    {
        if (cur->type == YNODE_TYPE_VAL)
        {
            res = tdata->cb(cur, tdata->addition);
        }
        else
        {
            if (!IS_SET(tdata->flags, YNODE_LEAF_ONLY))
                res = tdata->cb(cur, tdata->addition);
        }
    }
    return res;
}

ydb_res ynode_traverse(ynode *cur, ynode_callback cb, void *addition, unsigned int flags)
{
    struct ynode_traverse_data tdata;
    if (!cur || !cb)
        return YDB_E_NO_ARGS;
    tdata.cb = cb;
    tdata.addition = addition;
    tdata.flags = flags;
    return ynode_traverse_sub(cur, &tdata);
}

// find the ref ynode on the same position in target ynode grapes.
ynode *ynode_lookup(ynode *target, ynode *ref)
{
    ylist *parents;
    if (!target || !ref)
        return NULL;
    parents = ylist_create();
    if (!parents)
        return NULL;
    
    while (ref->parent) {
        ylist_push_front(parents, ref);
        ref = ref->parent;
    };

    while (!ylist_empty(parents) && target) {
        ref = ylist_pop_front(parents);
        int index = ynode_index(ref);
        if (index >= 0)
        {
            char key[64];
            sprintf(key, "%d", index);
            target = ynode_find_child(target, key);
        }
        else
        {
            target = ynode_find_child(target, ref->key);
        }
        // ynode_dump(target, 0, 0);
    }
    ylist_destroy(parents);
    return target;
}