



#include <errno.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <inttypes.h>

/* Toolbox */
#include <adts_private.h>
#include <adts_services.h>




/*
 ****************************************************************************
 * Each new term in the Fibonacci sequence is generated by adding the
 * previous two terms. By starting with 1 and 2, the first 10 terms will be:
 *
 * 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, ...
 *
 * By considering the terms in the Fibonacci sequence whose values do not
 * exceed four million, find the sum of the even-valued terms.
 *
 * Time:  O(n) // worst case is 2, average case is much better near ~1.6xx
 * Space: O(1)
 ****************************************************************************
 */
size_t
pe0002( const size_t limit )
{
    size_t  sum  = 0;
    size_t  prev = 0;
    size_t  curr = 0;

    if (0 == limit || 1 == limit) {
        return limit;
    }

    prev = 0;
    curr = 1;
    while (limit > curr) {
        size_t x = prev + curr;

        prev = curr;
        curr = x;

        if (0 == (curr % 2)) {
            sum += curr;
        }

        //CDISPLAY("curr: %8i  sum: %8i", curr, sum);
    }

    CDISPLAY("sum: %8i", sum);
    return sum;
} /* pe0002() */


/*
 ****************************************************************************
 *  If we list all the natural numbers below 10 that are multiples of 3 or 5,
 *  we get 3, 5, 6 and 9. The sum of these multiples is 23.
 *
 *  Find the sum of all the multiples of 3 or 5 below 1000.
 *
 * Time:  O(n/x + n/y) // more efficient than O(n)
 * Space: O(1)
 ****************************************************************************
 */
size_t
pe0001_e1( size_t limit )
{
    size_t val = 0;
    size_t n3  = (limit - 1) / 3;
    size_t n5  = (limit - 1) / 5;

    for (int32_t i = 0; i <= n3; i++) {
        val += 3 * i;
    }

    for (int32_t i = 0; i <= n5; i++) {
        val += 5 * i;
    }

    CDISPLAY("%i", val);
    return val;
} /* pe0001_e1() */


/*
 ****************************************************************************
 *  If we list all the natural numbers below 10 that are multiples of 3 or 5,
 *  we get 3, 5, 6 and 9. The sum of these multiples is 23.
 *
 *  Find the sum of all the multiples of 3 or 5 below 1000.
 *
 * Time:  O(n)  // not a good solution for time complexity
 * Space: O(1)
 ****************************************************************************
 */
size_t
pe0001( size_t limit )
{
    size_t val = 0;

    for (int32_t i = 0; i < limit; i++) {

        if ((0 == (i % 3)) || (0 == (i % 5))) {
            val += i;
        }
    }

    CDISPLAY("%i", val);
    return val;
} /* pe0001() */




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
        CDISPLAY("Test: 1");
        size_t arr[] = { 0, 10, 20, 50, 1000 };
        size_t elems = sizeof(arr) / sizeof(arr[0]);

        for (int32_t i = 0; i < elems; i++) {
            size_t limit = arr[i];

            CDISPLAY("Input: %i", limit);
            (void) pe0001(limit);
            (void) pe0001_e1(limit);
        }
    }

    CDISPLAY("=========================================================");
    {
        CDISPLAY("Test: 2");
        size_t arr[] = { 0, 10, 20, 50, 1000, (1000*1000*4) };
        size_t elems = sizeof(arr) / sizeof(arr[0]);

        for (int32_t i = 0; i < elems; i++) {
            size_t limit = arr[i];

            CDISPLAY("Input: %i", limit);
            (void) pe0002(limit);
            //(void) pe0001_e1(limit);
        }
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
utest_pe( void )
{
    CDISPLAY("=========================================================");

    utest_control();

    return;
} /* utest_pe() */
