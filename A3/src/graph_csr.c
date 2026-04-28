#include <stdlib.h>
#include "graph.h"
CSRGraph* convert_to_csr(Graph* g) {
    if (!g) return NULL;

    int total_edges=0;
    for (int i=0; i<g->num_vertices; i++){
        Edge* curr = g->vertices[i].head;
        while (curr){
            total_edges++;
            curr = curr->next;
        }
    }

    CSRGraph* csr = (CSRGraph*)malloc(sizeof(CSRGraph));
    if (!csr) return NULL;
    csr->num_vertices = g->num_vertices;
    csr->num_edges = total_edges;

    csr->row_ptr = (int*)malloc((size_t)(g->num_vertices + 1)*sizeof(int));     // row_ptr size = V+1
    csr->col_idx = (int*)malloc((size_t)total_edges * sizeof(int));     // col_idx size = E
    
    if (!csr->row_ptr || (!csr->col_idx && total_edges>0)){
        free_csr(csr);
        return NULL;
    }

    int current_edge_idx=0;     // tracks position in col_idx array
    for (int i=0; i<g->num_vertices; i++){
        csr->row_ptr[i] = current_edge_idx;     // Vertex i's list of neighbours begins at current_edge_idx
        Edge* curr = g->vertices[i].head;
        while(curr){
            csr->col_idx[current_edge_idx] = curr->dst;
            current_edge_idx++;
            curr = curr->next;
        }
    }
    csr->row_ptr[g->num_vertices] = current_edge_idx;
    return csr;
}
void free_csr(CSRGraph* g) {
    if (!g) return;
    if (g->row_ptr) free(g->row_ptr);
    if (g->col_idx) free(g->col_idx);
    free(g);
}
