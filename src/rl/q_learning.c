#include <stdlib.h>
#include <string.h>
#include <math.h>

#define STATE_BINS 10
#define ACTION_SPACE 4
#define LEARNING_RATE 0.1
#define DISCOUNT_FACTOR 0.95
#define EPSILON 0.1

typedef struct {
    double q_table[STATE_BINS][STATE_BINS][ACTION_SPACE];
    double epsilon;
    double alpha;
    double gamma;
} QLearning;

void* q_learning_init() {
    QLearning* ql = (QLearning*)calloc(1, sizeof(QLearning));
    if (!ql) return NULL;
    
    ql->epsilon = EPSILON;
    ql->alpha = LEARNING_RATE;
    ql->gamma = DISCOUNT_FACTOR;
    
    // Initialize Q-table with small random values
    for (int i = 0; i < STATE_BINS; i++) {
        for (int j = 0; j < STATE_BINS; j++) {
            for (int k = 0; k < ACTION_SPACE; k++) {
                ql->q_table[i][j][k] = ((double)rand() / RAND_MAX) * 0.01;
            }
        }
    }
    
    return ql;
}

void q_learning_destroy(void* rl_data) {
    free(rl_data);
}

static void discretize_state(double* obs, int* state_x, int* state_y) {
    // Simple discretization - customize based on your needs
    *state_x = (int)(obs[0] * STATE_BINS / 10.0);
    *state_y = (int)(obs[1] * STATE_BINS / 10.0);
    
    // Bounds checking
    *state_x = (*state_x < 0) ? 0 : (*state_x >= STATE_BINS) ? STATE_BINS - 1 : *state_x;
    *state_y = (*state_y < 0) ? 0 : (*state_y >= STATE_BINS) ? STATE_BINS - 1 : *state_y;
}

int q_learning_select_action(void* rl_data, double* observation, int obs_dim) {
    QLearning* ql = (QLearning*)rl_data;
    if (!ql || !observation || obs_dim < 2) return 0;
    
    // Epsilon-greedy policy
    if ((double)rand() / RAND_MAX < ql->epsilon) {
        return rand() % ACTION_SPACE;
    }
    
    // Get discretized state
    int state_x, state_y;
    discretize_state(observation, &state_x, &state_y);
    
    // Find best action
    int best_action = 0;
    double best_value = ql->q_table[state_x][state_y][0];
    
    for (int a = 1; a < ACTION_SPACE; a++) {
        if (ql->q_table[state_x][state_y][a] > best_value) {
            best_value = ql->q_table[state_x][state_y][a];
            best_action = a;
        }
    }
    
    return best_action;
}

void q_learning_update(void* rl_data, double* obs, int action, double reward,
                      double* next_obs, bool done) {
    QLearning* ql = (QLearning*)rl_data;
    if (!ql || !obs || !next_obs) return;
    
    int state_x, state_y, next_state_x, next_state_y;
    discretize_state(obs, &state_x, &state_y);
    discretize_state(next_obs, &next_state_x, &next_state_y);
    
    // Find max Q-value for next state
    double max_next_q = ql->q_table[next_state_x][next_state_y][0];
    for (int a = 1; a < ACTION_SPACE; a++) {
        if (ql->q_table[next_state_x][next_state_y][a] > max_next_q) {
            max_next_q = ql->q_table[next_state_x][next_state_y][a];
        }
    }
    
    // Q-learning update rule
    double current_q = ql->q_table[state_x][state_y][action];
    double target = done ? reward : reward + ql->gamma * max_next_q;
    ql->q_table[state_x][state_y][action] = 
        current_q + ql->alpha * (target - current_q);
}
