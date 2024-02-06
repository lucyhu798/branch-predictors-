//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include "predictor.h"


//
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//

//GSHARE
int GHR; // global history register
uint32_t *bht; // branch history table

//tournament
uint32_t *bht_local; //local branch history table
uint32_t *pht; //global pattenr history table
uint32_t *bht_global; //global branch history trable
uint32_t *chooser;

//perceptron
int** PT; // perceptron table 
int* bhr; // bhr 
int threshold_max; 
int threshold_min;
int pcBits = 7;
int bhrBits = 24;
int idx;
float y; 
int p_mask; 
int size_perceptron;






//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //
  if(bpType == GSHARE) {
    init_predictor_gshare();
  } else if (bpType == TOURNAMENT){
    init_predictor_tournament();
  } else if (bpType == CUSTOM){
    init_predictor_custom();
  }

}

void init_predictor_gshare() {
  GHR = 0;
  int size = 1 << ghistoryBits;
  bht = (uint32_t*) malloc(sizeof(uint32_t)*size);
  for(int i = 0; i < size; i++){
    bht[i] = WN;
  }
}

void init_predictor_tournament(){
  GHR = 0;
  int local_size = 1 << lhistoryBits;
  int global_size = 1 << ghistoryBits;
  int pht_size = 1 << pcIndexBits;
  int chooser_size = global_size;

  bht_local = (uint32_t*) malloc(local_size * sizeof(uint32_t));
  bht_global = (uint32_t*) malloc(global_size*sizeof(uint32_t));
  pht = (uint32_t*) malloc(pht_size*sizeof(uint32_t));
  chooser = (uint32_t*) malloc(chooser_size*sizeof(uint32_t));

  for(int i = 0; i < local_size; i++) {
    bht_local[i] = WN;
  }

  for(int i = 0; i < global_size; i++) {
    bht_global[i] = WN;
  }

  for(int i = 0; i < chooser_size; i++){
    chooser[i] = WG;
  }

  for(int i = 0; i < pht_size; i++){
    pht[i] = NOTTAKEN;
  }
}

void init_predictor_custom(){
    threshold_max = pow(2, 5);
    threshold_min = -threshold_max - 1;
    
    //mask for perceptron 
    for (int i = 0; i < pcBits; i++) {
        p_mask = (p_mask << 1) + 1;
    }

    //setting up the perceptron table 
    size_perceptron = pow(2, pcBits);
    PT = (int **)malloc(size_perceptron * sizeof(int *));
    for (int i = 0; i < size_perceptron; i++) {
        PT[i] = (int *)malloc((bhrBits + 1) * sizeof(int));
        for (int j = 0; j <= bhrBits; j++) {
            PT[i][j] = 1;
        }
    }
    
    //setting up the bhr table, can only be -1 or 1 
    bhr = (int *)malloc(bhrBits * sizeof(int));
    for (int i = 0; i < bhrBits; i++) {
        bhr[i] = -1;
    }

}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return make_prediction_gshare(pc);
    case TOURNAMENT:
      return make_prediction_tournament(pc);
    case CUSTOM:
      return make_prediction_custom(pc);
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}


uint8_t make_prediction_gshare(uint32_t pc)
{
  uint32_t mask = 0;
  //create mask for as long as our history is
  for(int i = 0; i < ghistoryBits; i++){
    mask = mask << 1;
    mask++;
  }

  uint32_t idx = (pc & mask) ^ (GHR & mask);
  if(bht[idx] <= 1){
    return NOTTAKEN;
  } else {
    return TAKEN;
  }
}

uint8_t make_prediction_tournament(uint32_t pc)
{
  uint32_t pc_mask = 0;
  uint32_t pht_mask = 0;
  for(int i =0; i< lhistoryBits; i++){
    pht_mask  = pht_mask << 1;
    pht_mask++;
  }
  
  for(int i = 0; i< pcIndexBits; i++){
    pc_mask = pc_mask << 1;
    pc_mask++;
  }

  int choice = chooser[GHR];
  if(choice < 2){
    //choose global
    if(bht_global[GHR]<2){
      return NOTTAKEN;
    }else {
      return TAKEN;
    }
  } else{
    //choose local
    int pht_idx = pc_mask & pc;
    int phtRes = pht_mask & pht[pht_idx];
    if(bht_local[phtRes] < 2){
      return NOTTAKEN;
    } else{
      return TAKEN;
    }
  }


}

uint8_t make_prediction_custom(uint32_t pc)
{
    // hashing the pc 
    idx = (pc & p_mask) % size_perceptron;
    
    // this is w_0, the bias 
    y = PT[idx][bhrBits];

    //calculate the dot product of y 
    for (int i = 0; i < bhrBits; i++) {
        y += PT[idx][i] * bhr[i];
    }

    // not taken only when y is negative otherwise it is taken 
    if (y < 0) {
        return NOTTAKEN;
    }
    else {
      return TAKEN;
    }
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //
  if(bpType == GSHARE) {
    train_predictor_gshare(pc, outcome);
  } else if (bpType == TOURNAMENT){
    train_predictor_tournament(pc, outcome);
  } else if (bpType == CUSTOM){
    train_predictor_custom(pc, outcome);
  }
}


void train_predictor_gshare(uint32_t pc, uint8_t outcome) {
  uint32_t mask = 0;
  for(int i = 0; i < ghistoryBits; i++){
    mask = mask << 1;
    mask++;
  }

  uint32_t idx = (pc & mask) ^ (GHR & mask);
  if(outcome == NOTTAKEN){
    GHR = GHR << 1;
    GHR = GHR & mask;
    if(bht[idx] > 0){
      bht[idx]--;
    }
  } else if (outcome == TAKEN){
    GHR = GHR << 1;
    GHR++;
    GHR = GHR & mask;
    if(bht[idx] < 3) {
      bht[idx]++;
    }
  }
}

void train_predictor_tournament(uint32_t pc, uint8_t outcome) {
  uint32_t pc_mask = 0;
  uint32_t pht_mask = 0;
  uint32_t ghr_mask = 0;
  for(int i =0; i< lhistoryBits; i++){
    pht_mask = pht_mask << 1;
    pht_mask++;
  }
  
  for(int i = 0; i< pcIndexBits; i++){
    pc_mask = pc_mask << 1;
    pc_mask++;
  }

  for(int i = 0; i< ghistoryBits; i++){
    ghr_mask = ghr_mask << 1;
    ghr_mask++;
  }

  int phtIdx = pc & pc_mask;
  int phtRes = pht_mask & pht[phtIdx];
  int localPred = bht_local[phtRes];
  int globalPred = bht_global[GHR];
  int choice = chooser[GHR];

  if(outcome == NOTTAKEN) {
    if((localPred < 2 && globalPred > 1) || (localPred > 1 && globalPred < 2)){
      //local and global predict different things
      //favor the correct one
      if(localPred <= 1 && choice < 3){
        //local was correct
        chooser[GHR]++;
      }else{
        //global was correct
        if(choice >0){
          chooser[GHR]--;
        }
      }
    }
    if(localPred > 0){
      bht_local[phtRes]--;
    }
    if(globalPred > 0){
      bht_global[GHR]--;
    }

    pht[phtIdx] = (phtRes << 1) & pht_mask;
    GHR = (GHR << 1) & ghr_mask;
  } else {
    //outcome is taken
    if((localPred < 2 && globalPred > 1) || (localPred > 1 && globalPred < 2)){
      //predictions are different
      if(localPred > 1 && choice < 3){
        //local was correct
        chooser[GHR]++;
      }else{
        if(choice > 0){
          chooser[GHR]--;
        }
      }
    }
    if(localPred < 3){
      bht_local[phtRes]++;
    }
    if(globalPred < 3){
      bht_global[GHR]++;
    }

    pht[phtIdx] = ((phtRes << 1)+1)&pht_mask;
    GHR = ((GHR << 1)+1)&ghr_mask;
  }


}

void train_predictor_custom(uint32_t pc, uint8_t outcome){
  // get the y from the make prediction. 
  uint8_t y_out = make_prediction_custom(pc);

  int t; 
  if(outcome == TAKEN){
    t = 1; 
  }
  else{
    t= -1; 
  }
  // training algorithm just like in paper
  if (y_out != outcome || (y > threshold_min && y < threshold_max)) {
      for (int i = 0; i < bhrBits; i++) {
          PT[idx][i] += bhr[i] * t;
      }
      PT[idx][bhrBits] += t; 
  }

  //update bhr
  for (int i = bhrBits; i > 0; i--) {
      bhr[i] = bhr[i - 1];
  }
  bhr[0] = t;

}
