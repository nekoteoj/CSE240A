//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include "predictor.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

//
// TODO:Student Information
//
const char* studentName = "Pisit Wajanasara";
const char* studentID = "A59009987";
const char* email = "pwajanasara@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char* bpName[4] = { "Static", "Gshare", "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits; // Number of bits used for PC index
int bpType; // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//

typedef struct {
    uint16_t* local_history_tables;
    uint16_t table_size;
    uint8_t* counters;
    uint16_t counter_size;
} local_context_type;

typedef struct {
    uint16_t global_history_table;
    uint8_t* counters;
    uint16_t counter_size;
    bool xorIndex;
} gshare_context_type;

typedef struct {
    uint8_t* choices;
    uint16_t choice_size;
} tournament_context_type;

typedef struct {
} custom_context_type;

local_context_type local_context;
gshare_context_type gshare_context;
tournament_context_type tournament_context;
custom_context_type custom_context;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void init_local_predictor()
{
    // Initialize history tables
    uint16_t table_size = 1 << pcIndexBits;
    local_context.table_size = table_size;
    local_context.local_history_tables = (uint16_t*)malloc(sizeof(uint16_t) * table_size);
    memset(local_context.local_history_tables, 0, sizeof(uint16_t) * table_size);

    // Initialize counter
    uint16_t counter_size = 1 << lhistoryBits;
    local_context.counter_size = counter_size;
    local_context.counters = (uint8_t*)malloc(sizeof(uint8_t) * counter_size);
    for (int i = 0; i < counter_size; i++) {
        local_context.counters[i] = WN;
    }
}

void init_gshare_predictor(bool xorIndex)
{
    uint16_t counter_size = 1 << ghistoryBits;
    gshare_context.global_history_table = 0;
    gshare_context.counters = (uint8_t*)malloc(sizeof(uint8_t) * counter_size);
    gshare_context.counter_size = counter_size;
    gshare_context.xorIndex = xorIndex;
    for (int i = 0; i < counter_size; i++) {
        gshare_context.counters[i] = WN;
    }
}

void init_tournament_predictor()
{
    // Init required predictors
    init_local_predictor();
    init_gshare_predictor(false);

    // Init the predictor chooser
    uint16_t choice_size = 1 << ghistoryBits;
    tournament_context.choice_size = choice_size;
    tournament_context.choices = (uint8_t*)malloc(sizeof(uint8_t) * choice_size);
    for (int i = 0; i < choice_size; i++) {
        tournament_context.choices[i] = 2;
    }
}

void init_custom_predictor() { }

void init_predictor()
{
    //
    // TODO: Initialize Branch Predictor Data Structures
    //
    switch (bpType) {
    case GSHARE:
        init_gshare_predictor(true);
        return;
    case TOURNAMENT:
        init_tournament_predictor();
        return;
    case CUSTOM:
        init_custom_predictor();
        return;
    }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t make_local_prediction(uint32_t pc)
{
    // Retrive history
    uint16_t pc_bit_mask = local_context.table_size - 1;
    uint16_t pc_index = pc & pc_bit_mask;
    uint16_t history = local_context.local_history_tables[pc_index];

    // Retrive state
    uint8_t state = local_context.counters[history];

    // Make prediction
    if (state == SN || state == WN) {
        return NOTTAKEN;
    }
    return TAKEN;
}

uint8_t make_gshare_prediction(uint32_t pc)
{
    uint16_t bit_mask = gshare_context.counter_size - 1;
    uint16_t ghr_index = gshare_context.global_history_table & bit_mask;
    uint16_t index = ghr_index;
    if (gshare_context.xorIndex) {
        uint16_t pc_index = pc & bit_mask;
        index ^= pc_index;
    }
    uint8_t state = gshare_context.counters[index];
    if (state == SN || state == WN) {
        return NOTTAKEN;
    }
    return TAKEN;
}

uint8_t make_tournament_prediction(uint32_t pc)
{
    uint16_t bit_mask = tournament_context.choice_size - 1;
    uint16_t ghr_index = gshare_context.global_history_table & bit_mask;
    uint8_t choice = tournament_context.choices[ghr_index];
    if (choice <= 1) {
        return make_local_prediction(pc);
    }
    return make_gshare_prediction(pc);
}

uint8_t make_custom_prediction(uint32_t pc)
{
}

uint8_t make_prediction(uint32_t pc)
{
    //
    // TODO: Implement prediction scheme
    //

    // Make a prediction based on the bpType
    switch (bpType) {
    case STATIC:
        return TAKEN;
    case GSHARE:
        return make_gshare_prediction(pc);
    case TOURNAMENT:
        return make_tournament_prediction(pc);
    case CUSTOM:
        return make_custom_prediction(pc);
    default:
        break;
    }

    // If there is not a compatable bpType then return NOTTAKEN
    return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void train_local_predictor(uint32_t pc, uint8_t outcome)
{
    // Retrive history
    uint16_t pc_bit_mask = local_context.table_size - 1;
    uint16_t pc_index = pc & pc_bit_mask;
    uint16_t history = local_context.local_history_tables[pc_index];

    // Update state
    uint8_t state = local_context.counters[history];
    if (outcome == TAKEN && state != ST) {
        state++;
    } else if (outcome == NOTTAKEN && state != SN) {
        state--;
    }
    local_context.counters[history] = state;

    // Update history
    uint16_t history_bit_mask = local_context.counter_size - 1;
    history = ((history << 1) + outcome) & history_bit_mask;
    local_context.local_history_tables[pc_index] = history;
}

void train_gshare_predictor(uint32_t pc, uint8_t outcome)
{
    uint16_t bit_mask = gshare_context.counter_size - 1;
    uint16_t ghr = gshare_context.global_history_table;
    uint16_t ghr_index = ghr & bit_mask;
    uint16_t index = ghr_index;
    if (gshare_context.xorIndex) {
        uint16_t pc_index = pc & bit_mask;
        index ^= pc_index;
    }
    uint8_t state = gshare_context.counters[index];

    // Update state in the counter
    if (outcome == TAKEN && state != ST) {
        state++;
    } else if (outcome == NOTTAKEN && state != SN) {
        state--;
    }
    gshare_context.counters[index] = state;

    // Update global history register
    ghr = ((ghr << 1) + outcome) & bit_mask;
    gshare_context.global_history_table = ghr;
}

void train_tournament_predictor(uint32_t pc, uint8_t outcome)
{
    // Retrive the predictor choice
    uint16_t bit_mask = tournament_context.choice_size - 1;
    uint16_t ghr_index = gshare_context.global_history_table & bit_mask;
    uint8_t choice = tournament_context.choices[ghr_index];

    // Train choice predictor
    uint8_t local_pred = make_local_prediction(pc);
    uint8_t gshare_pred = make_gshare_prediction(pc);

    if (local_pred == outcome && gshare_pred != outcome && choice > 0) {
        choice--;
    } else if (local_pred != outcome && gshare_pred == outcome && choice < 3) {
        choice++;
    }

    tournament_context.choices[ghr_index] = choice;

    // Train used predictors
    train_local_predictor(pc, outcome);
    train_gshare_predictor(pc, outcome);
}

void train_custom_predictor(uint32_t pc, uint8_t outcome)
{
}

void train_predictor(uint32_t pc, uint8_t outcome)
{
    //
    // TODO: Implement Predictor training
    // uint16_t bit_mask = gshare_context.counter_size - 1;
    switch (bpType) {
    case GSHARE:
        train_gshare_predictor(pc, outcome);
        return;
    case TOURNAMENT:
        train_tournament_predictor(pc, outcome);
        return;
    case CUSTOM:
        train_custom_predictor(pc, outcome);
        return;
    }
}
