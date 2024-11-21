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
 * File         : l2way_pref.h
 * Author       : HPS Research Group
 * Date         : 03/11/2004
 * Description  :
 ***************************************************************************************/
#ifndef __L2WAY_PREF_H__
#define __L2WAY_PREF_H__

#include "globals/global_types.h"
#include "pref_type.h"

// way predictor training structure
typedef struct L2way_Rec_Struct {
  uns     last_way;
  uns     pred_way;
  uns     counter;
  Counter last_access_time;
} L2way_Rec;

typedef struct L2set_Rec_Struct {
  Addr last_addr;
  Addr pred_addr;
} L2set_Rec;

typedef struct L1pref_Req_Struct {
  Flag    valid;
  Addr    va;
  Counter time;
  Counter rdy_cycle;
} L1pref_Req;

//Declare mapping data sturctures here
typedef struct Memory_Access_Map {
  Addr    zone_base;                  // Base addr for the hot zone
  uns8    prefetch_state[NUM_LINES_PER_ZONE]; // 2-bits status array
  Counter last_access_time;           // LRU policy's static data
  int     prefetch_degree;            // Curr Degree
  Counter NTP;                        // Total Prefetch accounts
  Counter NGP;                        // valid accounts
  Counter NCM;                        // raw cache misses
  Counter NCH;                        // raw cache hit.
} Memory_Access_Map;

//Concat prev,curr and next three map to manage stride.
typedef struct Concatenated_Access_Map {
  uns8 combined_state[NUM_LINES_PER_ZONE * 3];
  Addr base_addr;
} Concatenated_Access_Map;

void l2way_init(void);
//Interfaces for manage map sturcture
int find_or_allocate_memory_access_map(Addr addr, Counter cycle_count);
Concatenated_Access_Map concatenate_access_maps(int current_index);
//Dynamic adjusting
void adjust_prefetch_degree(Memory_Access_Map* map, int Tepoch, int Tlatency,
                            double threshold_accuracy, double threshold_coverage,
                            double threshold_hit_ratio, int max_degree);

void epoch_update(int Tepoch, int Tlatency, double threshold_accuracy, 
                  double threshold_coverage, double threshold_hit_ratio, int max_degree);
//Debugging
void print_memory_access_maps(void);
void print_epoch_stats(int Tepoch);

//void l2way_pref(Mem_Req_Info* req);
//void l2way_pref_train(Mem_Req_Info* req);
//void l2way_pref_pred(Mem_Req_Info* req);
//void insert_l2way_pref_req(Addr va, Counter time);
//void update_l2way_pref_req_queue(void);
#endif /* #ifndef __L2WAY_PREF_H__ */
