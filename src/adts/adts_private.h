#pragma once

#include <stdint.h>
#include <unistd.h>

/**
 **************************************************************************
 * \brief
 *   Portable definition for userspace branch prediction optimizations
 *
 **************************************************************************
 */
#ifdef __GNUC__
    #define likely(_x)       __builtin_expect(!!(_x), 1)
    #define unlikely(_x)     __builtin_expect(!!(_x), 0)
#else
    #define likely(_x)       (_x)
    #define unlikely(_x)     (_x)
#endif


/**
 **************************************************************************
 * \brief
 *   Min / Max macros
 *
 * \details
 **************************************************************************
 */
#ifndef MIN
    #define MIN( _a, _b )  (((_a) < (_b)) ? (_a) : (_b))
#endif

#ifndef MAX
    #define MAX( _a, _b )  (((_a) > (_b)) ? (_a) : (_b))
#endif



/**
 **************************************************************************
 * \brief
 *   Detect missing serialization
 *
 * \details
 *   Locking for ADTS must be provided by consumer.  This service allows for
 *   detecting lack thereof.
 *
 **************************************************************************
 */
typedef struct {
    volatile int32_t busy;
} adts_sanity_t;



/**
 **************************************************************************
 * \brief
 *   Detect missing serialization
 *
 * \details
 *   Locking for ADTS must be provided by consumer.  This service allows for
 *   detecting lack thereof.
 *
 **************************************************************************
 */
static inline void
adts_sanity_exit( adts_sanity_t *p_sanity )
{
    p_sanity->busy--;

    return;
} /* adts_sanity_exit() */

static inline void
adts_sanity_entry( adts_sanity_t *p_sanity )
{
    /* if !0 on entry, then there's clearly a serialization error on
     * behalf of the ADTS consumer */
    assert(0 == p_sanity->busy);

    p_sanity->busy++;
    /* memory barriers are a better solution to volatile, but... still need
     * to investigate how to portably and efficiently pull this capability
     * into the adts shared library */
    //wmb();


    return;
} /* adts_sanity_entry() */


/**
 **************************************************************************
 * \brief
 *   Simple API for memory allocation on page boundaries
 *
 * \details
 *
 **************************************************************************
 */
static void *
adts_mem_zalloc( size_t bytes )
{
    void      *p_mem  = NULL;
    void     **pp_mem = &(p_mem);
    int32_t    rc     = 0;
    int32_t    align  = getpagesize(); /**< platform alignment */

    rc = posix_memalign(pp_mem, align, bytes);
    if (rc) {
        p_mem = NULL;
        goto exception;
    }
    memset(p_mem, 0, bytes);

exception:
    return p_mem;
} /* adts_mem_zalloc() */


/*
 ****************************************************************************
 *  \details
 *    Number of pointers per system pagesize
 ****************************************************************************
 */
static inline size_t
adts_ptrs_per_page( void )
{
    return getpagesize() / sizeof(void *);
} /* adts_ptrs_per_page() */

