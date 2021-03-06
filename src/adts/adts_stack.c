
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <inttypes.h>

/* Toolbox */
#include <adts_stack.h>
#include <adts_sanity.h>
#include <adts_private.h>
#include <adts_display.h>

/*
 ****************************************************************************
 *  Future work items:
 *    - probailistic failure reduction on resize growth.  Use a high-watermark
 *      to attempt grow operations to reduce likelyhood of malloc failures.
 *      the current implementaion may fail to grow because it waits till the
 *      stack if full before attempting.
 *    - stack_create() options:
 *      ADTS_STACK_STATIC  = requires dedicated create_wrapper()
 *      ADTS_STACK_DYNAMIC = (ADTS_STACK_DYNAMIC_GROW|ADTS_STACK_DYNAMIC_SHRINK
 *      ADTS_STACK_DYNAMIC_GROW
 *      ADTS_STACK_DYNAMIC_SHRINK
 *      ADTS_STACK_DESTROY_SANITY = display unfreed entries
 *      ADTS_STACK_DESTROY_AUTO_DRAIN
 *    - counters for all of the above to assist in identifying performance
 *      bottlenecks
 *    - utilization statistics
 *    - what is the current limit, vs current use?
 *    - what is the max collision depth?
 *    - how many current collisions exist?
 *    - optional: force an error on threadhold / assupmtion violations
 *
 *    - adts_stack_create()     - simple defaults
 *    - adts_stack_create_ext() - user defined options
 *
 ****************************************************************************
 */


/******************************************************************************
 #####  ####### ######  #     #  #####  ####### #     # ######  #######  #####
#     #    #    #     # #     # #     #    #    #     # #     # #       #     #
#          #    #     # #     # #          #    #     # #     # #       #
 #####     #    ######  #     # #          #    #     # ######  #####    #####
      #    #    #   #   #     # #          #    #     # #   #   #             #
#     #    #    #    #  #     # #     #    #    #     # #    #  #       #     #
 #####     #    #     #  #####   #####     #     #####  #     # #######  #####
******************************************************************************/

/*
 ****************************************************************************
 *
 ****************************************************************************
 */
typedef enum {
    STACK_GROW   = 0x22222222,
    STACK_SHRINK = 0x55555555,
} stack_resize_op_t;


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
typedef struct {
    void   *p_data;
    size_t  bytes;
} stack_node_t;


/*
 ****************************************************************************
 * \details
 *   The heap workspace is an array of stack_node_t, thus reserve as:
 *      32bit:
 *        512 entries = pagesize / stack_node_t size
 *
 *      64bit:
 *        256  entries = pagesize / stack_node_t size
 *
 ****************************************************************************
 */
#define STACK_DEFAULT_ELEMS (4)


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
typedef struct {
    size_t push;
    size_t pop;
    size_t peek;
    size_t height;
    size_t height_max;
} stack_stats_t;


/**
 **************************************************************************
 * \details
 *   lifetime resize statistics
 *
 **************************************************************************
 */
typedef struct {
    size_t grow;
    size_t shrink;
    size_t error;
} stack_resize_t;


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
typedef struct {
    size_t          elems_curr;
    size_t          elems_limit;
    stack_node_t   *workspace;
    adts_sanity_t   sanity;
    stack_stats_t   stats;
    stack_resize_t  resize;
} stack_t;




/******************************************************************************
 * ####### #     # #     #  #####  #######   ###   ####### #     #  #####
 * #       #     # ##    # #     #    #       #    #     # ##    # #     #
 * #       #     # # #   # #          #       #    #     # # #   # #
 * #####   #     # #  #  # #          #       #    #     # #  #  #  #####
 * #       #     # #   # # #          #       #    #     # #   # #       #
 * #       #     # #    ## #     #    #       #    #     # #    ## #     #
 * #        #####  #     #  #####     #      ###   ####### #     #  #####
******************************************************************************/


/**
 **************************************************************************
 * \details
 *   Internal / Private use only - serialization disabled
 *
 **************************************************************************
 */
#define stack_display( _p_stack, _p_message ) \
    do {                                      \
        bool             _private = true;     \
        adts_snapshot_t  _snap   = {0};       \
        adts_snapshot_t *_p_snap = &(_snap);  \
                                              \
        /* Get the call properties */         \
        adts_snapshot(_p_snap);               \
                                              \
        /* Perform the hexdump */             \
        stack_display_worker( _p_stack,       \
                             _p_message,      \
                             _p_snap,         \
                             _private );      \
    } while (0);


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
static void
stack_display_workspace( stack_t *p_stack )
{
    size_t elems  = p_stack->elems_curr;
    size_t digits = 0;

    /* display the entire stack with dynamic width formatting */
    digits = adts_digits_decimal(elems);
    for (size_t idx = 0; idx < elems; idx++) {
        printf("[%*d]  data: %16p  bytes: %8d \n",
                digits,
                idx,
                p_stack->workspace[idx].p_data,
                p_stack->workspace[idx].bytes);
    }

    return;
} /* stack_display_workspace() */


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
static void
stack_display_worker( stack_t         *p_stack,
                      char            *p_msg,
                      adts_snapshot_t *p_snap,
                      bool             private )
{
    stack_stats_t  *p_stats  = &(p_stack->stats);
    stack_resize_t *p_resize = &(p_stack->resize);

    printf("\n");
    printf("---------------------------------------------------------------\n");
    adts_snapshot_display(p_snap);
    if (p_msg) {
        printf(" Message: \"%s\"\n", p_msg);
    }
    printf("---------------------------------------------------------------\n");

    printf("stats.push           = %i\n", p_stats->push);
    printf("stats.pop            = %i\n", p_stats->pop);
    printf("stats.peek           = %i\n", p_stats->peek);
    printf("stats.height         = %i\n", p_stats->height);
    printf("stats.height_max     = %i\n", p_stats->height_max);

    printf("resize.grow          = %i\n", p_resize->grow);
    printf("resize.shrink        = %i\n", p_resize->shrink);
    printf("resize.error         = %i\n", p_resize->error);

    printf("elems_curr           = %i\n", p_stack->elems_curr);
    printf("elems_limit          = %i\n", p_stack->elems_limit);

    if (private) {
        printf("p_stack->workspace   = %i\n", p_stack->workspace);
        printf("p_stack->sanity.busy = %i\n", p_stack->sanity.busy);
    }

    stack_display_workspace(p_stack);

    return;
} /* stack_display_worker() */



/*
 ****************************************************************************
 * \details
 *   Given a starting input limit, round up to the next pow2.  Proced to
 *   grow or shrink to corresponding next pow2.  Then return largest prime
 *   within the new pow2 ceiling.
 *
 ****************************************************************************
 */
static size_t
stack_resize_limit( size_t            val,
                    stack_resize_op_t  op )
{
    size_t limit = adts_pow2_round_up(val);

    switch (op) {
        case STACK_GROW:
            limit *= 2;
            break;
        case STACK_SHRINK:
            limit /= 2;
            break;
        default:
            /* invalid op */
            assert(0);
    }

    return limit;
} /* stack_resize_limit() */


/*
 ****************************************************************************
 * \details
 *   Dynamically grow or shrink the workspace.  ADTS consumer is
 *   responsible for serialization.
 *
 ****************************************************************************
 */
static int32_t
stack_resize( stack_t          *p_stack,
              stack_resize_op_t op )
{
    size_t        limit_new = stack_resize_limit(p_stack->elems_limit, op);
    size_t        bytes     = 0;
    int32_t       rc        = 0;
    stack_node_t *p_old     = NULL;
    stack_node_t *p_tmp     = NULL;

    /* p_tmp used to handle error case and preserve the workspace */
    bytes = limit_new * sizeof(p_stack->workspace[0]);
    p_tmp = adts_mem_zalloc(bytes);
    if (NULL == p_tmp) {
        rc = ENOMEM;
        goto exception;
    }

    /* copy _current_ elements into new workspace */
    bytes = p_stack->elems_curr * sizeof(p_stack->workspace[0]);
    memcpy(p_tmp, p_stack->workspace, bytes);

    /* Set new stack properties fast by deferring free() */
    p_old                = p_stack->workspace;
    p_stack->workspace   = p_tmp;
    p_stack->elems_limit = limit_new;
    free(p_old);

exception:
    return rc;
} /* stack_resize() */


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
static inline void
stack_resize_check_shrink( stack_t *p_stack )
{
    size_t             limit_new = 0;
    size_t             trigger   = 0;
    int32_t            rc        = 0;
    stack_stats_t     *p_stats   = &(p_stack->stats);
    stack_resize_t    *p_resize  = &(p_stack->resize);
    stack_resize_op_t  op        = STACK_SHRINK;

    limit_new = stack_resize_limit(p_stack->elems_limit, op);
    if (STACK_DEFAULT_ELEMS > limit_new) {
        /* Prevent shrink to less than min stacktbl slots */
        goto exception;
    }

    /* Shrink if usage below .25 to avoid churn, note that the default is
     * assumed to be >4, otherwise this is a  devide by 0 operation. */
    trigger = p_stack->elems_limit / 4;
    if (p_stack->elems_curr < trigger) {
        rc = stack_resize(p_stack, op);
        if (rc) {
            p_resize->error++;
            goto exception;
        }
        p_resize->shrink++;
    }

exception:
    return;
} /* stack_resize_check_shrink() */


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
static inline int32_t
stack_resize_check_grow( stack_t *p_stack )
{
    int32_t         rc       = 0;
    stack_stats_t  *p_stats  = &(p_stack->stats);
    stack_resize_t *p_resize = &(p_stack->resize);

    if (unlikely(p_stack->elems_curr == p_stack->elems_limit)) {
        rc = stack_resize(p_stack, STACK_GROW);
        if (rc) {
            p_resize->error++;
            goto exception;
        }
        p_resize->grow++;
    }

exception:
    return rc;
} /* stack_resize_check_grow() */


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
bool
adts_stack_is_empty( adts_stack_t *p_adts_stack )
{
    stack_t *p_stack = (stack_t *) p_adts_stack;

    return (0 == p_stack->elems_curr);
} /* adts_stack_is_empty() */


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
bool
adts_stack_is_not_empty( adts_stack_t *p_adts_stack )
{
    return !(adts_stack_is_empty(p_adts_stack));
} /* adts_stack_is_not_empty() */


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
size_t
adts_stack_entries( adts_stack_t *p_adts_stack )
{
    stack_t *p_stack = (stack_t *) p_adts_stack;

    return p_stack->elems_curr;
} /* adts_stack_entries() */



/*
 ****************************************************************************
 *
 ****************************************************************************
 */
void
adts_stack_display_worker( adts_stack_t   *p_adts_stack,
                           char            *p_msg,
                           adts_snapshot_t *p_snap )
{
    bool           private  = false;
    stack_t       *p_stack  = (stack_t *) p_adts_stack;
    adts_sanity_t *p_sanity = &(p_stack->sanity);

    adts_sanity_entry(p_sanity);
    stack_display_worker(p_stack, p_msg, p_snap, private);
    adts_sanity_exit(p_sanity);

    return;
} /* adts_stack_display_worker() */


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
void *
adts_stack_peek( adts_stack_t *p_adts_stack )
{
    stack_t       *p_stack  = (stack_t *) p_adts_stack;
    stack_stats_t *p_stats  = &(p_stack->stats);
    adts_sanity_t *p_sanity = &(p_stack->sanity);

    adts_sanity_entry(p_sanity);

    int32_t       idx     = p_stack->elems_curr - 1;
    stack_node_t *p_elem  = &(p_stack->workspace[idx]);
    void         *p_data  = p_elem->p_data;

    p_stats->peek++;
    adts_sanity_exit(p_sanity);

    return p_data;
} /* adts_stack_peek() */


/*
 ****************************************************************************
 * \details
 *   Consider adding dynamic memory reduction logic for memory efficiency
 *
 ****************************************************************************
 */
void *
adts_stack_pop( adts_stack_t *p_adts_stack )
{
    stack_t       *p_stack  = (stack_t *) p_adts_stack;
    void          *p_data   = NULL;
    int32_t        idx      = 0;
    stack_node_t  *p_elem   = NULL;
    stack_stats_t *p_stats  = &(p_stack->stats);
    adts_sanity_t *p_sanity = &(p_stack->sanity);

    adts_sanity_entry(p_sanity);

    if (unlikely(0 >= p_stack->elems_curr)) {
        /* empty stack */
        goto exception;
    }

    idx    = p_stack->elems_curr - 1;
    p_elem = &(p_stack->workspace[idx]);

    p_stack->elems_curr--;
    p_stats->pop++;
    p_stats->height--;

    p_data = p_elem->p_data;
    memset(p_elem, 0, sizeof(*p_elem));

    stack_resize_check_shrink(p_stack);

exception:
    adts_sanity_exit(p_sanity);
    return p_data;
} /* adts_stack_pop() */


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
int32_t
adts_stack_push( adts_stack_t *p_adts_stack,
                 void         *p_data,
                 size_t        bytes )
{
    int32_t        rc       = 0;
    int32_t        idx      = 0;
    stack_t       *p_stack  = (stack_t *) p_adts_stack;
    stack_node_t  *p_elem   = NULL;
    stack_stats_t *p_stats  = &(p_stack->stats);
    adts_sanity_t *p_sanity = &(p_stack->sanity);

    adts_sanity_entry(p_sanity);

    rc = stack_resize_check_grow(p_stack);
    if (rc) {
        goto exception;
    }

    idx            = p_stack->elems_curr;
    p_elem         = &(p_stack->workspace[idx]);
    p_elem->p_data = p_data;
    p_elem->bytes  = bytes;

    p_stack->elems_curr++;
    p_stats->push++;
    p_stats->height++;
    p_stats->height_max = MAX(p_stats->height, p_stats->height_max);

exception:
    adts_sanity_exit(p_sanity);
    return rc;
} /* adts_stack_push() */


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
void
adts_stack_destroy( adts_stack_t *p_adts_stack )
{
    stack_t       *p_stack  = (stack_t *) p_adts_stack;
    adts_sanity_t *p_sanity = &(p_stack->sanity);

    adts_sanity_entry(p_sanity);

    free(p_stack->workspace);
    free(p_stack);

    /* No adts_sanity_exit() since we've freed the memory */

    return;
} /* adts_stack_destroy() */


/*
 ****************************************************************************
 *
 *
 ****************************************************************************
 */
adts_stack_t *
adts_stack_create( void )
{
    int32_t       rc           = 0;
    stack_t      *p_stack      = NULL;
    const size_t  elems        = STACK_DEFAULT_ELEMS;
    stack_node_t *p_elems      = NULL;
    adts_stack_t *p_adts_stack = NULL;

    p_adts_stack = adts_mem_zalloc(sizeof(*p_adts_stack));
    if (NULL == p_adts_stack) {
        rc = ENOMEM;
        goto exception;
    }

    p_elems = adts_mem_zalloc(elems * sizeof(*p_elems));
    if (NULL == p_elems) {
        rc = ENOMEM;
        goto exception;
    }

    p_stack              = (stack_t *) p_adts_stack;
    p_stack->workspace   = p_elems;
    p_stack->elems_limit = elems;

exception:
    if (rc) {
        if (p_elems) {
            free(p_elems);
			p_elems = NULL;
        }

        if (p_adts_stack) {
            free(p_adts_stack);
			p_adts_stack = NULL;
        }
    }

    return p_adts_stack;
} /* adts_stack_create() */



/******************************************************************************
 * #     # #     #   ###   ####### ####### #######  #####  #######  #####
 * #     # ##    #    #       #       #    #       #     #    #    #     #
 * #     # # #   #    #       #       #    #       #          #    #
 * #     # #  #  #    #       #       #    #####    #####     #     #####
 * #     # #   # #    #       #       #    #             #    #          #
 * #     # #    ##    #       #       #    #       #     #    #    #     #
 *  #####  #     #   ###      #       #    #######  #####     #     #####
******************************************************************************/

/**
 **************************************************************************
 * \brief
 *   Compile time structure sanity
 *
 * \details
 *   Sanitize the abstract data type interface.  Enforced in header file so
 *   as to catch improper usage/include by unauthorized callers.
 *
 **************************************************************************
 */
static void
utest_stack_bytes( void )
{
    CDISPLAY("[%u]", sizeof(stack_t));
    CDISPLAY("[%u]", sizeof(adts_stack_t));

    _Static_assert(sizeof(stack_t) <= sizeof(adts_stack_t),
        "Mismatch structs detected");

    return;
} /* utest_stack_bytes() */


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
static void
utest_control( void )
{
    size_t key[] = {-1,0,1,2,3,4,5,6,7,8,9};
    size_t elems = sizeof(key) / sizeof(key[0]);

    utest_stack_bytes();

    CDISPLAY("=========================================================");
    {
        CDISPLAY("Test: simple func test");

        char          foo[64] = {0};
        adts_stack_t *p_stack = NULL;

        p_stack = adts_stack_create();
        (void) adts_stack_push(p_stack, &(foo), sizeof(foo));
        (void) adts_stack_peek(p_stack);
        adts_stack_display(p_stack, NULL);
        (void) adts_stack_pop(p_stack);
        adts_stack_destroy(p_stack);
    }

    CDISPLAY("=========================================================");
    {
        CDISPLAY("Test: stack push");
        int32_t       rc      = 0;
        adts_stack_t *p_stack = NULL;

        p_stack = adts_stack_create();
        assert(p_stack);

        for (size_t idx = 0; idx < elems; idx++) {
            rc = adts_stack_push(p_stack, key[idx], sizeof(key[idx]));
            assert(0 == rc);
            adts_stack_display(p_stack, NULL);
        }

        adts_stack_destroy(p_stack);
    }

    CDISPLAY("=========================================================");
    {
        CDISPLAY("Test: stack push -> peek validate");

        void         *val     = 0;
        int32_t       rc      = 0;
        adts_stack_t *p_stack = NULL;

        p_stack = adts_stack_create();
        assert(p_stack);

        for (size_t idx = 0; idx < elems; idx++) {
            rc = adts_stack_push(p_stack, key[idx], sizeof(key[idx]));
            assert(0 == rc);
        }

        adts_stack_display(p_stack, NULL);

        val = adts_stack_peek(p_stack);
        assert(key[elems - 1] == val);

        adts_stack_destroy(p_stack);
    }

    CDISPLAY("=========================================================");
    {
        CDISPLAY("Test: stack push -> pop");

        void         *val     = 0;
        int32_t       rc      = 0;
        adts_stack_t *p_stack = NULL;

        p_stack = adts_stack_create();
        assert(p_stack);

        for (size_t idx = 0; idx < elems; idx++) {
            rc = adts_stack_push(p_stack, key[idx], sizeof(key[idx]));
            assert(0 == rc);
        }

        adts_stack_display(p_stack, NULL);

        for (int32_t idx = (elems - 1); idx >= 0; idx--) {
            (void) adts_stack_pop(p_stack);
            adts_stack_display(p_stack, NULL);
        }

        adts_stack_destroy(p_stack);
    }

    return;
} /* utest_control() */

/**
 **************************************************************************
 * \details
 *   Public test entrypoint
 *
 **************************************************************************
 */
void
utest_adts_stack( void )
{
    utest_control();

    return;
} /* utest_adts_stack() */
