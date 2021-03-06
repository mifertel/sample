
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <inttypes.h>

/* Toolbox */
#include <adts_queue.h>
#include <adts_sanity.h>
#include <adts_private.h>
#include <adts_display.h>


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
typedef struct queue_node_s {
    void                *p_data;
    size_t               bytes;
    struct queue_node_s *p_prev;
    struct queue_node_s *p_next;
} queue_node_t;


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
typedef struct {
    size_t        elems_curr;
    queue_node_t *p_head;
    queue_node_t *p_tail;
    adts_sanity_t sanity;
} queue_t;


/******************************************************************************
 * ####### #     # #     #  #####  #######   ###   ####### #     #  #####
 * #       #     # ##    # #     #    #       #    #     # ##    # #     #
 * #       #     # # #   # #          #       #    #     # # #   # #
 * #####   #     # #  #  # #          #       #    #     # #  #  #  #####
 * #       #     # #   # # #          #       #    #     # #   # #       #
 * #       #     # #    ## #     #    #       #    #     # #    ## #     #
 * #        #####  #     #  #####     #      ###   ####### #     #  #####
******************************************************************************/

/*
 ****************************************************************************
 *
 *
 ****************************************************************************
 */
bool
adts_queue_is_empty( adts_queue_t *p_adts_queue )
{
    queue_t *p_queue = (queue_t *) p_adts_queue;

    return ((NULL == p_queue->p_head) && (NULL == p_queue->p_tail));
} /* adts_queue_is_empty() */


/*
 ****************************************************************************
 *
 *
 ****************************************************************************
 */
bool
adts_queue_is_not_empty( adts_queue_t *p_adts_queue )
{
    return !(adts_queue_is_empty(p_adts_queue));
} /* adts_queue_is_not_empty() */


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
size_t
adts_queue_entries( adts_queue_t *p_adts_queue )
{
    queue_t *p_queue = (queue_t *) p_adts_queue;

    return p_queue->elems_curr;
} /* adts_queue_entries() */


/*
 ****************************************************************************
 *
 ****************************************************************************
 */
void
adts_queue_display( adts_queue_t *p_adts_queue )
{
    size_t         idx      = 0;
    size_t         elems    = 0;
    size_t         digits   = 0;
    queue_t       *p_queue  = (queue_t *) p_adts_queue;
    queue_node_t  *p_tmp    = NULL;
    adts_sanity_t *p_sanity = &(p_queue->sanity);

    adts_sanity_entry(p_sanity);

    /* display the entire queue with dynamic width formatting */
    elems  = adts_queue_entries(p_adts_queue);
    digits = adts_digits_decimal(elems);
    p_tmp  = p_queue->p_head;
    while (p_tmp) {
        printf("[%*d]  node: %p  vaddr: %p  bytes: %d \n",
                digits,
                idx,
                p_tmp,
                p_tmp->p_data,
                p_tmp->bytes);

        idx++;
        p_tmp = p_tmp->p_next;
    }

    adts_sanity_exit(p_sanity);

    return;
} /* adts_queue_display() */


/*
 ****************************************************************************
 *
 *
 ****************************************************************************
 */
void *
adts_queue_dequeue( adts_queue_t *p_adts_queue )
{
    void          *p_data   = NULL;
    queue_t       *p_queue  = (queue_t *) p_adts_queue;
    queue_node_t  *p_node   = NULL;
    adts_sanity_t *p_sanity = &(p_queue->sanity);

    adts_sanity_entry(p_sanity);

    /* Ensure we don't dereference a null tail pointer */
    if (likely(p_queue->p_tail)) {
        p_data = p_queue->p_tail->p_data;
    }else {
        goto exception;
    }

    if (p_queue->p_head == p_queue->p_tail) {
        /* This is the last element */
        p_queue->p_head = NULL;
        p_queue->p_tail = NULL;
    }else {
        /* Remove from tail only */
        p_queue->p_tail         = p_queue->p_tail->p_prev;
        p_queue->p_tail->p_next = NULL;
    }

    /* Remove the node memory */
    free(p_node);
    p_queue->elems_curr--;

exception:
    adts_sanity_exit(p_sanity);
    return p_data;
} /* adts_queue_dequeue() */


/*
 ****************************************************************************
 *
 *
 ****************************************************************************
 */
int32_t
adts_queue_enqueue( adts_queue_t *p_adts_queue,
                    void         *p_data,
                    size_t        bytes )
{
    int32_t        rc       = 0;
    queue_t       *p_queue  = (queue_t *) p_adts_queue;
    queue_node_t  *p_node   = NULL;
    adts_sanity_t *p_sanity = &(p_queue->sanity);

    adts_sanity_entry(p_sanity);

    p_node = adts_mem_zalloc(sizeof(*p_node));
    if (unlikely(NULL == p_node)) {
        rc = ENOMEM;
        goto exception;
    }
    p_node->p_data = p_data;
    p_node->bytes  = bytes;

    if ((NULL == p_queue->p_head) &&
        (NULL == p_queue->p_tail)) {
        /* first addition to list */
        p_queue->p_head = p_node;
        p_queue->p_tail = p_node;
    }else {
        /* Add new nodes to head of list */
        p_node->p_next          = p_queue->p_head;
        p_queue->p_head->p_prev = p_node;
        p_queue->p_head         = p_node;
    }

    p_queue->elems_curr++;

exception:
    adts_sanity_exit(p_sanity);
    return rc;
} /* adts_queue_enqueue() */



/*
 ****************************************************************************
 *
 *
 ****************************************************************************
 */
void
adts_queue_destroy( adts_queue_t *p_adts_queue )
{
    queue_t       *p_queue  = (queue_t *) p_adts_queue;
    adts_sanity_t *p_sanity = &(p_queue->sanity);

    adts_sanity_entry(p_sanity);

    free(p_queue);

    /* No adts_sanity_exit() since we've freed the memory */

    return;
} /* adts_queue_destroy() */


/*
 ****************************************************************************
 *
 *
 ****************************************************************************
 */
adts_queue_t *
adts_queue_create( void )
{
    adts_queue_t *p_adts_queue = adts_mem_zalloc(sizeof(*p_adts_queue));

    return p_adts_queue;
} /* adts_queue_create() */




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
utest_queue_bytes( void )
{
    CDISPLAY("[%u]", sizeof(queue_t));
    CDISPLAY("[%u]", sizeof(adts_queue_t));

    _Static_assert(sizeof(queue_t) <= sizeof(adts_queue_t),
        "Mismatch structs detected");

    return;
} /* utest_queue_bytes() */


/*
 ****************************************************************************
 * test control
 *
 ****************************************************************************
 */
static void
utest_control( void )
{
    CDISPLAY("=========================================================");
    {
        adts_queue_t *p_queue = NULL;

        p_queue = adts_queue_create();
        adts_queue_enqueue(p_queue, -1, 0x11223344);
        adts_queue_display(p_queue);
        (void) adts_queue_dequeue(p_queue);
        adts_queue_destroy(p_queue);
    }

    return;
} /* utest_control() */



/*
 ****************************************************************************
 * test entrypoint
 *
 ****************************************************************************
 */
void
utest_adts_queue( void )
{
    utest_control();

    return;
} /* utest_adts_queue() */
