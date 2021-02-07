#include <iostream>
#include <cstdlib>
#include <time.h>
#include <fstream>

using namespace std;

#define MINVERTICE 5000

int main(void) {

    // Init Rand Function
    srand(time(NULL));

    // Rand a Vertices and Edges
    int vertices = rand() % (6000 - MINVERTICE + 1) + MINVERTICE;
    int edges = rand() % (vertices * (vertices-1) - MINVERTICE * (MINVERTICE-1) +1) + MINVERTICE * (MINVERTICE-1);

    cout << "Generate for Vertices = " << vertices << " & Edges = " << edges << endl;

    // Open a File
    fstream graph;
    graph.open("mycase.in", ios::out  | ios::binary);

    // Write in Vertices & Edges
    graph.write(reinterpret_cast<char* >(&vertices), sizeof(int));
    graph.write(reinterpret_cast<char* >(&edges), sizeof(int));

    // Graph
    int * graphMem = new int[vertices*vertices];
    for (int i = 0; i < vertices*vertices; i++) graphMem[i] = -1;

    int src, dst, edgeDist, j=0;
    while (j < edges) {
        // Rand Out all Interger
        src = rand() % vertices;
        dst = rand() % vertices;
        edgeDist = rand() % 1001;

        // Check if exist
        if (graphMem[src*vertices + dst] == -1 && src != dst) {
            graphMem[src*vertices + dst] = edgeDist;

            graph.write(reinterpret_cast<char* >(&src), sizeof(int));
            graph.write(reinterpret_cast<char* >(&dst), sizeof(int));
            graph.write(reinterpret_cast<char* >(&edgeDist), sizeof(int));

            // cout << "Generate Set of (" << src << ", " << dst << ", " << edgeDist << ")" << endl;
            j++;
            // if (j % 1000 == 0) cout << "Generate for j = " << j << " Completed." << endl;
        } else continue;
    }

    graph.close();

    return 0;
}