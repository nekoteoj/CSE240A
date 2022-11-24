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
    uint32_t global_history_table;
    uint32_t perceptron_table_size;
    uint32_t perceptron_size;
    uint32_t weight_size;
    int32_t weight_max;
    int32_t weight_min;
    float theta;
    int32_t** perceptrons;
} perceptron_context_type;

typedef struct {
    uint32_t local_history_table_size;
    uint32_t* local_history_table;
    uint32_t perceptron_table_size;
    uint32_t perceptron_size;
    uint32_t weight_size;
    int32_t weight_max;
    int32_t weight_min;
    float theta;
    int32_t** perceptrons;
} perceptron_local_context_type;

typedef struct {
    uint8_t* choices;
    uint16_t choice_size;
} custom_context_type;

local_context_type local_context;
gshare_context_type gshare_context;
tournament_context_type tournament_context;
perceptron_context_type perceptron_context;
perceptron_local_context_type perceptron_local_context;
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

    uint32_t predictor_size = (table_size * lhistoryBits) + (counter_size * 2);
    printf("local predictor size: %d\n", predictor_size);
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

    uint32_t predictor_size = (counter_size * 2);
    printf("global predictor size: %d\n", predictor_size);
}

void init_tournament_predictor(bool xorIndex)
{
    // Init required predictors
    init_local_predictor();
    init_gshare_predictor(xorIndex);

    // Init the predictor chooser
    uint16_t choice_size = 1 << ghistoryBits;
    tournament_context.choice_size = choice_size;
    tournament_context.choices = (uint8_t*)malloc(sizeof(uint8_t) * choice_size);
    for (int i = 0; i < choice_size; i++) {
        tournament_context.choices[i] = 2;
    }

    uint32_t predictor_size = (choice_size * 2);
    printf("tournament predictor size: %d\n", predictor_size);
}

void init_perceptron_predictor()
{
    // Define local parameters
    uint32_t perceptron_table_size = 1 << pcIndexBits;
    uint32_t perceptron_size = ghistoryBits;
    uint32_t weight_size = 7;

    perceptron_context.global_history_table = 0;
    perceptron_context.perceptron_table_size = perceptron_table_size;
    perceptron_context.perceptron_size = perceptron_size;
    perceptron_context.weight_size = weight_size;
    perceptron_context.weight_max = (1 << (weight_size - 1)) - 1;
    perceptron_context.weight_min = -(perceptron_context.weight_max + 1);
    perceptron_context.theta = 1.93 * perceptron_size + 14;

    uint32_t predictor_size = (perceptron_table_size * (perceptron_size + 1) * weight_size) + perceptron_size;

    // Initialize data structures
    perceptron_context.perceptrons = (int32_t**)malloc(sizeof(int32_t*) * perceptron_table_size);
    for (int i = 0; i < perceptron_context.perceptron_table_size; i++) {
        perceptron_context.perceptrons[i] = (int32_t*)malloc(sizeof(int32_t) * (perceptron_size + 1));
        memset(perceptron_context.perceptrons[i], 0, sizeof(int32_t) * (perceptron_size + 1));
    }

    // printf("================================================================================\n");
    // printf("Using perceptron predictor:\n");
    // printf("table size: %d\n", perceptron_table_size);
    // printf("perceptron size: %d\n", perceptron_size);
    // printf("weight max: %d\n", perceptron_context.weight_max);
    // printf("weight min: %d\n", perceptron_context.weight_min);
    // printf("theta: %f\n", perceptron_context.theta);
    printf("perceptron predictor size: %d\n", predictor_size);
    // printf("================================================================================\n");
}

void init_perceptron_local_predictor()
{
    // Define local parameters
    uint32_t local_history_table_size = 1 << pcIndexBits;
    uint32_t perceptron_table_size = 1 << lhistoryBits;
    uint32_t perceptron_size = lhistoryBits;
    uint32_t weight_size = 8;

    perceptron_local_context.local_history_table_size = local_history_table_size;
    perceptron_local_context.perceptron_table_size = perceptron_table_size;
    perceptron_local_context.perceptron_size = perceptron_size;
    perceptron_local_context.weight_size = weight_size;
    perceptron_local_context.weight_max = (1 << (weight_size - 1)) - 1;
    perceptron_local_context.weight_min = -(perceptron_local_context.weight_max + 1);
    perceptron_local_context.theta = 1.93 * (perceptron_size + 1) + 14;

    uint32_t predictor_size = (perceptron_table_size * (perceptron_size + 1) * weight_size)
        + perceptron_size + (local_history_table_size * perceptron_size);

    // Initialize data structures
    perceptron_local_context.local_history_table = (uint32_t*)malloc(sizeof(uint32_t) * local_history_table_size);
    memset(perceptron_local_context.local_history_table, 0, sizeof(uint32_t) * local_history_table_size);
    perceptron_local_context.perceptrons = (int32_t**)malloc(sizeof(int32_t*) * perceptron_table_size);
    for (int i = 0; i < perceptron_local_context.perceptron_table_size; i++) {
        perceptron_local_context.perceptrons[i] = (int32_t*)malloc(sizeof(int32_t) * (perceptron_size + 1));
        memset(perceptron_local_context.perceptrons[i], 0, sizeof(int32_t) * (perceptron_size + 1));
    }

    printf("perceptron local predictor size: %d\n", predictor_size);
}

void init_custom_predictor()
{
    lhistoryBits = 8;
    pcIndexBits = 10;
    init_perceptron_local_predictor();

    ghistoryBits = 14;
    init_gshare_predictor(true);

    // Init the predictor chooser
    uint16_t choice_size = 1 << pcIndexBits;
    custom_context.choice_size = choice_size;
    custom_context.choices = (uint8_t*)malloc(sizeof(uint8_t) * choice_size);
    for (int i = 0; i < choice_size; i++) {
        custom_context.choices[i] = 3;
    }

    uint32_t predictor_size = (choice_size * 2);
    printf("custom predictor size: %d\n", predictor_size);
    printf("max storage: %d bits\n", 64 * 1024 + 256);
}

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
        init_tournament_predictor(false);
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

int32_t make_perceptron_prediction_proba(uint32_t pc)
{
    uint32_t table_bit_mask = perceptron_context.perceptron_table_size - 1;
    uint32_t table_index = (pc)&table_bit_mask;
    int32_t* weights = perceptron_context.perceptrons[table_index];

    uint32_t history = perceptron_context.global_history_table;
    int32_t pred = 0;
    for (int i = 0; i < perceptron_context.perceptron_size; i++) {
        int32_t feature = (history & 1) ? 1 : -1;
        pred += weights[i] * feature;
        history >>= 1;
    }
    pred += weights[perceptron_context.perceptron_size];
    return pred;
}

uint8_t make_perceptron_prediction(uint32_t pc)
{
    int32_t pred = make_perceptron_prediction_proba(pc);
    if (pred >= 0) {
        return TAKEN;
    }
    return NOTTAKEN;
}

int32_t make_perceptron_local_prediction_proba(uint32_t pc)
{
    uint32_t lhr_bit_mask = perceptron_local_context.local_history_table_size - 1;
    uint32_t lhr_index = pc & lhr_bit_mask;
    uint32_t table_index = perceptron_local_context.local_history_table[lhr_index];

    uint32_t table_bit_mask = perceptron_local_context.perceptron_table_size - 1;
    table_index &= table_bit_mask;
    int32_t* weights = perceptron_local_context.perceptrons[table_index];

    uint32_t history = perceptron_local_context.local_history_table[lhr_index];
    int32_t pred = 0;
    for (int i = 0; i < perceptron_local_context.perceptron_size; i++) {
        int32_t feature = (history & 1) ? 1 : -1;
        pred += weights[i] * feature;
        history >>= 1;
    }
    pred += weights[perceptron_local_context.perceptron_size];
    return pred;
}

uint8_t make_perceptron_local_prediction(uint32_t pc)
{
    int32_t pred = make_perceptron_local_prediction_proba(pc);
    if (pred >= 0) {
        return TAKEN;
    }
    return NOTTAKEN;
}

uint8_t make_custom_prediction(uint32_t pc)
{
    // return make_perceptron_prediction(pc);
    uint16_t bit_mask = custom_context.choice_size - 1;
    uint16_t pc_index = pc & bit_mask;
    uint8_t choice = custom_context.choices[pc_index];
    if (choice <= 1) {
        return make_perceptron_local_prediction(pc);
    }
    return make_gshare_prediction(pc);
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

void train_perceptron_predictor(uint32_t pc, uint8_t outcome)
{
    uint32_t table_bit_mask = perceptron_context.perceptron_table_size - 1;
    uint32_t table_index = (pc)&table_bit_mask;
    int32_t* weights = perceptron_context.perceptrons[table_index];

    uint8_t pred = make_perceptron_prediction(pc);
    uint8_t pred_proba = make_perceptron_prediction_proba(pc);
    uint32_t history = perceptron_context.global_history_table;
    if (pred != outcome || abs(pred_proba) < perceptron_context.theta) {
        for (int i = 0; i < perceptron_context.perceptron_size; i++) {
            int32_t feature = (history & 1) ? 1 : -1;
            weights[i] += (outcome * 2 - 1) * feature;
            history >>= 1;
        }
        weights[perceptron_context.perceptron_size] += outcome * 2 - 1;
        for (int i = 0; i < perceptron_context.perceptron_size + 1; i++) {
            if (weights[i] > perceptron_context.weight_max) {
                weights[i] = perceptron_context.weight_max;
            } else if (weights[i] < perceptron_context.weight_min) {
                weights[i] = perceptron_context.weight_min;
            }
        }
    }

    history = perceptron_context.global_history_table;
    history = ((history << 1) + outcome) & table_bit_mask;
    perceptron_context.global_history_table = history;
}

void train_perceptron_local_predictor(uint32_t pc, uint8_t outcome)
{
    uint32_t lhr_bit_mask = perceptron_local_context.local_history_table_size - 1;
    uint32_t lhr_index = pc & lhr_bit_mask;
    uint32_t table_index = perceptron_local_context.local_history_table[lhr_index];

    uint32_t table_bit_mask = perceptron_local_context.perceptron_table_size - 1;
    table_index &= table_bit_mask;
    int32_t* weights = perceptron_local_context.perceptrons[table_index];

    uint8_t pred = make_perceptron_local_prediction(pc);
    uint8_t pred_proba = make_perceptron_local_prediction_proba(pc);
    uint32_t history = perceptron_local_context.local_history_table[lhr_index];
    if (pred != outcome || abs(pred_proba) < perceptron_local_context.theta) {
        for (int i = 0; i < perceptron_local_context.perceptron_size; i++) {
            int32_t feature = (history & 1) ? 1 : -1;
            weights[i] += (outcome * 2 - 1) * feature;
            history >>= 1;
        }
        weights[perceptron_local_context.perceptron_size] += outcome * 2 - 1;
        for (int i = 0; i < perceptron_local_context.perceptron_size + 1; i++) {
            if (weights[i] > perceptron_local_context.weight_max) {
                weights[i] = perceptron_local_context.weight_max;
            } else if (weights[i] < perceptron_local_context.weight_min) {
                weights[i] = perceptron_local_context.weight_min;
            }
        }
    }

    history = perceptron_local_context.local_history_table[lhr_index];
    history = ((history << 1) + outcome) & table_bit_mask;
    perceptron_local_context.local_history_table[lhr_index] = history;
}

void train_custom_predictor(uint32_t pc, uint8_t outcome)
{
    // train_perceptron_predictor(pc, outcome);
    // Retrive the predictor choice
    uint16_t bit_mask = custom_context.choice_size - 1;
    uint16_t pc_index = pc & bit_mask;
    uint8_t choice = custom_context.choices[pc_index];

    // Train choice predictor
    uint8_t local_pred = make_perceptron_local_prediction(pc);
    uint8_t perceptron_pred = make_gshare_prediction(pc);

    if (local_pred == outcome && perceptron_pred != outcome && choice > 0) {
        choice--;
    } else if (local_pred != outcome && perceptron_pred == outcome && choice < 3) {
        choice++;
    }

    custom_context.choices[pc_index] = choice;

    // Train used predictors
    train_perceptron_local_predictor(pc, outcome);
    train_gshare_predictor(pc, outcome);
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
