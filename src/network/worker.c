#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#define MASTER_PORT 8080
#define BUFFER_SIZE 4096

typedef struct {
    int socket;
    bool connected;
    pthread_mutex_t mutex;
    Agent* agent;
    Environment* local_env;
} WorkerClient;

WorkerClient worker;
volatile bool running = true;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\nShutting down worker...\n");
        running = false;
    }
}

bool connect_to_master(const char* master_ip) {
    worker.socket = socket(AF_INET, SOCK_STREAM, 0);
    if (worker.socket < 0) {
        perror("socket creation failed");
        return false;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(MASTER_PORT);
    
    if (inet_pton(AF_INET, master_ip, &server_addr.sin_addr) <= 0) {
        perror("invalid address");
        close(worker.socket);
        return false;
    }
    
    if (connect(worker.socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        close(worker.socket);
        return false;
    }
    
    worker.connected = true;
    printf("Connected to master at %s:%d\n", master_ip, MASTER_PORT);
    return true;
}

void* communication_thread(void* arg) {
    char buffer[BUFFER_SIZE];
    
    while (running && worker.connected) {
        pthread_mutex_lock(&worker.mutex);
        
        snprintf(buffer, BUFFER_SIZE, "WORKER_UPDATE id:%d steps:%d reward:%.2f",
                worker.agent->id, worker.agent->steps, worker.agent->reward_sum);
        
        pthread_mutex_unlock(&worker.mutex);
        
        // Send update to master
        if (send(worker.socket, buffer, strlen(buffer), 0) < 0) {
            perror("send failed");
            worker.connected = false;
            break;
        }
        
        ssize_t bytes = recv(worker.socket, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) {
            if (bytes == 0) {
                printf("Master disconnected\n");
            } else {
                perror("recv failed");
            }
            worker.connected = false;
            break;
        }
        
        usleep(100000); // 100ms between updates
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {
    const char* master_ip = "127.0.0.1";
    if (argc > 1) {
        master_ip = argv[1];
    }
    
    // Initialize
    srand(time(NULL) ^ getpid());
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    pthread_mutex_init(&worker.mutex, NULL);
    
    // Create agent and environment
    worker.agent = agent_create(getpid(), Q_LEARNING);
    worker.local_env = env_create(10, 10);
    
    // Connect to master
    if (!connect_to_master(master_ip)) {
        printf("Failed to connect to master, running in standalone mode\n");
    }
    
    // Start communication thread if connected
    pthread_t comm_thread;
    if (worker.connected) {
        pthread_create(&comm_thread, NULL, communication_thread, NULL);
    }
    
    // Main training loop
    int episode = 0;
    while (running) {
        env_reset(worker.local_env);
        agent_reset(worker.agent);
        
        double* obs = env_get_observation(worker.local_env, worker.agent);
        bool done = false;
        
        while (!done && running) {
            int action = agent_select_action(worker.agent, obs, 2);
            
            Agent* agents[] = {worker.agent};
            int actions[] = {action};
            StepResult result = env_step(worker.local_env, agents, actions, 1);
            
            agent_update(worker.agent, obs, action, result.reward, 
                        result.observation, result.done);
            
            obs = result.observation;
            done = result.done;
            
            usleep(1000); // 1ms per step
        }
        
        episode++;
        if (episode % 100 == 0) {
            printf("Episode %d: Total reward = %.2f\n", 
                   episode, worker.agent->reward_sum);
        }
    }
    
    if (worker.connected) {
        pthread_join(comm_thread, NULL);
        close(worker.socket);
    }
    
    agent_destroy(worker.agent);
    env_destroy(worker.local_env);
    pthread_mutex_destroy(&worker.mutex);
    
    printf("Worker shutdown complete\n");
    return 0;
}
