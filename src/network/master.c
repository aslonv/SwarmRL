#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#define PORT 8080
#define MAX_WORKERS 100
#define BUFFER_SIZE 4096

typedef struct {
    int socket;
    int worker_id;
    pthread_t thread;
    bool active;
    pthread_mutex_t mutex;
} WorkerConnection;

typedef struct {
    WorkerConnection workers[MAX_WORKERS];
    int num_workers;
    pthread_mutex_t global_mutex;
    bool running;
} MasterServer;

MasterServer server;

void* handle_worker(void* arg) {
    WorkerConnection* worker = (WorkerConnection*)arg;
    char buffer[BUFFER_SIZE];
    
    printf("Worker %d connected\n", worker->worker_id);
    
    while (worker->active && server.running) {
        ssize_t bytes = recv(worker->socket, buffer, BUFFER_SIZE, 0);
        
        if (bytes <= 0) {
            if (bytes == 0) {
                printf("Worker %d disconnected\n", worker->worker_id);
            } else {
                perror("recv failed");
            }
            break;
        }
        
        // Process worker data
        pthread_mutex_lock(&worker->mutex);
        // TODO: Process state updates, aggregate Q-values, etc.
        pthread_mutex_unlock(&worker->mutex);
        
        // Send response
        const char* response = "ACK";
        if (send(worker->socket, response, strlen(response), 0) < 0) {
            perror("send failed");
            break;
        }
    }
    
    // Cleanup
    close(worker->socket);
    worker->active = false;
    return NULL;
}

void* accept_connections(void* arg) {
    int server_socket = *(int*)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    while (server.running) {
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        
        if (client_socket < 0) {
            if (errno != EINTR) {
                perror("accept failed");
            }
            continue;
        }
        
        pthread_mutex_lock(&server.global_mutex);
        
        // Find available slot
        int slot = -1;
        for (int i = 0; i < MAX_WORKERS; i++) {
            if (!server.workers[i].active) {
                slot = i;
                break;
            }
        }
        
        if (slot >= 0) {
            server.workers[slot].socket = client_socket;
            server.workers[slot].worker_id = slot;
            server.workers[slot].active = true;
            
            if (pthread_create(&server.workers[slot].thread, NULL, 
                             handle_worker, &server.workers[slot]) != 0) {
                perror("pthread_create failed");
                close(client_socket);
                server.workers[slot].active = false;
            } else {
                server.num_workers++;
            }
        } else {
            printf("Max workers reached, rejecting connection\n");
            close(client_socket);
        }
        
        pthread_mutex_unlock(&server.global_mutex);
    }
    
    return NULL;
}

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\nShutting down server...\n");
        server.running = false;
    }
}

int main() {
    // Initialize server
    memset(&server, 0, sizeof(server));
    server.running = true;
    pthread_mutex_init(&server.global_mutex, NULL);
    
    for (int i = 0; i < MAX_WORKERS; i++) {
        pthread_mutex_init(&server.workers[i].mutex, NULL);
    }
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket creation failed");
        return 1;
    }
    
    // Allow socket reuse
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_socket);
        return 1;
    }
    
    // Bind
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_socket);
        return 1;
    }
    
    // Listen
    if (listen(server_socket, 10) < 0) {
        perror("listen failed");
        close(server_socket);
        return 1;
    }
    
    printf("Master server listening on port %d\n", PORT);
    
    // Accept connections in separate thread
    pthread_t accept_thread;
    pthread_create(&accept_thread, NULL, accept_connections, &server_socket);
    
    // Main loop
    while (server.running) {
        sleep(1);
        
        // Periodic tasks
        pthread_mutex_lock(&server.global_mutex);
        // TODO: Global optimization, synchronization, etc.
        pthread_mutex_unlock(&server.global_mutex);
    }
    
    // Cleanup
    for (int i = 0; i < MAX_WORKERS; i++) {
        if (server.workers[i].active) {
            server.workers[i].active = false;
            pthread_join(server.workers[i].thread, NULL);
        }
        pthread_mutex_destroy(&server.workers[i].mutex);
    }
    
    pthread_join(accept_thread, NULL);
    close(server_socket);
    pthread_mutex_destroy(&server.global_mutex);
    
    printf("Server shutdown complete\n");
    return 0;
}
