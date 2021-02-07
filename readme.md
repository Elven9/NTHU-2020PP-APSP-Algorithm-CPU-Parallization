# 2020 Parallel Programming: APSP Algorithm CPU-Version

- [2020 Parallel Programming: APSP Algorithm CPU-Version](#2020-parallel-programming-apsp-algorithm-cpu-version)
  - [Summary](#summary)
  - [Implementation](#implementation)

## Summary

作業的主題是使用 CPU 來實作一個非常有名的 Graph Algorithm, All Pair Shortest Path 演算法。APSP 是個 Memory Bound 的演算法，如果使用一般的 Floyd–Warshall algorithm 在 Edge 非常 Sparse 的狀態下效能會很差，所以為了利用 Memory Cache 的特性，這次我主要實作的演算法是 Blocked Version 的 Floyd–Warshall algorithm(Asymptotic notation 仍然是 O(V^3)，但實際效果好很多)，在和一般版本的 Floyd Warshall APSP Algorithm 加上 Johnson's Algorithm 去當實驗組比較。

## Implementation