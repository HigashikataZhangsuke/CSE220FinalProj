/* Copyright 2020 HPS/SAFARI Research Groups
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/***************************************************************************************
 * File         : pref_ampm.h
 * Author       : HPS Research Group
 * Date         : 1/23/2005
 * Description  :
 ***************************************************************************************/
#ifndef __PREF_AMPM_H__
#define __PREF_AMPM_H__

#include "pref_common.h"
#define AMPM_PAGE_COUNT 64
#define PREFETCH_DEGREE 2

typedef struct ampm_page
{
  // page address
  uns64 page;

  // The access map itself.
  // Each element is set when the corresponding cache line is accessed.
  // The whole structure is analyzed to make prefetching decisions.
  // While this is coded as an integer array, it is used conceptually as a single 64-bit vector.
  int access_map[64];

  // This map represents cache lines in this page that have already been prefetched.
  // We will only prefetch lines that haven't already been either demand accessed or prefetched.
  int pf_map[64];

  // used for page replacement
  uns64 lru;
} ampm_page_t;

ampm_page_t ampm_pages_ul1[AMPM_PAGE_COUNT];
HWP* hwp_ptr;


/*************************************************************/
/* HWP Interface */
void pref_ampm_init(HWP* hwp);

void pref_ampm_ul1_miss(uns8 proc_id, Addr lineAddr, Addr loadPC,
                          uns32 global_hist);
void pref_ampm_ul1_hit(uns8 proc_id, Addr lineAddr, Addr loadPC,
                         uns32 global_hist);
/*************************************************************/
/* Internal Function */
void init_ampm(HWP* hwp);
void pref_ampm_train(Addr lineAddr, Addr loadPC, Flag is_hit);

#endif /*  __PREF_AMPM_H__*/
