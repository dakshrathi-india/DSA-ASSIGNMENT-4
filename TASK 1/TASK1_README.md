# IC253 — DSA Assignment 4 | Task 1: Smart City Road Network Optimizer

> A C++ implementation of an undirected weighted graph with two weights per
> edge (construction cost and travel time). The class supports validating the
> graph, building the MST, finding the shortest travel-time path, comparing
> the MST path against the shortest path, and identifying critical roads.

---

## Table of Contents

1. [Problem Statement](#problem-statement)
2. [Concepts and Algorithms Used](#concepts-and-algorithms-used)
3. [Graph Validation Policy](#graph-validation-policy)
4. [Data Structures](#data-structures)
5. [Detailed Function Walkthrough](#detailed-function-walkthrough)
6. [Time and Space Complexity](#time-and-space-complexity)
7. [How to Run](#how-to-run)
8. [Test Cases](#test-cases)
9. [Project Structure](#project-structure)

---

## Problem Statement

A city is modelled as an undirected weighted graph with `N` intersections and `M` roads.
Every road carries two weights:

- a **construction cost** (used while building the MST)
- a **travel time** (used while finding the shortest path)

The program must:

1. validate the input graph against a fixed policy
2. build the **minimum cost spanning road network** using Kruskal's algorithm
3. find the **minimum travel time path** between two intersections using Dijkstra's algorithm
4. compare the MST path against the optimal shortest path
5. identify **critical roads** — MST edges whose removal increases the MST cost
   (or disconnects the graph)

> **Note on independence of operations.** The shortest path on the full graph
> is mathematically independent of MST construction, and the code structure
> reflects that: `adjFull` is built once inside the **constructor**, so
> `shortestPath(S, T)` can be called on its own without requiring
> `buildMST()` first. Only `compareNetworks(S, T)` and `criticalRoads()`
> genuinely need the MST, and both of them guard on the `mstBuilt` flag.

---

## Concepts and Algorithms Used

| Concept | Where it is used | Why we use it |
|---|---|---|
| **Adjacency chains** (`vector<vector<pair<int,int>>>`) | inside `adjFull` and `adjMST` | the syllabus mentions "adjacency matrix and linked adjacency chains"; chains are space-efficient when M is much smaller than N² and are the natural structure for Dijkstra's scan |
| **Disjoint Set Union (DSU)** with path compression and union by size | inside `buildMST` and `criticalRoads` | DSU lets us check in nearly O(1) whether two nodes lie in the same component — exactly what Kruskal's needs for cycle detection |
| **Kruskal's algorithm** (greedy) | inside `buildMST` | builds the MST by repeatedly picking the cheapest edge that does not form a cycle; this is the greedy approach taught under "Basic Algorithm Techniques" |
| **Selection sort** on edge indices | inside `sortEdgeIndicesByCost` | the syllabus lists selection sort explicitly; it sorts the M edge indices by construction cost so Kruskal's can process them in ascending order |
| **Dijkstra's algorithm** (O(N²) form) | inside `dijkstra` | finds the shortest path from one source to every other node when all edge weights are non-negative; the simple O(N²) form is used because it needs no heap and is easy to follow |
| **BFS** | inside `countReachableFromZero` | unweighted traversal used only to check whether the input graph is connected during validation |
| **Path reconstruction via predecessor array** | inside `reconstructPath` | once Dijkstra fills `prev[]`, walking backwards from T to S and reversing gives the actual S → T path |

---

## Graph Validation Policy

The policy is enforced inside `validateGraph()`. The rules are checked in the
order listed below, and the **very first** rule that fails causes the input
to be rejected with a specific reason printed to stdout.

| Order | Rule | Description |
|---|---|---|
| 1 | Self-loops | an edge with `u == v` is invalid |
| 2 | Node id range | every endpoint must lie in `[0, N - 1]` |
| 3 | Non-negative weights | both `cost` and `travelTime` must be `>= 0` |
| 4 | Duplicate roads | the same unordered pair `{u, v}` cannot appear twice |
| 5 | Connectivity | a BFS from node 0 must reach every node |

For runtime queries, **invalid source / destination node ids** in
`shortestPath()` and `compareNetworks()` are caught at the start of the call
and reported, leaving the graph state untouched.

---

## Data Structures

### `Edge` (struct)

```cpp
struct Edge {
    int u;            //first endpoint
    int v;            //second endpoint
    int cost;         //construction cost (used for MST)
    int travelTime;   //travel time (used for shortest path)
};
```

A simple POD struct. The graph is treated as undirected so the order of `u`
and `v` does not matter.

### `CityGraph` (class)

| Member | Purpose |
|---|---|
| `int N` | number of intersections |
| `vector<Edge> edges` | the original input list of roads, never modified after the constructor |
| `vector<Edge> mstEdges` | the roads accepted into the MST by `buildMST()` |
| `int mstCost` | total construction cost of the MST |
| `vector<vector<pair<int,int>>> adjFull` | adjacency chain of the full graph, weighted by `travelTime`; **built once in the constructor** |
| `vector<vector<pair<int,int>>> adjMST` | adjacency chain of the MST only, weighted by `travelTime`; built inside `buildMST()` |
| `bool mstBuilt` | guard flag; true only after `buildMST()` has completed |
| `vector<int> parent`, `setSize` | the two arrays of the DSU; reused across the build call and the rebuild loop inside `criticalRoads` |

---

## Detailed Function Walkthrough

### Private Helpers

#### `initDSU(n)`
Allocates `parent[]` and `setSize[]` to size `n`, sets `parent[i] = i` and
`setSize[i] = 1` so every node starts as its own set of size 1.

#### `findParent(x)`
Returns the root of the set that contains `x`. Uses **path compression**:
while walking up to the root, every visited node has its `parent[]` reset to
the root so a subsequent call on the same node is essentially O(1).

#### `unionSets(x, y)`
Finds the roots of `x` and `y`. If they are already equal, no union is
performed. Otherwise the **smaller tree is attached under the larger one**
(union by size). This keeps the trees shallow so subsequent `findParent`
calls remain fast. Returns `true` if a real union actually happened.

#### `sortEdgeIndicesByCost(indexList)`
Plain **selection sort** on a list of indices into `edges[]`. We sort indices
(not the edges themselves) so the original input order is preserved. At every
iteration the smallest-cost edge in the unsorted suffix is swapped to the
front of that suffix.

#### `dijkstra(adj, S, dist, prev)`
The classic O(N²) form of Dijkstra's algorithm:
1. initialise `dist[]` to `INT_MAX`, set `dist[S] = 0`, mark all `visited[]` false
2. repeat N times:
   1. linearly scan `dist[]` and pick the unvisited node `u` with the smallest distance
   2. if no such node exists, break (all remaining nodes are unreachable)
   3. mark `u` as visited
   4. for every neighbour `v` of `u`, **relax** the edge: if `dist[u] + w < dist[v]`, update `dist[v]` and set `prev[v] = u`

No priority queue is used. The O(N²) form is simple and sufficient at the
assignment's input sizes.

#### `reconstructPath(T, prev, dist)`
If `dist[T] == INT_MAX`, returns an empty vector (T is unreachable).
Otherwise walks backwards from T using `prev[]` until the source is reached
(`prev == -1`), then reverses the collected vector so the path goes S → T.

#### `getPathTravelTime(adj, S, T)`
Wrapper around `dijkstra` and the `dist[T]` lookup that returns the integer
travel time of the shortest S → T path, or `-1` if unreachable.

#### `buildFullAdjacency()`
Called **once** from the constructor. Walks `edges[]` and pushes back the
`{neighbour, travelTime}` pairs into `adjFull`. Because the input edge list
does not change after the constructor, `adjFull` is built exactly once and
reused by every subsequent call.

#### `buildMSTAdjacency()`
Called at the end of `buildMST()`. Walks `mstEdges[]` and pushes back the
`{neighbour, travelTime}` pairs into `adjMST`. Clears `adjMST` before
populating it so repeated calls do not duplicate entries.

#### `countReachableFromZero()`
Standard **BFS** from node 0 over an unweighted version of the graph.
Returns the count of nodes that were reached. Uses a `vector<int>` with a
front index as a queue (no `<queue>` header required).

### Public Operations

#### `validateGraph()`
Prints the policy, then for each input edge checks rules (1), (2), (3) and
(4) in that order. The duplicate detection is a nested loop that scans all
earlier edges — simple and fine for the assignment's input sizes. Finally
rule (5) is checked via `countReachableFromZero()`. The first failing rule
returns `false`; if every rule passes, `true` is returned.

#### `buildMST()`
1. builds an `indexList` of integers `0, 1, ..., M - 1` and sorts it by
   `edges[i].cost` using `sortEdgeIndicesByCost`
2. initialises the DSU
3. walks the sorted indices: if the two endpoints are in different sets, the
   edge is accepted into `mstEdges`, the sets are unioned, and `mstCost` is
   updated. Otherwise the edge is rejected as a cycle.
4. every accept / reject is printed along with the running MST cost
5. calls `buildMSTAdjacency()` to prepare `adjMST` for later queries and
   sets `mstBuilt = true`

#### `shortestPath(S, T)`
Runs on the **full graph**, entirely independent of `buildMST()`. Validates
`S` and `T` against `[0, N - 1]`, runs `dijkstra` on `adjFull`, reconstructs
the path, and prints the path along with a per-hop travel time breakdown and
the total. The per-hop time is looked up by scanning `adjFull[from]` for the
matching neighbour entry.

#### `compareNetworks(S, T)`
Guards on `mstBuilt`. Calls `getPathTravelTime` once on `adjFull` and once on
`adjMST`. If both are non-negative, prints both travel times and the
difference. The percentage is reported relative to the optimal shortest-path
travel time.

#### `criticalRoads()`
Guards on `mstBuilt`. For every edge in `mstEdges`:
1. reinitialises the DSU and reruns Kruskal's pass while skipping this one edge
2. if fewer than `N - 1` edges can be added, the road is critical because
   removing it disconnects the graph
3. else if the new MST cost is greater than the original `mstCost`, the road
   is critical and the cost difference is printed as the impact
4. otherwise (a same-cost alternative exists) the road is not critical

---

## Time and Space Complexity

> `N` = number of intersections, `M` = number of roads, `E` = number of MST
> edges (= `N - 1` for a valid input)

| Operation | Time Complexity | Space Complexity | Notes |
|---|---|---|---|
| `validateGraph()` | `O(M^2 + N + M)` | `O(N + M)` | the `O(M^2)` term comes from the duplicate-edge double loop |
| `sortEdgeIndicesByCost()` | `O(M^2)` | `O(M)` | selection sort on M edge indices |
| `findParent()` | nearly `O(1)` amortised | `O(N)` stack (worst case recursion) | path compression |
| `unionSets()` | nearly `O(1)` | `O(1)` | union by size keeps trees shallow |
| `buildMST()` (Kruskal's) | `O(M^2)` | `O(N + M)` | dominated by the selection sort |
| `dijkstra()` (O(N²) form) | `O(N^2 + M)` | `O(N)` | scanning `dist[]` to pick the next node is `O(N)` per iteration |
| `shortestPath()` | `O(N^2 + M)` | `O(N)` | one Dijkstra call plus path reconstruction |
| `compareNetworks()` | `O(N^2 + M)` | `O(N)` | two Dijkstra calls |
| `criticalRoads()` | `O(E · M^2)` | `O(N + M)` | Kruskal's rerun once per MST edge |

---

## How to Run

### Prerequisites

- A C++17-compatible compiler (g++, clang++, MSVC)

### Compile

```bash
g++ -std=c++17 -o task1 TASK1.cpp
```

### Run

```bash
./task1
```

`main()` runs four built-in test cases automatically and prints all output
to `stdout`.

---

## Test Cases

### Test Case 1 — 6 intersections, 9 roads
Validates successfully. The MST has 5 edges with total cost = **13**. The
shortest path from node 0 to node 5 has travel time = **9**, while the MST
path between the same pair takes **19** units — the MST path is **111.1%
slower**. All 5 MST edges are critical because removing any one of them
strictly increases the MST cost.

### Test Case 2 — 4 intersections, 5 roads
Validates successfully. The MST has 3 edges with total cost = **19**. The
shortest path from node 0 to node 3 has travel time = **3**, while the MST
path takes **8** units — the MST path is **166.7% slower**. All 3 MST edges
are critical.

### Test Case 3 — `shortestPath` called **before** `buildMST`
Validates successfully. `shortestPath(0, 4)` is invoked **first** and
correctly returns the path `0 -> 2 -> 4` with total travel time 3. Only
after that is `buildMST()` called, followed by `compareNetworks(0, 4)` and
`criticalRoads()`. This test case demonstrates that the shortest-path
operation on the full graph is independent of MST construction.

### Test Case 4 — Invalid graph (self-loop)
The input contains the edge `(0, 0)`. Rule (1) flags it immediately and the
graph is rejected without any further processing.

---

## Project Structure

```
TASK1/
├── TASK1.cpp        # complete implementation + 4 built-in test cases
└── README.md        # this file
```

---

> **Course:** IC253 — Data Structures and Algorithms
> **Author:** Daksh Rathi
