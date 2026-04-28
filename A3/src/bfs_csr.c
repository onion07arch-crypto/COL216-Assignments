#include <stdlib.h>
#include "graph.h"
int bfs_csr(CSRGraph* g, int source, int* dist) {
    if (!g || !dist || source<0 || source>=g->num_vertices){return -1;}
    for (int i=0; i<g->num_vertices; i++){
        dist[i] = -1;   // Initially mark all vertices as unvisited
    }
    int* queue = (int*)malloc((size_t)g->num_vertices * sizeof(int));
    if (!queue){
        return -1;  // in case malloc fails
    }
    int head = 0;   // next item to pop
    int tail = 0;   // next empty slot to push to
    int visited_count = 0;

    dist[source] = 0;
    queue[tail] = source;
    tail++;
    visited_count++;

    while (head<tail){
        int current_vertex = queue[head];
        head++;
        int start_edge = g->row_ptr[current_vertex];
        int end_edge = g->row_ptr[current_vertex+1];
        for (int i=start_edge; i<end_edge; i++){
            int neighbour = g->col_idx[i];
            if (dist[neighbour]==-1){
                // Neighbour is unvisited
                dist[neighbour] = dist[current_vertex]+1;
                queue[tail] = neighbour;
                tail++;
                visited_count++;
            }
        }
    }

    free(queue);
    return visited_count;

}
