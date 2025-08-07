#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "swarmrl/agent.h"
#include "swarmrl/environment.h"

#define NUM_AGENTS 5
#define NUM_EPISODES 1000

int main() {
    printf("SwarmRL Simple Swarm Example\n");
    printf("============================\n\n");
    
    srand(time(NULL));
    
    Environment* env = env_create(20, 20);
    if (!env) {
        fprintf(stderr, "Failed to create environment\n");
        return 1;
    }
    
    Agent* agents[NUM_AGENTS];
    for (int i = 0; i < NUM_AGENTS; i++) {
        agents[i] = agent_create(i, Q_LEARNING);
        if (!agents[i]) {
            fprintf(stderr, "Failed to create agent %d\n", i);
            return 1;
        }
    }
    
    printf("Created %d agents in %dx%d environment\n\n", 
           NUM_AGENTS, env->width, env->height);
    
    // Training loop
    for (int episode = 0; episode < NUM_EPISODES; episode++) {
        env_reset(env);
        
        for (int i = 0; i < NUM_AGENTS; i++) {
            agent_reset(agents[i]);
        }
        
        double episode_reward = 0;
        int steps = 0;
        bool done = false;
        
        while (!done && steps < 1000) {
            int actions[NUM_AGENTS];
            double* observations[NUM_AGENTS];
            
            for (int i = 0; i < NUM_AGENTS; i++) {
                observations[i] = env_get_observation(env, agents[i]);
                actions[i] = agent_select_action(agents[i], observations[i], 3);
            }
       
            StepResult result = env_step(env, agents, actions, NUM_AGENTS);
     
            for (int i = 0; i < NUM_AGENTS; i++) {
                double* next_obs = env_get_observation(env, agents[i]);
                agent_update(agents[i], observations[i], actions[i], 
                           result.reward, next_obs, result.done);
            }
            
            episode_reward += result.reward;
            done = result.done;
            steps++;
        }

        if ((episode + 1) % 100 == 0) {
            printf("Episode %4d: Steps=%4d, Reward=%.2f\n", 
                   episode + 1, steps, episode_reward);
  
            printf("  Agent positions: ");
            for (int i = 0; i < NUM_AGENTS; i++) {
                printf("A%d(%.1f,%.1f) ", i, 
                       agents[i]->state.x, agents[i]->state.y);
            }
            printf("\n");
        }
    }
    
    printf("\nTraining complete!\n");

    for (int i = 0; i < NUM_AGENTS; i++) {
        agent_destroy(agents[i]);
    }
    env_destroy(env);
    
    return 0;
}
