# IC253 — DSA Assignment 4 | Task 2: Logistics Dispatch Centre

> A C++ implementation of a regional disaster-relief dispatch centre that
> handles aid requests in real time using a hand-written **max-heap**, and
> produces post-shift sorted reports using **in-place randomised quicksort**.

---

## Table of Contents

1. [Problem Statement](#problem-statement)
2. [Concepts and Algorithms Used](#concepts-and-algorithms-used)
3. [Data Handling Policy](#data-handling-policy)
4. [Data Structures](#data-structures)
5. [Detailed Function Walkthrough](#detailed-function-walkthrough)
6. [Time and Space Complexity](#time-and-space-complexity)
7. [How to Run](#how-to-run)
8. [Test Cases](#test-cases)
9. [Project Structure](#project-structure)

---

## Problem Statement

A disaster-relief centre receives `N` aid requests over an operation window.
Each request has:

- a unique `requestID`
- an integer `priority` in `[0, 100]` (higher = more urgent)
- a submission `timestamp`

Two simultaneous requirements must be supported:

1. **Live dispatch.** At any moment the next request to be processed must be
   the one with the highest priority. Priorities can be updated mid-window
   without rebuilding the structure from scratch.
2. **Shift-end records.** After the window closes, all records must be sorted
   by priority for situation reports and independently sorted by timestamp for
   resource accounting. Both sorts must be **in-place** with `O(n log n)`
   average time and `O(log n)` auxiliary space.

---

## Concepts and Algorithms Used

| Concept | Where it is used | Why we use it |
|---|---|---|
| **Max-heap (array based)** | inside `heap` and the helpers `siftUp` / `siftDown` | extracting the max in O(log n) and updating a single element in O(log n) is exactly what the live dispatch queue needs; the syllabus covers heap implementation and heaps as priority queues |
| **Bottom-up heapify (Floyd's algorithm)** | inside `buildStructure` | builds the heap in O(n) overall instead of O(n log n) of repeated insertion |
| **Linear scan for ID lookup** | inside `findIndexByID` | the simplest way to locate a request inside the heap; no extra hash table is maintained |
| **Randomised quicksort with Lomuto partition** | inside `sortRecords` and `quicksortHelper` | divide-and-conquer sort that achieves O(n log n) average time and O(log n) auxiliary space (only the recursion stack); randomisation of the pivot avoids the worst case on already-sorted input |

---

## Data Handling Policy

The policy is enforced inside `validateInput()` and `updatePriority()` and is
consistent across every operation.

| Rule | Description |
|---|---|
| 1 | **Unique IDs**: every `requestID` inside one input batch must be unique |
| 2 | **Priority range**: `priority` must be an integer in `[0, 100]` |
| 3 | **Duplicate IDs** are rejected at input time, with the offending ID printed |
| 4 | **Out-of-range priorities** are rejected at input time, with the offending value printed |
| 5 | **`updatePriority`** rejects unknown IDs and out-of-range new priorities — both produce a clear error and leave the heap unchanged |

> **Assumption.** The task statement says `topKRequests` must not modify the
> live structure. We implement this by copying the heap array into a temporary
> buffer and running k extract-max operations on the copy. The live heap is
> never touched.

---

## Data Structures

### `Request` (struct)

```cpp
struct Request {
    int requestID;    //unique identifier
    int priority;     //integer in [0, 100], higher = more urgent
    int timestamp;    //submission time
};
```

A simple POD struct that the heap stores.

### `DispatchCentre` (class)

The class holds a single private member:

```cpp
vector<Request> heap;
```

The heap is laid out as a **standard array-based binary heap** using
`parent = (i-1)/2`, `leftChild = 2i+1`, `rightChild = 2i+2`. The array is
managed by the `vector<Request>` so growing and shrinking the heap (during
dispatch) is handled by `push_back` / `pop_back`.

Note that we do **not** maintain a separate ID → index lookup table. Locating
a request by ID is done by `findIndexByID`, a simple linear scan over the heap
array. This keeps the implementation simple and matches the syllabus level;
the trade-off is that `updatePriority` becomes O(n) instead of O(log n).

---

## Detailed Function Walkthrough

### Private Helpers

#### `swapNodes(i, j)`
Swaps `heap[i]` and `heap[j]` using a temporary `Request`.

#### `siftUp(i)`
Bubbles `heap[i]` upwards while it is larger than its parent. Used after a
priority increase. The loop stops at the root, or as soon as the heap property
is satisfied (current node is no larger than its parent).

#### `siftDown(i, n)`
Pushes `heap[i]` downwards while it is smaller than at least one of its
children. Used after replacing the root with the last element (in
`dispatchNext`) or after a priority decrease (in `updatePriority`). The
parameter `n` is the **effective size** of the heap so we never read past the
live region.

#### `findIndexByID(reqID)`
Linear scan of the heap. Returns the index where the request with
`requestID == reqID` lives, or -1 if no such request is present.

#### `getKey(r, key)`
Returns either `r.priority` or `r.timestamp` based on the `key` string.
Used inside `quicksortHelper` so the same partition logic can sort by either
field.

#### `quicksortHelper(arr, lo, hi, key, partitionCount)`
Recursive randomised quicksort with **Lomuto partition**:

1. base case: if `lo >= hi`, return
2. pick a random pivot index in `[lo, hi]` and swap that element to `arr[hi]`
3. partition: `i` tracks the boundary of the "less than or equal to pivot"
   region; for every `j` from `lo` to `hi - 1`, if `arr[j] <= pivot`,
   increment `i` and swap `arr[i]` with `arr[j]`
4. swap `arr[i+1]` with `arr[hi]` so the pivot lands at index `p = i + 1`
5. recurse on `[lo, p-1]` and `[p+1, hi]`

Pivot value, range, and partition boundary are printed at every recursive
call. `partitionCount` is incremented each time so the total partition count
can be reported at the end.

The whole sort is in-place. Auxiliary space is `O(log n)` on average from
the recursion stack only — no auxiliary arrays of size n are allocated.

#### `printHeap()`
Prints the current heap as an array. Called after every modification so the
trace is easy to follow.

### Public Operations

#### `validateInput(requests)`
Prints the policy. Then for each request scans all earlier requests for a
duplicate ID (rule 3) and checks that the priority lies in `[0, 100]`
(rule 4). The first failure returns false.

#### `buildStructure(requests)`
1. copies `requests` into `heap`
2. starts from the last non-leaf node (index `n/2 - 1`) and walks backwards
   to the root
3. at each index, sifts the current node down so that the subtree rooted at
   it satisfies the max-heap property
4. every swap is printed; if no swap is needed at a level, that is also
   printed

The bottom-up heapify is `O(n)` overall — much better than the `O(n log n)`
that repeated insertion would give.

#### `dispatchNext()`
Returns the root (highest-priority request) and restores the heap:

1. save the root in a local variable
2. replace `heap[0]` with `heap.back()`
3. shrink the heap by 1
4. sift the new root down

A sentinel `Request {-1, -1, -1}` is returned if the heap is empty. The new
heap state is printed after restoration.

#### `updatePriority(reqID, newPriority)`
1. range-check `newPriority` (rule 5)
2. locate the request via `findIndexByID` (rule 5)
3. save the old priority, overwrite with the new one
4. if the new priority is greater than the old one → sift up
   if smaller → sift down
   if equal → no movement

The direction of movement is printed and the resulting heap is shown.

#### `sortRecords(records, key)`
1. sanity-check `key` is either `"priority"` or `"timestamp"`
2. seed the RNG with a fixed seed (so the trace is reproducible)
3. call `quicksortHelper` on the full range
4. print the sorted result

#### `topKRequests(k)`
1. handle the edge cases `k <= 0` and `k > heap.size()`
2. copy `heap` into `tempHeap`
3. perform k extract-max operations on `tempHeap` only — the live heap is
   never modified
4. print the k highest-priority requests as they come out of the temp heap

---

## Time and Space Complexity

> n = number of requests currently inside the heap, k = parameter to `topKRequests`

| Operation | Time Complexity | Space Complexity | Notes |
|---|---|---|---|
| `validateInput()` | `O(n^2)` | `O(1)` extra | duplicate check is the dominant `O(n^2)` term |
| `buildStructure()` (bottom-up heapify) | `O(n)` | `O(n)` for the heap itself, `O(1)` extra | classic Floyd's algorithm result |
| `dispatchNext()` | `O(log n)` | `O(1)` | one sift down |
| `updatePriority()` | `O(n)` | `O(1)` | dominated by the linear ID search; the sift itself is `O(log n)` |
| `sortRecords()` (randomised quicksort) | `O(n log n)` average, `O(n^2)` worst | `O(log n)` auxiliary (recursion stack only, no extra arrays) | meets the "no auxiliary arrays of size n" requirement |
| `topKRequests(k)` | `O(n + k log n)` | `O(n)` for the temporary copy | live heap is untouched |
| `siftUp` / `siftDown` | `O(log n)` | `O(1)` | walks one root-to-leaf path |
| `findIndexByID` | `O(n)` | `O(1)` | linear scan |

---

## How to Run

### Prerequisites

- A C++17-compatible compiler (g++, clang++, MSVC)

### Compile

```bash
g++ -std=c++17 -o task2 TASK2.cpp
```

### Run

```bash
./task2
```

`main()` runs four built-in test cases automatically (two valid, two invalid)
and prints all output to `stdout`.

---

## Test Cases

### Test Case 1 — Six requests

After heapify, the root is `ID = 104` with priority **90**. After two
`dispatchNext` calls, the dispatched IDs are 104 and 102.
`updatePriority(103, 95)` lifts request 103 to the top (sifted UP).
`updatePriority(106, 20)` pushes 106 down (sifted DOWN).
`topKRequests(3)` returns `{103, 105, 101}` without disturbing the live heap.
The full record list is then sorted by both `"priority"` and `"timestamp"`
using randomised quicksort, with the full pivot / partition trace printed.

### Test Case 2 — Five requests

Demonstrates `dispatchNext`, an "up" `updatePriority`, two **invalid**
`updatePriority` calls (unknown ID `999` and out-of-range priority `250`)
that are correctly rejected, `topKRequests(2)`, and a sort by priority.

### Test Case 3 — Invalid input (duplicate ID)

Two requests share ID `301`. The validator flags it at position 1 and
rejects the input.

### Test Case 4 — Invalid input (out-of-range priority)

Request `402` has priority `150`. The validator flags it and rejects the input.

---

## Project Structure

```
TASK2/
├── TASK2.cpp        # complete implementation + 4 built-in test cases
└── README.md        # this file
```

---

> **Course:** IC253 — Data Structures and Algorithms
> **Author:** Daksh Rathi
