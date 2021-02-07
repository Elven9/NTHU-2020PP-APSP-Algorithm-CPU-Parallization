# 2020 Parallel Programming: APSP Algorithm CPU-Version

- [2020 Parallel Programming: APSP Algorithm CPU-Version](#2020-parallel-programming-apsp-algorithm-cpu-version)
  - [Summary](#summary)
  - [Implementation](#implementation)
    - [Algorithm](#algorithm)
    - [Data Structure](#data-structure)
    - [Input & Output](#input--output)
    - [Time Complexity](#time-complexity)
  - [Experiment](#experiment)
    - [Setup](#setup)
    - [時間種類](#時間種類)
    - [Sequential Version](#sequential-version)
    - [Blocked Size Tuning](#blocked-size-tuning)
    - [Strong Scalability & Time Profile (Blocked Version)](#strong-scalability--time-profile-blocked-version)
    - [Strong Scalability & Time Profile for Johnson's Algorithm](#strong-scalability--time-profile-for-johnsons-algorithm)
    - [Test Cases Generation](#test-cases-generation)
    - [Conclusion](#conclusion)

## Summary

作業的主題是使用 CPU 來實作一個非常有名的 Graph Algorithm, All Pair Shortest Path 演算法。APSP 是個 Memory Bound 的演算法，如果使用一般的 Floyd–Warshall algorithm 在 Edge 非常 Sparse 的狀態下效能會很差，所以為了利用 Memory Cache 的特性，這次我主要實作的演算法是 Blocked Version 的 Floyd–Warshall algorithm（Asymptotic notation 仍然是 O(V^3)，但實際效果好很多），在和一般版本的 Floyd Warshall APSP Algorithm 加上 Johnson's Algorithm 去當實驗組比較。

## Implementation

### Algorithm

```
#define BLOCK_SIZE 128

void floyd_warshall(blocks) {
	// Floyd Warshall Algorithm Subroutine for One Block.
	…
}

// Main Algorithm Part
for k = 0 to V/BLOCK_SIZE {
	// Dependent Phase: Process Diagonal Dependent Block
	// block_ij for block at (i, j)
	floyd_warshall(block_kk);

	// Partial Dependent Phase:
	// Compute Blocks on Same Row
	parallel for j from 0 to V/BLOCK_SIZE: floyd_warshall(block_kj);

	// Compute Blocks on Same Column
	parallel for i from 0 to V/BLOCK_SIZE: floyd_warshall(block_ik);

	// Independent Phase:
	parallel for i from 0 to V/BLOCK_SIZE, j from 0 to V/BLOCK_SIZE:
		if (i not equal k and j not equal k ) floyd_warshall(block_ij);
}
```

這麼做其實算出來的 Asymptotic notation 其實還是 O(V^3)，但這個版本的最大好處是有考量到 Memory Cache 的問題。原本的演算法中每一次新增一個 Vertex 來考慮時都是每個點一次考慮完所有該點對其他點的完整 Distance Matrix，可是這樣可能同時使 CPU 需要 Load 好多塊不同地方的記憶體，讓整體效能下降，反而不利於演算法的執行。

### Data Structure

使用的是 Adjacency Matrix，new 出一個 contiguous 的 heap memory，然後再透過 Pointer 解釋 Data Byte Length 的方式，使 Dist Matrix 可以視為 Two Dimension 的 Array 而省掉計算 Index 的時間。

```c++
  // ...

  // Create Data Structure to Hold up all Dist
  int (*dist)[DIST_SIZE] = reinterpret_cast<int (*)[DIST_SIZE]>(new int[DIST_SIZE * DIST_SIZE]);\

  // ...
```

### Input & Output

Input 跟 Output 都是使用 `std::fstream` 的物件實作，並且在 Input 的時候有實作 Unroll (4)，一次讀進 4 個數，Output 的時候一次寫入一整個 Row 的資料回去檔案。

```c++
do {
  graph.read(reinterpret_cast<char *>(&buf), sizeof(buf));

  // Unroll
  dist[buf[0]][buf[1]] = buf[2];

  // ...
} while (inCounter < edges);
```

### Time Complexity

```
// Time Complexity
for k = 0 to V/BLOCK_SIZE {               - - - - - > O(V/BLOCK_SIZE) = O(V)
	// Dependent Phase:                     - - - - - > O(BLOCK_SIZE ^ 2) = O(1)

  floyd_warshall(block_kk);

	// Partial Dependent Phase:             - - - - - > O(2V / (BLOCK_SIZE * P) * BLOCK_SIZE ^ 2)
						                                        = O(2 * V * BLOCK_SIZE / P)
						                                        = O(V/P) = O(V)

	parallel for j from 0 to V/BLOCK_SIZE: floyd_warshall(block_kj);
	parallel for i from 0 to V/BLOCK_SIZE: floyd_warshall(block_ik);

	// Independent Phase:                   - - - - - > O((V / BLOCK_SIZE) ^ 2 * BLOCK_SIZE ^ 2 / P)
					                                          = O(V ^ 2 / P)

	parallel for i from 0 to V/BLOCK_SIZE, j from 0 to V/BLOCK_SIZE:
		if (i not equal k and j not equal k ) floyd_warshall(block_ij);
}
```

Time Complexity = O(V ^ 3 / P)

Floyd Warshall 這個演算法對平行話來說，最大的問題就是最外面 K 是不能平行的，因為 Dynamic Programming 的關係。Phase1 同樣不能平行（或是說不用平行，畢竟大小才 Block_Size ^ 2 而已），所花的時間為 O(1)（Assume Block_Size << Vertices）；Phase 2 可以平行，這個 Phase 時 Same Column 跟 Same Row 是可以分別完成的；Independent Phase 時每個 Block 就完全不相關了，每個 Thread 一次會分配到一個 Block 執行。

這個 Blocked Version Floyd Warshall 跟一般 Floyd Warshall APSP 的 Asymptotic Notation 是相同的，但因為 Cache 的關係，效能會比原本的好很多。

## Experiment

### Setup

- 使用的是課堂提供的機器(apollo)
- Data Setup 為 c.18.1 的資料，V = 3000，E = 3483319
- LLVM Compiler (Compile with clang)
- 時間計算方法使用的是 clock_gettime with CLOCK_MONOTONIC。

### 時間種類

這次 Project 我總共測了四種時間，分別如下：

- Preprocessing & Input Time: 主要是初始化 Distance Matrix(0 for Diagonal, 2^30-1 for Others)，以及把 Edge 資料從檔案讀入到 Distance Matrix 中。
- Computation Time: 真正在執行 APSP 演算法所花的時間
- Output Time: 資料寫回檔案所花費的時間
- Full Time: 全部加總所花費的時間

### Sequential Version

一個以一般 O(V ^ 3) 的 Floyd Warshall APSP Algorithm 跑出來的時間為：33.7104 (Sec.)，但是因為我在實作我的 Parallel Version 時已經把演算法換成 Blocked Version，所以下面 Strong Scalability 的加速基準是以一個 Thread 跑我的 Parallel Version 所得到的數據為主。

### Blocked Size Tuning

根據 Paper 所寫的結果，最好的 Block Size 是 48，他是透過一連串計算得來的結果（L1, L2 Cache Hit / Miss Rate）。但是，那篇論文是 2003 年的文章，現在 OS 跟硬體都已經有長足的進步，再加上我今天是實作成 Parallel Version 的緣故，每個 Thread 的 Process 單位是一個 Block，取太大的 BlockSize 反而不利於 Load Balancing，所以我自己又做了個實驗，嘗試畫出哪一個大小是 Sweet Point（我不會用計算的，有點複雜）：

![](Image/Screen%20Shot%202021-02-07%20at%204.05.57%20PM.png)

最後得到的結果是 128 最好。

### Strong Scalability & Time Profile (Blocked Version)

![](Image/Screen%20Shot%202021-02-07%20at%204.09.30%20PM.png)

Vanilla Version 的演算法最大的問題就在於 Memory Bound 的問題，即使今天開再多的 Core 給他，因為 Memory Access Pattern 的問體導致整體 Scalability 沒辦法一直往上升，讓整體平行話的效果下降。

### Strong Scalability & Time Profile for Johnson's Algorithm

這個實驗算是一個失敗組，我原本預期效果會比 Floyd Warshall 版本好（一方面我覺得 test cases 裡的 Edges 應該很分散，另一方面 Sequential 版本的 Asymptotic 表現比較好...），但實作出來後才發現 Johnsons’s Algorithm 唯一可以簡單平行的地方就是對每個 Node 的分別計算，至於每個 Node 裡面的計算如果要平行的話就必須要讓 Adjacency List 以及 Fibonacci Heap 變成一個 Concurrent 的 Data Structure，實作實在太好工了，而且最後可能會因為這些 Concurrent 的 Data Structure 需要 Lock 機制而導致 Scalability 不好。

當然可以實作出 Lock Free 的 Concurrent Data Structure，但這個工我覺得有一點太大了。以下是 Johnson’s Algorithm 的 Scalability 以及 Time Profile（我只列出一組 12 Thread 的 c.14.1，因為時間太長了）:

![](Image/Screen%20Shot%202021-02-07%20at%204.13.07%20PM.png)

還有一個可能，是我不太熟悉 boost 的 fibonacci heap 使用，我可能無意中呼叫了太多沒用的 Function Call，所以導致時間變得很長....

### Test Cases Generation

我 Generate Test Case 時寫了一個簡單的 Script，按照 Input Case 的要求生成出要的資料。唯一比較搞人的部分是我有限制 Test Case 至少在 5000 組以上，對應的 Edges 數量也要大於 5000 * 5000 - 1 組，但是基本上如果 Random 出一組不會太 Sparse 的資料搞人的效果可能也不會太好。

### Conclusion

這次作業要實作出來真的不難，畢竟 Floyd Warshall 基本上就是 DP 建表，實作也只是三個迴圈的事情。不過有個小遺憾的地方是我最後還是沒能寫出一個有 Vectorized 的版本（其他地方都是靠 -O3 的 Loop Auto Vectorization），嘗試過很多方法如對角線計算等等 Vectorized 方法，但最後都是以變慢收場.....