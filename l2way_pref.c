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
 * File         : l2way_pref.c
 * Author       : HPS Research Group
 * Date         : 03/10/2004
 * Description  :
 ***************************************************************************************/

#include "debug/debug_macros.h"
#include "debug/debug_print.h"
#include "globals/global_defs.h"
#include "globals/global_types.h"
#include "globals/global_vars.h"

#include "globals/assert.h"
#include "globals/utils.h"
#include "op.h"

#include "core.param.h"
#include "debug/debug.param.h"
#include "general.param.h"
#include "libs/cache_lib.h"
#include "libs/hash_lib.h"
#include "libs/list_lib.h"
#include "memory/memory.h"
#include "memory/memory.param.h"
#include "prefetcher/l2l1pref.h"
#include "statistics.h"

/**************************************************************************************/
/* Macros */
#define DEBUG(args...) _DEBUG(DEBUG_WAY, ##args)
#define MAX_MEMORY_ACCESS_MAPS 256
#define ZONE_SIZE 8
/**************************************************************************************/
/* Global Variables */

extern Memory*       mem;
extern Dcache_Stage* dc;


/***************************************************************************************/
/* Local Prototypes */
Memory_Access_Map mem_access_map_table[MAX_MEMORY_ACCESS_MAPS];
/**************************************************************************************/

//For Init, note that we only have the param for L1 cache, so I think we should apply AMPM on L1 cache. Here from the memory param
//def file, we know the size, associativity are:
/*
DEF_PARAM(l1_size, L1_SIZE, uns, uns, (4 * 1024 * 1024), )
DEF_PARAM(l1_assoc, L1_ASSOC, uns, uns, 8, )
DEF_PARAM(l1_line_size, L1_LINE_SIZE, uns, uns,
          64, )
DEF_PARAM(l1_cycles, L1_CYCLES, uns, uns, 24, )
*/
//And remember the Dcache is actually the "L0" cache. If you check the param.def part.

void l2way_init(void) {
    // Use L1 Cache to init
    uns num_sets = L1_SIZE / L1_LINE_SIZE; 
    uns assoc = L1_ASSOC;                 
    uns line_size = L1_LINE_SIZE;         
    uns l1_cache_size = num_sets * assoc * line_size; 

    // Init Memory Access Map Table
    for (uns i = 0; i < MAX_MEMORY_ACCESS_MAPS; i++) {
        mem_access_map_table[i].zone_base = 0;
        memset(mem_access_map_table[i].prefetch_state, STATE_INIT, sizeof(mem_access_map_table[i].prefetch_state));
        mem_access_map_table[i].last_access_time = 0;
        mem_access_map_table[i].prefetch_degree = 1; //Set the ori to be 1
        mem_access_map_table[i].NTP = 0;
        mem_access_map_table[i].NGP = 0;
        mem_access_map_table[i].NCM = 0;
        mem_access_map_table[i].NCH = 0;
    }

    DEBUG("L1 Cache Size: %u bytes\n", l1_cache_size);
    DEBUG("L1 Cache Line Size: %u bytes\n", line_size);
    DEBUG("L1 Cache Associativity: %u\n", assoc);
    DEBUG("Number of Sets: %u\n", num_sets);
}
//Figure out the corresponding Addr to which zone.
int find_or_allocate_memory_access_map(Addr addr, Counter cycle_count) {
    Addr zone_base = addr & ~(ZONE_SIZE - 1);
    int free_index = -1, lru_index = -1;
    Counter oldest_time = cycle_count;

    for (int i = 0; i < MAX_MEMORY_ACCESS_MAPS; i++) {
        if (mem_access_map_table[i].zone_base == zone_base) {
            mem_access_map_table[i].last_access_time = cycle_count;
            return i;
        }
        if (mem_access_map_table[i].zone_base == 0 && free_index == -1) {
            free_index = i;
        } else if (mem_access_map_table[i].last_access_time < oldest_time) {
            oldest_time = mem_access_map_table[i].last_access_time;
            lru_index = i;
        }
    }

    int index = (free_index != -1) ? free_index : lru_index;
    mem_access_map_table[index].zone_base = zone_base;
    mem_access_map_table[index].last_access_time = cycle_count;
    memset(mem_access_map_table[index].prefetch_state, STATE_INIT, sizeof(mem_access_map_table[index].prefetch_state));
    mem_access_map_table[index].prefetch_degree = PREFETCH_DEGREE_DEFAULT;

    return index;
}
//Concat the maps
Concatenated_Access_Map concatenate_access_maps(int current_index) {
    Concatenated_Access_Map result;
    memset(result.combined_state, STATE_INIT, sizeof(result.combined_state));

    int indices[3] = { current_index - 1, current_index, current_index + 1 };

    for (int i = 0; i < 3; i++) {
        if (indices[i] >= 0 && indices[i] < MAX_MEMORY_ACCESS_MAPS) {
            Memory_Access_Map* map = &mem_access_map_table[indices[i]];
            memcpy(&result.combined_state[i * NUM_LINES_PER_ZONE],
                   map->prefetch_state, NUM_LINES_PER_ZONE);
            if (i == 0) {
                result.base_addr = map->zone_base;
            }
        }
    }

    return result;
}
//Check the stride to see whether they are ready to fetch
void detect_stride_patterns(Concatenated_Access_Map* map, Addr t, int prefetch_degree) {
    int num_lines = NUM_LINES_PER_ZONE * 3;
    int candidates[prefetch_degree];
    int num_candidates = 0;

    for (int k = 1; k <= NUM_LINES_PER_ZONE / 2 - 1; k++) {
        int t_index = (t - map->base_addr) / CACHE_LINE_SIZE;
        int t_plus_k = t_index + k;
        int t_plus_2k = t_index + 2 * k;

        if (t_plus_2k < num_lines &&
            map->combined_state[t_plus_k] == STATE_ACCESS &&
            map->combined_state[t_plus_2k] == STATE_ACCESS) {
            Addr prefetch_addr = t - k * CACHE_LINE_SIZE;
            if (num_candidates < prefetch_degree) {
                candidates[num_candidates++] = prefetch_addr;
            }
        }
    }

    qsort(candidates, num_candidates, sizeof(int), compare_distance_to_t);

    for (int i = 0; i < num_candidates; i++) {
        insert_l2way_pref_req(candidates[i], cycle_count + PREFETCH_LATENCY);
    }
}
//Dynamic adjusting
void adjust_prefetch_degree(Memory_Access_Map* map, int Tepoch, int Tlatency, 
                            double threshold_accuracy, double threshold_coverage, 
                            double threshold_hit_ratio, int max_degree) {
    double Mbandwidth = BANDWIDTH_DELAY_PRODUCT(map, Tepoch, Tlatency);
    double Museful = max_degree;

    if (PREFETCH_ACCURACY(map) < threshold_accuracy) {
        Museful -= 2;
    } else if (PREFETCH_COVERAGE(map) < threshold_coverage) {
        Museful -= 1;
    } else if (CACHE_HIT_RATIO(map) < threshold_hit_ratio) {
        Museful += 1;
    }

    map->prefetch_degree = (int)fmin(fmin(Mbandwidth, Museful), max_degree);
}
//Update the results later.
void epoch_update(int Tepoch, int Tlatency, double threshold_accuracy, 
                  double threshold_coverage, double threshold_hit_ratio, int max_degree) {
    for (int i = 0; i < MAX_MEMORY_ACCESS_MAPS; i++) {
        if (mem_access_map_table[i].zone_base != 0) {
            adjust_prefetch_degree(&mem_access_map_table[i], Tepoch, Tlatency, 
                                   threshold_accuracy, threshold_coverage, threshold_hit_ratio, max_degree);

            mem_access_map_table[i].NTP = 0;
            mem_access_map_table[i].NGP = 0;
            mem_access_map_table[i].NCM = 0;
            mem_access_map_table[i].NCH = 0;
        }
    }
}