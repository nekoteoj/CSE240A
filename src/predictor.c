//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include "predictor.h"
#include <stdio.h>

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
    uint16_t global_history_table;
    uint8_t* counters;
    uint16_t counter_size;
} gshare_context_type;

typedef struct {
} tournament_context_type;

typedef struct {
} custom_context_type;

gshare_context_type gshare_context;
tournament_context_type tournament_context;
custom_context_type custom_context;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void init_gshare_predictor()
{
    uint16_t counter_size = 1;
    for (int i = 0; i < ghistoryBits; i++) {
        counter_size <<= 1;
    }
    gshare_context.global_history_table = 0;
    gshare_context.counters = (uint8_t*)malloc(sizeof(uint8_t) * counter_size);
    gshare_context.counter_size = counter_size;
}

void init_tournament_predictor() { }

void init_custom_predictor() { }

void init_predictor()
{
    //
    // TODO: Initialize Branch Predictor Data Structures
    //
    switch (bpType) {
    case GSHARE:
        init_gshare_predictor();
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
uint8_t make_gshare_prediction(uint32_t pc)
{
    uint16_t bit_mask = gshare_context.counter_size - 1;
    uint16_t pc_index = pc & bit_mask;
    uint16_t ghr_index = gshare_context.global_history_table & bit_mask;
    uint16_t index = ghr_index ^ pc_index;
    uint8_t state = gshare_context.counters[index];
    if (state == SN || state == WN) {
        return NOTTAKEN;
    }
    return TAKEN;
}

uint8_t make_tournament_prediction(uint32_t pc) { }

uint8_t make_custom_prediction(uint32_t pc) { }

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
void train_predictor(uint32_t pc, uint8_t outcome)
{
    //
    // TODO: Implement Predictor training
    // uint16_t bit_mask = gshare_context.counter_size - 1;
    uint16_t bit_mask = gshare_context.counter_size - 1;
    uint16_t pc_index = pc & bit_mask;
    uint16_t ghr = gshare_context.global_history_table;
    uint16_t ghr_index = ghr & bit_mask;
    uint16_t index = ghr_index ^ pc_index;
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
