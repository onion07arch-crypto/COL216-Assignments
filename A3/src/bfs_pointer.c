#include <stdlib.h>
#include <string.h>
#include "graph.h"

int bfs_pointer(Graph* g, int source, int* dist) {
    int n = g->num_vertices;

    /* initialise all distances to -1 (unvisited) */
    memset(dist, -1, (size_t)n * sizeof(int));

    /* simple circular queue backed by an array of size n */
    int* queue = (int*)malloc((size_t)n * sizeof(int));
    if (!queue) return -1;

    int head = 0, tail = 0;
    int visited = 0;

    dist[source] = 0;
    queue[tail++] = source;
    visited++;

    while (head != tail) {
        int v = queue[head++];

        for (Edge* e = g->vertices[v].head; e; e = e->next) {
            int u = e->dst;
            if (dist[u] == -1) {
                dist[u] = dist[v] + 1;
                queue[tail++] = u;
                visited++;
            }
        }
    }

    free(queue);
    return visited;
}
