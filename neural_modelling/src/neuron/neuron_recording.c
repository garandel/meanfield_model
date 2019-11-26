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

#include "neuron_recording.h"
#include <bit_field.h>
#include <stddef.h>

//! The index to record each variable to for each neuron
uint8_t **neuron_recording_indexes;

//! The index to record each bitfield variable to for each neuron
uint8_t **bitfield_recording_indexes;

//! The number of variables that *can* be recorded - might not be enabled
uint32_t n_recorded_vars;

//! The number of bitfield variables that can be recorded
uint32_t n_bitfield_vars;

recording_info_t *recording_info;

bitfield_info_t *bitfield_info;

uint8_t **recording_values;

uint32_t **bitfield_values;

//! The number of recordings outstanding
uint32_t n_recordings_outstanding = 0;

//! The address of the recording region to read on reset
static void *reset_address;

//! \brief resets all states back to start state.
static void reset_record_counter(void) {
    for (uint32_t i = 0; i < n_recorded_vars; i++) {
        if (recording_info[i].rate == 0) {
            // Setting increment to zero means count will never equal rate
            recording_info[i].increment = 0;

            // Count is not rate so does not record
            recording_info[i].count = 1;
        } else {
            // Increase one each call so count gets to rate
            recording_info[i].increment = 1;

            // Using rate here so that the zero time is recorded
            recording_info[i].count = recording_info[i].rate;
        }
    }

    // clear the bitfields
    for (uint32_t i = 0; i < n_bitfield_vars; i++) {
        if (bitfield_info[i].rate == 0) {
            // Setting increment to zero means count will never equal rate
            bitfield_info[i].increment = 0;

            // Count is not rate so does not record
            bitfield_info[i].count = 1;
        } else {
            // Increase one each call so count gets to rate
            bitfield_info[i].increment = 1;

            // Using rate here so that the zero time is recorded
            bitfield_info[i].count = bitfield_info[i].rate;
            clear_bit_field(bitfield_info[i].values->bits,
                    bitfield_info[i].n_words);
        }
    }
}

//! \brief wrapper to recording finalise
void neuron_recording_finalise(void){
    recording_finalise();
}

//! \brief the number of bytes used in bitfield recording for n_neurons
static inline uint32_t bitfield_data_size(uint32_t n_neurons) {
    return sizeof(bitfield_values_t) + (get_bit_field_size(n_neurons) * sizeof(uint32_t));
}

//! \brief reads recording data from sdram
//! \param[in] recording_address: sdram location for the recording data
//! \param[in] n_neurons: the number of neurons to setup for
//! \return bool stating if the read was successful or not
static bool neuron_recording_read_in_elements(
        void *data_address, uint32_t n_neurons) {

    // Round up the number of bytes to align at a word boundary i.e. round to
    // the next multiple of 4
    uint32_t ceil_n_entries = (n_neurons + 3) & 0xFFFFFFFC;

    // GCC lets you define a struct like this!
    typedef struct neuron_recording_data {
        uint32_t rate;
        uint32_t n_neurons_recording;
        uint32_t element_size;
        uint8_t indices[ceil_n_entries];
    } neuron_recording_data_t;

    neuron_recording_data_t *data = data_address;

    for (uint32_t i = 0; i < n_recorded_vars; i++) {
        recording_info[i].rate = data[i].rate;
        uint32_t n_neurons_rec = data[i].n_neurons_recording;
        recording_info[i].element_size = data[i].element_size;
        recording_info[i].size = sizeof(recording_values_t)
                + (n_neurons_rec * recording_info[i].element_size);
        uint32_t alloc_size = recording_info[i].size +
                recording_info[i].element_size;

        // allocate memory for the recording
        if (recording_info[i].values == NULL) {
            recording_info[i].values = spin1_malloc(alloc_size);
            if (recording_info[i].values == NULL) {
                log_error("couldn't allocate recording data space for %d", i);
                return false;
            }
            recording_values[i] = recording_info[i].values->data;
        }

        // copy over the indexes
        spin1_memcpy(neuron_recording_indexes[i], data[i].indices,
            n_neurons * sizeof(uint8_t));
    }

    typedef struct bitfield_recording_data {
        uint32_t rate;
        uint32_t n_neurons_recording;
        uint8_t indices[ceil_n_entries];
    } bitfield_recording_data_t;

    bitfield_recording_data_t *bitfield_data =
            (bitfield_recording_data_t *) &data[n_recorded_vars];

    for (uint32_t i = 0; i < n_bitfield_vars; i++) {
        bitfield_info[i].rate = bitfield_data[i].rate;
        if (bitfield_info[i].rate == 0) {
            continue;
        }
        uint32_t n_neurons_rec = bitfield_data[i].n_neurons_recording;
        bitfield_info[i].size = bitfield_data_size(n_neurons_rec);
        uint32_t alloc_size = bitfield_data_size(n_neurons_rec + 1);

        // allocate memory for the recording
        if (bitfield_info[i].values == NULL) {
            bitfield_info[i].values = spin1_malloc(alloc_size);
            if (bitfield_info[i].values == NULL) {
                log_error("couldn't allocate bitfield recording data space for %d", i);
                return false;
            }
            bitfield_info[i].n_words = get_bit_field_size(n_neurons_rec + 1);
            bitfield_values[i] = bitfield_info[i].values->bits;
        }

        // copy over the indexes
        spin1_memcpy(bitfield_recording_indexes[i], bitfield_data[i].indices,
            n_neurons * sizeof(uint8_t));
    }
    return true;
}

bool neuron_recording_reset(uint32_t n_neurons) {
    recording_reset();
    bool success = neuron_recording_read_in_elements(reset_address, n_neurons);
    if (!success){
        log_error("failed to reread in the new elements after reset");
        return false;
    }
    return true;
}

//! \brief handles all the dtcm allocations
//! \param[in] n_neurons: how many neurons to set dtcm for
static inline bool allocate_dtcm(uint32_t n_neurons) {

    recording_info = spin1_malloc(n_recorded_vars * sizeof(recording_info_t));
    if (recording_info == NULL) {
        log_error("Could not allocated space for recording_info");
        return false;
    }

    // allocate dtcm for the overall holder for indexes
    neuron_recording_indexes = spin1_malloc(n_recorded_vars * sizeof(uint8_t *));
    if (neuron_recording_indexes == NULL) {
        log_error("Could not allocate space for var_recording_indexes");
        return false;
    }

    recording_values = spin1_malloc(n_recorded_vars * sizeof(uint8_t *));
    if (recording_values == NULL) {
        log_error("Could not allocate space for recording_values");
        return false;
    }

    for (uint32_t i = 0; i < n_recorded_vars; i++) {

        // clear recorded values pointer
        recording_info[i].values = NULL;

        // allocate dtcm for indexes for each recording region
        neuron_recording_indexes[i] = spin1_malloc(n_neurons * sizeof(uint8_t));
        if (neuron_recording_indexes[i] == NULL){
            log_error("failed to allocate memory for recording index %d", i);
            return false;
        }
    }

    // successfully allocated all dtcm.
    return true;
}

//! \brief handles all the dtcm allocations
//! \param[in] n_neurons: how many neurons to set dtcm for
static inline bool allocate_bitfield_dtcm(uint32_t n_neurons) {

    bitfield_info = spin1_malloc(n_bitfield_vars * sizeof(bitfield_info_t));
    if (bitfield_info == NULL) {
        log_error("Failed to allocate space for bitfield_info");
        return false;
    }

    // allocate dtcm for the overall holder for indexes
    bitfield_recording_indexes = spin1_malloc(n_bitfield_vars * sizeof(uint8_t *));
    if (bitfield_recording_indexes == NULL) {
        log_error("Could not allocate space for bitfield_recording_indexes");
        return false;
    }

    bitfield_values = spin1_malloc(n_bitfield_vars * sizeof(uint32_t *));
    if (bitfield_values == NULL) {
        log_error("Could not allocate space for bitfield_values");
        return false;
    }

    for (uint32_t i = 0; i < n_bitfield_vars; i++) {

        // clear recorded values pointer
        bitfield_info[i].values = NULL;

        // allocate dtcm for indexes for each recording region
        bitfield_recording_indexes[i] = spin1_malloc(n_neurons * sizeof(uint8_t));
        if (bitfield_recording_indexes[i] == NULL){
            log_error("failed to allocate memory for bitfield index %d", i);
            return false;
        }
    }

    // successfully allocated all dtcm.
    return true;
}

typedef struct neuron_recording_header {
    uint32_t n_recorded_vars;
    uint32_t n_bitfield_vars;
} neuron_recording_header_t;

bool neuron_recording_initialise(
        void *recording_address, uint32_t *recording_flags,
        uint32_t n_neurons) {

    // boot up the basic recording
    void *data_addr = recording_address;
    bool success = recording_initialize(&data_addr, recording_flags);
    if (!success) {
        log_error("failed to init basic recording.");
        return false;
    }

    // read in the n neuron recording elements
    neuron_recording_header_t *header = data_addr;
    n_recorded_vars = header->n_recorded_vars;
    n_bitfield_vars = header->n_bitfield_vars;
    data_addr = &header[1];
    log_info("Recording %d variables and %d bitfield variables",
            n_recorded_vars, n_bitfield_vars);

    bool dtcm_allocated = allocate_dtcm(n_neurons);
    if (!dtcm_allocated) {
        log_error("failed to allocate dtcm for the neuron recording structs.");
        return false;
    }
    dtcm_allocated = allocate_bitfield_dtcm(n_neurons);
    if (!dtcm_allocated) {
        log_error("failed to allocate dtcm for the bitfield recording structs");
        return false;
    }

    // read in the sdram params into the allocated data objects
    reset_address = data_addr;
    if (!neuron_recording_read_in_elements(data_addr, n_neurons)){
        log_error("failed to read in the elements");
        return false;
    }

    // reset stuff
    reset_record_counter();

    return true;
}
