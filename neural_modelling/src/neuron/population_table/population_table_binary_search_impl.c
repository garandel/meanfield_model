/*
 * Copyright (c) 2017-2019 The University of Manchester
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//! \file
//! \brief Master population table implementation that uses binary search
#include "population_table.h"
#include <neuron/synapse_row.h>
#include <debug.h>
#include <stdbool.h>
#include <bit_field.h>

//! \brief The number of bits of address.
//!        This is a constant as it is used more than once below.
#define N_ADDRESS_BITS 23

//! \brief The shift to apply to indirect addresses.
//!    The address is in units of four words, so this multiplies by 16 (= up
//!    shifts by 4)
#define INDIRECT_ADDRESS_SHIFT 4

//! \brief. An Invalid address and row length.
//!    Used to keep indices aligned between delayed and undelayed tables
#define INVALID_ADDRESS ((1 << N_ADDRESS_BITS) - 1)

//! \brief The maximum row length supported in words.
//!    This excludes the row header words.
#define MAX_ROW_LENGTH 256

//! \brief An entry in the master population table.
typedef struct master_population_table_entry {
    //! The key to match against the incoming message
    uint32_t key;
    //! The mask to select the relevant bits of \p key for matching
    uint32_t mask;
    //! The index into ::address_list for this entry
    uint32_t start: 15;
    //! Flag to indicate if an extra_info struct is present
    uint32_t extra_info_flag: 1;
    //! The number of entries in ::address_list for this entry
    uint32_t count: 16;
} master_population_table_entry;

//! \brief A packed extra info (note: same size as address and row length)
typedef struct extra_info {
    //! The mask to apply to the key once shifted get the core index
    uint32_t core_mask: 16;
    //! The shift to apply to the key to get the core part (0-31)
    uint32_t mask_shift: 5;
    //! The number of neurons per core (up to 2048)
    uint32_t n_neurons: 11;
} extra_info;

//! \brief A packed address and row length (note: same size as extra info)
typedef struct {
    //! the length of the row
    uint32_t row_length : 8;
    //! the address
    uint32_t address : N_ADDRESS_BITS;
    //! whether this is a direct/single address
    uint32_t is_single : 1;
} address_and_row_length;

//! \brief An entry in the address list is either an address and row length or extra
//! info if flagged.
typedef union {
    address_and_row_length addr;
    extra_info extra;
} address_list_entry;

//! The master population table. This is sorted.
static master_population_table_entry *master_population_table;

//! The length of ::master_population_table
static uint32_t master_population_table_length;

//! The array of information that points into the synaptic matrix
static address_list_entry *address_list;

//! Base address for the synaptic matrix's indirect rows
static uint32_t synaptic_rows_base_address;

//! Base address for the synaptic matrix's direct rows
static uint32_t direct_rows_base_address;

//! \brief the number of times a DMA resulted in 0 entries
static uint32_t ghost_pop_table_searches = 0;

//! \brief the number of times packet isnt in the master pop table at all!
static uint32_t invalid_master_pop_hits = 0;

//! \brief The last spike received
static spike_t last_spike = 0;

//! \brief The last neuron id for the key
static uint32_t last_neuron_id = 0;

//! the index for the next item in the ::address_list
static uint16_t next_item = 0;

//! The number of relevant items remaining in the ::address_list
static uint16_t items_to_go = 0;

//! \brief The number of packets dropped because the bitfield filter says
//!     they don't hit anything
static uint32_t bit_field_filtered_packets = 0;

//! The bitfield map
bit_field_t *connectivity_bit_field = NULL;

//! \name Support functions
//! \{

//! \brief Get the direct row address out of an entry
//! \param[in] entry: the table entry
//! \return a direct row address
static inline uint32_t get_direct_address(address_and_row_length entry) {
    return entry.address + direct_rows_base_address;
}

//! \brief Get the standard address offset out of an entry
//!
//! The address is in units of four words, so this multiplies by 16 (= up
//! shifts by 4)
//! \param[in] entry: the table entry
//! \return a row address (which is an offset)
static inline uint32_t get_offset(address_and_row_length entry) {
    return entry.address << INDIRECT_ADDRESS_SHIFT;
}

//! \brief Get the standard address out of an entry
//! \param[in] entry: the table entry
//! \return a row address
static inline uint32_t get_address(address_and_row_length entry) {
    return get_offset(entry) + synaptic_rows_base_address;
}

//! \brief Get the length of the row from the entry
//!
//! Row lengths are stored offset by 1, to allow 1-256 length rows
//!
//! \param[in] entry: the table entry
//! \return the row length
static inline uint32_t get_row_length(address_and_row_length entry) {
    return entry.row_length + 1;
}

//! \brief Get the total number of neurons on cores which come before this core
//! \param[in] extra: The extra info entry
//! \param[in] spike: The spike received
//! \return the base neuron number of this core
static inline uint32_t get_core_sum(extra_info extra, spike_t spike) {
    return ((spike >> extra.mask_shift) & extra.core_mask) *
            extra.n_neurons;
}

//! \brief Get the source neuron ID for a spike given its table entry (without extra info)
//! \param[in] entry: the table entry
//! \param[in] spike: the spike
//! \return the neuron ID
static inline uint32_t get_neuron_id(
        master_population_table_entry entry, spike_t spike) {
    return spike & ~entry.mask;
}

//! \brief Get the neuron id of the neuron on the source core, for a spike with
//         extra info
//! \param[in] entry: the table entry
//! \param[in] extra_info: the extra info entry
//! \param[in] spike: the spike received
//! \return the source neuron id local to the core
static inline uint32_t get_local_neuron_id(
        master_population_table_entry entry, extra_info extra, spike_t spike) {
    return spike & ~(entry.mask | (extra.core_mask << extra.mask_shift));
}

//! \brief Get the full source neuron id for a spike with extra info
//! \param[in] entry: the table entry
//! \param[in] extra_info: the extra info entry
//! \param[in] spike: the spike received
//! \return the source neuron id
static inline uint32_t get_extended_neuron_id(
        master_population_table_entry entry, extra_info extra, spike_t spike) {
    uint32_t local_neuron_id = get_local_neuron_id(entry, extra, spike);
    uint32_t neuron_id = local_neuron_id + get_core_sum(extra, spike);
#ifdef DEBUG
    uint32_t n_neurons = get_n_neurons(extra);
    if (local_neuron_id > n_neurons) {
        log_error("Spike %u is outside of expected neuron id range"
            "(neuron id %u of maximum %u)", spike, local_neuron_id, n_neurons);
        rt_error(RTE_SWERR);
    }
#endif
    return neuron_id;
}

//! \brief Prints the master pop table.
//!
//! For debugging
static inline void print_master_population_table(void) {
    log_info("master_population\n");
    for (uint32_t i = 0; i < master_population_table_length; i++) {
        master_population_table_entry entry = master_population_table[i];
        log_info("key: 0x%08x, mask: 0x%08x", entry.key, entry.mask);
        int count = entry.count;
        int start = entry.start;
        if (entry.extra_info_flag) {
            extra_info extra = address_list[start].extra;
            start += 1;
            log_info("    core_mask: 0x%08x, core_shift: %u, n_neurons: %u",
                    extra.core_mask, extra.mask_shift, extra.n_neurons);
        }
        for (uint16_t j = start; j < (start + count); j++) {
            address_and_row_length addr = address_list[j].addr;
            if (addr.address == INVALID_ADDRESS) {
                log_info("    index %d: INVALID", j);
            } else if (!addr.is_single) {
                log_info("    index %d: offset: %u, address: 0x%08x, row_length: %u",
                    j, get_offset(addr), get_address(addr), get_row_length(addr));
            } else {
                log_info("    index %d: offset: %u, address: 0x%08x, single",
                    j, addr.address, get_direct_address(addr));
            }
        }
    }
    log_info("Population table has %u entries", master_population_table_length);
}
//! \}

//! \name API functions
//! \{

bool population_table_initialise(
        address_t table_address, address_t synapse_rows_address,
        address_t direct_rows_address, uint32_t *row_max_n_words) {
    log_debug("population_table_initialise: starting");

    master_population_table_length = table_address[0];
    log_debug("master pop table length is %d\n", master_population_table_length);
    log_debug("master pop table entry size is %d\n",
            sizeof(master_population_table_entry));
    uint32_t n_master_pop_bytes =
            master_population_table_length * sizeof(master_population_table_entry);
    uint32_t n_master_pop_words = n_master_pop_bytes >> 2;
    log_debug("pop table size is %d\n", n_master_pop_bytes);

    // only try to malloc if there's stuff to malloc.
    if (n_master_pop_bytes != 0) {
        master_population_table = spin1_malloc(n_master_pop_bytes);
        if (master_population_table == NULL) {
            log_error("Could not allocate master population table");
            return false;
        }
    }

    uint32_t address_list_length = table_address[1];
    uint32_t n_address_list_bytes =
            address_list_length * sizeof(address_list_entry);

    // only try to malloc if there's stuff to malloc.
    if (n_address_list_bytes != 0) {
        address_list = spin1_malloc(n_address_list_bytes);
        if (address_list == NULL) {
            log_error("Could not allocate master population address list");
            return false;
        }
    }

    log_debug("pop table size: %u (%u bytes)",
            master_population_table_length, n_master_pop_bytes);
    log_debug("address list size: %u (%u bytes)",
            address_list_length, n_address_list_bytes);

    // Copy the master population table
    spin1_memcpy(master_population_table, &table_address[2],
            n_master_pop_bytes);
    spin1_memcpy(address_list, &table_address[2 + n_master_pop_words],
            n_address_list_bytes);

    // Store the base address
    log_info("the stored synaptic matrix base address is located at: 0x%08x",
            synapse_rows_address);
    log_info("the direct synaptic matrix base address is located at: 0x%08x",
            direct_rows_address);
    synaptic_rows_base_address = (uint32_t) synapse_rows_address;
    direct_rows_base_address = (uint32_t) direct_rows_address;

    *row_max_n_words = MAX_ROW_LENGTH + N_SYNAPSE_ROW_HEADER_WORDS;

    print_master_population_table();
    return true;
}

bool population_table_get_first_address(
        spike_t spike, address_t* row_address, size_t* n_bytes_to_transfer) {
    // locate the position in the binary search / array
    log_debug("searching for key %d", spike);

    // check we don't have a complete miss
    uint32_t position;
    if (!population_table_position_in_the_master_pop_array(spike, &position)) {
        invalid_master_pop_hits++;
        log_debug("Ghost searches: %u\n", ghost_pop_table_searches);
        log_debug("spike %u (= %x): "
                "population not found in master population table",
                spike, spike);
        return false;
    }
    log_debug("position = %d", position);

    master_population_table_entry entry = master_population_table[position];
    if (entry.count == 0) {
        log_debug("spike %u (= %x): population found in master population"
                "table but count is 0", spike, spike);
    }

    last_spike = spike;
    next_item = entry.start;
    items_to_go = entry.count;
    if (entry.extra_info_flag) {
        extra_info extra = address_list[next_item++].extra;
        last_neuron_id = get_extended_neuron_id(entry, extra, spike);
    } else {
        last_neuron_id = get_neuron_id(entry, spike);
    }

    // check we have a entry in the bit field for this (possible not to due to
    // DTCM limitations or router table compression). If not, go to DMA check.
    log_debug("checking bit field");
    if (connectivity_bit_field != NULL &&
            connectivity_bit_field[position] != NULL) {
        log_debug("can be checked, bitfield is allocated");
        // check that the bit flagged for this neuron id does hit a
        // neuron here. If not return false and avoid the DMA check.
        if (!bit_field_test(
                connectivity_bit_field[position], last_neuron_id)) {
            log_debug("tested and was not set");
            bit_field_filtered_packets += 1;
            return false;
        }
        log_debug("was set, carrying on");
    } else {
        log_debug("bit_field was not set up. "
                "either its due to a lack of dtcm, or because the "
                "bitfield was merged into the routing table");
    }

    log_debug("spike = %08x, entry_index = %u, start = %u, count = %u",
            spike, position, next_item, items_to_go);

    // A local address is used here as the interface requires something
    // to be passed in but using the address of an argument is odd!
    uint32_t local_spike_id;
    bool get_next = population_table_get_next_address(
            &local_spike_id, row_address, n_bytes_to_transfer);

    // tracks surplus DMAs
    if (!get_next) {
        log_debug("found a entry which has a ghost entry for key %d", spike);
        ghost_pop_table_searches++;
    }
    return get_next;
}

bool population_table_position_in_the_master_pop_array(
        spike_t spike, uint32_t *position) {
    uint32_t imin = 0;
    uint32_t imax = master_population_table_length;

    while (imin < imax) {
        int imid = (imax + imin) >> 1;
        master_population_table_entry entry = master_population_table[imid];
        if ((spike & entry.mask) == entry.key) {
            *position = imid;
            return true;
        } else if (entry.key < spike) {

            // Entry must be in upper part of the table
            imin = imid + 1;
        } else {
            // Entry must be in lower part of the table
            imax = imid;
        }
    }
    return false;
}

bool population_table_get_next_address(
        spike_t *spike, address_t *row_address, size_t *n_bytes_to_transfer) {
    // If there are no more items in the list, return false
    if (items_to_go <= 0) {
        return false;
    }

    bool is_valid = false;
    do {
        address_and_row_length item = address_list[next_item].addr;
        if (item.address != INVALID_ADDRESS) {

            // If the row is a direct row, indicate this by specifying the
            // n_bytes_to_transfer is 0
            if (item.is_single) {
                *row_address = (address_t) (get_direct_address(item) +
                    (last_neuron_id * sizeof(uint32_t)));
                *n_bytes_to_transfer = 0;
                is_valid = true;
            } else {

                uint32_t row_length = get_row_length(item);
                uint32_t block_address = get_address(item);
                uint32_t stride = (row_length + N_SYNAPSE_ROW_HEADER_WORDS);
                uint32_t neuron_offset = last_neuron_id * stride * sizeof(uint32_t);

                *row_address = (address_t) (block_address + neuron_offset);
                *n_bytes_to_transfer = stride * sizeof(uint32_t);
                log_debug(
                    "neuron_id = %u, block_address = 0x%.8x,"
                    "row_length = %u, row_address = 0x%.8x, n_bytes = %u",
                    last_neuron_id, block_address, row_length, *row_address,
                    *n_bytes_to_transfer);
                *spike = last_spike;
                is_valid = true;
            }
        }

        next_item++;
        items_to_go--;
    } while (!is_valid && (items_to_go > 0));

    return is_valid;
}

uint32_t population_table_get_ghost_pop_table_searches(void) {
    return ghost_pop_table_searches;
}

uint32_t population_table_get_invalid_master_pop_hits(void) {
    return invalid_master_pop_hits;
}

void population_table_set_connectivity_bit_field(
        bit_field_t* connectivity_bit_fields){
    connectivity_bit_field = connectivity_bit_fields;
}

uint32_t population_table_length(void) {
    return master_population_table_length;
}

spike_t population_table_get_spike_for_index(uint32_t index) {
    return master_population_table[index].key;
}

uint32_t population_table_get_mask_for_entry(uint32_t index) {
    return master_population_table[index].mask;
}

uint32_t population_table_get_filtered_packet_count(void) {
    return bit_field_filtered_packets;
}
//! \}
