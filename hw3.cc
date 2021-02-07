#include <iostream>
#include <fstream>
#include <cassert>
#include <omp.h>
#include <cmath>

using namespace std;

// Input Spec
// argv[1] -> InputFileName
// argv[2] -> OutputFileName

// Share Resources - Dist Data Structure
// Time Complexity O(n^3)

// Blocked Version of APSP algorithm
#define BLOCK_SIZE 128
#define DIST_SIZE 6016 // 128 * 47

void inplaceChange(int (*C)[DIST_SIZE], int(*A)[DIST_SIZE], int(*B)[DIST_SIZE], int kiLimit);

int main(int argc, char** argv) {
    
    assert(argc == 3);

    // Get Threads' Count
    cpu_set_t cpu_set;
    sched_getaffinity(0, sizeof(cpu_set), &cpu_set);
    int ncpus = CPU_COUNT(&cpu_set);

    // Vertices, Edges
    int vertices, edges;

    // Input Buffer
    int src, dst, edgeDist;
    fstream graph;

    // Open File and Read in Setting
    graph.open(argv[1], ios::in | ios::binary);
    graph.read(reinterpret_cast<char *>(&vertices), sizeof(int));
    graph.read(reinterpret_cast<char *>(&edges), sizeof(int));

    // Calculate Blocked Settings
    const int BLOCK_DIMENSION = ceil((double)vertices / BLOCK_SIZE);
    
    // Create Data Structure to Hold up all Dist
    int (*dist)[DIST_SIZE] = reinterpret_cast<int (*)[DIST_SIZE]>(new int[DIST_SIZE * DIST_SIZE]);

    // cout << "Vertices = " << vertices << " Edges = " << edges << " Block Dimension = " << BLOCK_DIMENSION << endl;
    
    // Manually Set Up Whole Dist Array
    for (int i = 0; i < vertices; i++) {
        for (int j = 0; j < vertices; j++) {
            if (i != j) dist[i][j] = 1073741823; // 2^30 - 1
            else dist[i][j] = 0;
        }
    }

    // Setup Initial Dist Buffer
    int buf[12];
    int inCounter = 0;
    if (edges>=4) {
        do {
            graph.read(reinterpret_cast<char *>(&buf), sizeof(buf));

            // Unroll
            dist[buf[0]][buf[1]] = buf[2];
            dist[buf[3]][buf[4]] = buf[5];
            dist[buf[6]][buf[7]] = buf[8];
            dist[buf[9]][buf[10]] = buf[11];
            inCounter+=4;
        } while (inCounter < edges);
    }

    while(inCounter++ < edges && graph.read(reinterpret_cast<char *>(&buf), sizeof(int) * 3)) {
        // graph.read(reinterpret_cast<char *>(&dst), sizeof(int));
        // graph.read(reinterpret_cast<char *>(&edgeDist), sizeof(int));

        // cout << src << " -> " << dst << ": " << edgeDist << endl;

        dist[buf[0]][buf[1]] = buf[2];
    }
    graph.close();

    // omp_lock_t posLock;
    // omp_init_lock(&posLock);

    // Algorithm Main Part
    // Updated Version - Block Version
#pragma omp parallel num_threads(ncpus) shared(dist)
{
    // Thread Private Variable
    int kiLimit, kiLimitt;

    // Non Parallable Part of Algorithm
    for (int k = 0; k < vertices; k+=BLOCK_SIZE) {

        // Phase1 -> Compute Diagonal Block
        // Single Thread Computation (Can be Optimized?)
        // * "ki" for k-inner
        // kiLimit = k+BLOCK_SIZE > vertices ? vertices : k+BLOCK_SIZE;
        kiLimitt = k+BLOCK_SIZE > vertices ? vertices-k : BLOCK_SIZE;

#pragma omp single
{
        // Check Boundrary
        // jB = iB = k+BLOCK_SIZE > vertices ? vertices : k + BLOCK_SIZE;
        // jB = iB = k+BLOCK_SIZE > vertices ? vertices-k : BLOCK_SIZE;

        // cout << "k = " << k << endl;

        // Compute Self Dependent Block
        // for (int ki = k; ki < kiLimit; ki++) {
        //     // cout << "ki = " << ki << endl;
        //     for (int i = k; i < iB; i++) {
        //         for (int j = k; j < jB; j++) {
        //             if ((dist[i][ki] + dist[ki][j]) < dist[i][j])
        //                 dist[i][j] = dist[i][ki] + dist[ki][j];
        //             // cout << dist[i][j] << " ";
        //         }
        //         // cout << endl;
        //     }
        //     // cout << endl;
        // }
        inplaceChange(
            reinterpret_cast<int (*)[DIST_SIZE]>(&dist[k][k]),
            reinterpret_cast<int (*)[DIST_SIZE]>(&dist[k][k]),
            reinterpret_cast<int (*)[DIST_SIZE]>(&dist[k][k]),
            kiLimitt
        );

}
        // Phase2 -> Compute Row and Column
        // Same Row
        // iB = k+BLOCK_SIZE > vertices ? vertices : k+BLOCK_SIZE;
        // iB = k+BLOCK_SIZE > vertices ? vertices-k : BLOCK_SIZE;
#pragma omp for schedule(guided) nowait
        for (int bj = 0; bj < BLOCK_DIMENSION; bj++) {
            if (bj*BLOCK_SIZE == k) continue;

            // Check Boundrary
            // jB = (bj+1)*BLOCK_SIZE > vertices ? vertices : (bj+1)*BLOCK_SIZE;
            // jB = (bj+1)*BLOCK_SIZE > vertices ? vertices-bj*BLOCK_SIZE : BLOCK_SIZE;

            // Computation
            // for (int ki = k; ki < kiLimit; ki++) {
            //     for (int i = k; i < iB; i++) {
            //         for (int j = bj*BLOCK_SIZE; j < jB; j++) {
            //             if ((dist[i][ki] + dist[ki][j]) < dist[i][j])
            //                 dist[i][j] = dist[i][ki] + dist[ki][j];
            //         }
            //     }
            // }

            inplaceChange(
                reinterpret_cast<int (*)[DIST_SIZE]>(&dist[k][bj*BLOCK_SIZE]),
                reinterpret_cast<int (*)[DIST_SIZE]>(&dist[k][k]),
                reinterpret_cast<int (*)[DIST_SIZE]>(&dist[k][bj*BLOCK_SIZE]),
                kiLimitt
            );
        }

        // Same Column
        // jB = k+BLOCK_SIZE > vertices ? vertices : k+BLOCK_SIZE;
        // jB = k+BLOCK_SIZE > vertices ? vertices-k : BLOCK_SIZE;
#pragma omp for schedule(guided)
        for (int bi = 0; bi < BLOCK_DIMENSION; bi++) {
            if (bi * BLOCK_SIZE == k) continue;

            // Check Boundrary
            // iB = (bi+1)*BLOCK_SIZE > vertices ? vertices : (bi+1)*BLOCK_SIZE;
            // iB = (bi+1)*BLOCK_SIZE > vertices ? vertices-bi*BLOCK_SIZE : BLOCK_SIZE;

            // Computation
            // for (int ki = k; ki < kiLimit; ki++) {
            //     for (int i = bi*BLOCK_SIZE; i < iB; i++) {
            //         for (int j = k; j < jB; j++) {
            //             if ((dist[i][ki] + dist[ki][j]) < dist[i][j])
            //                 dist[i][j] = dist[i][ki] + dist[ki][j];
            //         }
            //     }
            // }
            inplaceChange(
                reinterpret_cast<int (*)[DIST_SIZE]>(&dist[bi*BLOCK_SIZE][k]),
                reinterpret_cast<int (*)[DIST_SIZE]>(&dist[bi*BLOCK_SIZE][k]),
                reinterpret_cast<int (*)[DIST_SIZE]>(&dist[k][k]),
                kiLimitt
            );
        }

        // Phase3 -> All Remaining Independent Calculation
        // i, j != k (Don't Calculate Repeated Part)
#pragma omp for schedule(guided) collapse(2)
        for (int bi = 0; bi < BLOCK_DIMENSION; bi++) {
            for (int bj= 0; bj < BLOCK_DIMENSION; bj++) {
                // Avoid Repeated Calculation
                if (bi*BLOCK_SIZE == k || bj*BLOCK_SIZE == k) continue;

                // omp_set_lock(&posLock);
                // cout << "bi = " << bi << " bj = " << bj << " Proceed." << endl;

                // Check Boundrary
                // iB = (bi+1)*BLOCK_SIZE > vertices ? vertices: (bi+1)*BLOCK_SIZE;
                // jB = (bj+1)*BLOCK_SIZE > vertices ? vertices: (bj+1)*BLOCK_SIZE;
                // iB = (bi+1)*BLOCK_SIZE > vertices ? vertices-bi*BLOCK_SIZE: BLOCK_SIZE;
                // jB = (bj+1)*BLOCK_SIZE > vertices ? vertices-bj*BLOCK_SIZE: BLOCK_SIZE;

                // cout << "With iB = " << iB << " jB = " << jB << endl;

                // omp_unset_lock(&posLock);

                // Computation
                // for (int ki = k; ki < kiLimit; ki++) {
                //     for (int i = bi * BLOCK_SIZE; i < iB; i++) {
                //         for (int j = bj * BLOCK_SIZE; j < jB; j++) {
                //             if ((dist[i][ki] + dist[ki][j]) < dist[i][j])
                //                 dist[i][j] = dist[i][ki] + dist[ki][j];
                //         }
                //     }
                // }
                inplaceChange(
                    reinterpret_cast<int (*)[DIST_SIZE]>(&dist[bi*BLOCK_SIZE][bj*BLOCK_SIZE]),
                    reinterpret_cast<int (*)[DIST_SIZE]>(&dist[bi*BLOCK_SIZE][k]),
                    reinterpret_cast<int (*)[DIST_SIZE]>(&dist[k][bj*BLOCK_SIZE]),
                    kiLimitt
                );
            }
        }

    }
}

    // Write to Output
    graph.open(argv[2], ios::out  | ios::binary);

    // Write out and Cleanup Dist Memory Usage
    for (int i = 0; i < vertices; i++) {
        graph.write(reinterpret_cast<char *>(&dist[i][0]), sizeof(int)*vertices);
        // for (int j = 0; j < vertices; j++) {
        //     cout << dist[i][j] << " ";
        // }
        // cout << endl;
    }
    delete [] reinterpret_cast<int *>(dist);

    // Close the File
    graph.close();

    return 0;
}

void inplaceChange(int (*C)[DIST_SIZE], int(*A)[DIST_SIZE], int(*B)[DIST_SIZE], int kiLimit) {
    int tmp;
    for (int ki = 0; ki < kiLimit; ki++) {
        for (int i = 0; i < BLOCK_SIZE; i++) {
            for (int j = 0; j < BLOCK_SIZE; j++) {
                tmp = A[i][ki] + B[ki][j];
                C[i][j] = C[i][j] > tmp ? tmp : C[i][j];
            }
        }
    }   
}

