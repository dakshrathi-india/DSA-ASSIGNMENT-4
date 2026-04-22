# IC253 — DSA Assignment 4 | Task 2: Logistics Dispatch Centre

> A C++ implementation of a regional disaster-relief dispatch centre that
> handles aid requests in real time using a hand-written **max-heap**, and
> produces post-shift sorted reports using **in-place randomised quicksort**.

---

## Table of Contents

1. [Problem Statement](#problem-statement)
2. [Concepts and Algorithms Used](#concepts-and-algorithms-used)
3. [Data Handling Policy](#data-handling-policy)
4. [Sort Order and Top-K Conventions](#sort-order-and-top-k-conventions)
5. [Data Structures](#data-structures)
6. [Detailed Function Walkthrough](#detailed-function-walkthrough)
7. [Time and Space Complexity](#time-and-space-complexity)
8. [How to Run](#how-to-run)
9. [Test Cases](#test-cases)
10. [Project Structure](#project-structure)

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
   by priority for situation reports and independently sorted by timestamp
   for resource accounting. Both sorts must be **in-place** with `O(n log n)`
   average time and `O(log n)` auxiliary space.

---

## Concepts and Algorithms Used

| Concept | Where it is used | Why we use it |
|---|---|---|
| **Max-heap (array based)** | inside `heap` and the helpers `siftUp` / `siftDown` | extracting the max in O(log n) and updating a single element in O(log n) is exactly what the live dispatch queue needs; the syllabus covers heaps as priority queues |
| **Bottom-up heapify (Floyd's algorithm)** | inside `buildStructure` | builds the heap in O(n) overall instead of the O(n log n) of repeated insertion |
| **Linear scan for ID lookup** | inside `findIndexByID` | the simplest way to locate a request inside the heap; no extra hash table is maintained |
| **Randomised quicksort with Lomuto partition** | inside `sortRecords` and `quicksortHelper` | divide-and-conquer sort that achieves O(n log n) average time and O(log n) auxiliary space (only the recursion stack, no auxiliary array); pivot randomisation avoids the worst case on already-sorted input |

> **Possible optimisation (not implemented).** `findIndexByID` does a linear
> O(n) scan. If needed in the future, this could be reduced to O(1) by
> maintaining a second array / hash map from `requestID` to heap index that
> is kept in sync inside `swapNodes`. The current implementation deliberately
> keeps the code simple and within syllabus-level concepts, so
> `updatePriority` ends up O(n) instead of O(log n).

---

## Data Handling Policy

The policy is enforced inside `validateInput()` and `updatePriority()` and
is consistent across every operation.

| Rule | Description |
|---|---|
| 1 | **Unique IDs**: every `requestID` inside one input batch must be unique |
| 2 | **Priority range**: `priority` must be an integer in `[0, 100]` |
| 3 | **Duplicate IDs** are rejected at input time, with the offending ID printed |
| 4 | **Out-of-range priorities** are rejected at input time, with the offending value printed |
| 5 | **`updatePriority`** rejects unknown IDs and out-of-range new priorities — both produce a clear error and leave the heap unchanged |

---

## Sort Order and Top-K Conventions

**Sort order.** `sortRecords(records, key)` sorts in **descending** order of
the chosen key:
- for `key == "priority"` → highest priority first
- for `key == "timestamp"` → latest timestamp first

This matches the natural reading of a shift-end situation report where the
most urgent / most recent records should appear at the top. The convention
is enforced by the comparison `val >= pivotVal` inside the Lomuto partition
loop (see `quicksortHelper`).

**Top-k.** `topKRequests(k)` **returns** a `vector<Request>` holding the k
highest-priority requests in decreasing order, **and** also prints each ID
and priority. The live heap (`this->heap`) is **never** modified — the
extractions are performed on a local copy `tempHeap`.

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

A simple POD struct. The heap stores `Request` values directly.

### `DispatchCentre` (class)

The class has a single private member:

```cpp
vector<Request> heap;
```

The heap is laid out as a **standard array-based binary heap** using
`parent = (i - 1) / 2`, `leftChild = 2i + 1`, `rightChild = 2i + 2`. The
array is held inside a `vector<Request>` so the heap can grow and shrink
naturally via `push_back` / `pop_back` during `dispatchNext`.

---

## Detailed Function Walkthrough

### Private Helpers

#### `swapNodes(i, j)`
Swaps `heap[i]` and `heap[j]` via a temporary `Request`.

#### `siftUp(i)`
Bubbles `heap[i]` upwards as long as it is greater than its parent. Used
after a priority increase. Stops at the root or as soon as the heap property
is satisfied.

#### `siftDown(i, n)`
Pushes `heap[i]` downwards as long as one of its children is greater than
it. Used after replacing the root with the last element (in `dispatchNext`)
or after a priority decrease (in `updatePriority`). The parameter `n` is
the **effective size** of the heap so we never read past the live region.

#### `findIndexByID(reqID)`
Linear scan over `heap`. Returns the index where the request with
`requestID == reqID` lives, or `-1` if no such request is present.

#### `getKey(r, key)`
Returns either `r.priority` or `r.timestamp` based on the `key` string.
Used inside `quicksortHelper` so the same partition logic can sort by either
field.

#### `quicksortHelper(arr, lo, hi, key, partitionCount)`
Recursive randomised quicksort with **Lomuto partition**, producing a
**descending** order:

1. base case: if `lo >= hi`, return
2. pick a random pivot index in `[lo, hi]` and swap that element to `arr[hi]`
3. partition: `i` tracks the boundary of the "greater than or equal to pivot"
   region; for every `j` from `lo` to `hi - 1`, if `arr[j] >= pivot`,
   increment `i` and swap `arr[i]` with `arr[j]`
4. swap `arr[i + 1]` with `arr[hi]` so the pivot lands at index `p = i + 1`
5. recurse on `[lo, p - 1]` and `[p + 1, hi]`

Pivot value, range, and partition boundary are printed at every recursive
call. `partitionCount` is incremented each time so the total number of
partitions is reported at the end.

The sort is fully in-place. Auxiliary space is `O(log n)` on average from
the recursion stack only — no auxiliary array of size n is allocated, which
is exactly what the problem statement asks for.

#### `printHeap()`
Prints the current heap as an array. Called after every modification so the
trace is easy to follow during viva.

### Public Operations

#### `validateInput(requests)`
Prints the policy. Then for each request, scans all earlier requests for a
duplicate ID (rule 3) and checks that the priority lies in `[0, 100]`
(rule 4). The first failure returns `false`.

#### `buildStructure(requests)`
1. copies the input `requests` into `heap`
2. starts from the last non-leaf node (index `n / 2 - 1`) and walks
   backwards to the root
3. at each index, sifts the current node down so that the subtree rooted at
   it satisfies the max-heap property
4. every swap is printed; if no swap is needed at a level, that is also
   printed

The bottom-up heapify is `O(n)` overall — better than the `O(n log n)` that
repeated insertion would give.

#### `dispatchNext()`
Returns the root (highest-priority request) and restores the heap:

1. save the root in a local `top`
2. replace `heap[0]` with `heap.back()`
3. shrink the heap by 1 via `pop_back()`
4. sift the new root down

A sentinel `Request {-1, -1, -1}` is returned if the heap is empty. The
resulting heap state is printed after restoration.

#### `updatePriority(reqID, newPriority)`
1. range-checks `newPriority` (rule 5)
2. locates the request via `findIndexByID` (rule 5)
3. saves the old priority, overwrites with the new one
4. if new > old → sift up
   if new < old → sift down
   if new == old → no movement

Direction of movement is printed and the resulting heap is shown.

#### `sortRecords(records, key)`
1. checks that `key` is either `"priority"` or `"timestamp"`
2. seeds the RNG with a fixed seed (so the trace is reproducible run after run)
3. calls `quicksortHelper` on the full range
4. prints the sorted result with a `"sorted result (descending on <key>):"`
   banner that makes the sort order self-documenting

#### `topKRequests(k)`
1. handles the edge cases `k <= 0` and `k > heap.size()`
2. copies `heap` into `tempHeap`
3. performs `k` extract-max operations on `tempHeap` only — the live heap is
   **never** modified
4. prints each of the k highest-priority requests as they come out
5. **returns** a `vector<Request>` containing those same requests in
   decreasing order of priority (i.e. same order as printed)

---

## Time and Space Complexity

> `n` = number of requests currently inside the heap, `k` = parameter to
> `topKRequests`

| Operation | Time Complexity | Space Complexity | Notes |
|---|---|---|---|
| `validateInput()` | `O(n^2)` | `O(1)` extra | duplicate check is the dominant `O(n^2)` term |
| `buildStructure()` (bottom-up heapify) | `O(n)` | `O(n)` for the heap, `O(1)` extra | classic Floyd's algorithm result |
| `dispatchNext()` | `O(log n)` | `O(1)` | one sift down |
| `updatePriority()` | `O(n)` | `O(1)` | dominated by `findIndexByID`'s linear scan; the sift itself is `O(log n)` |
| `sortRecords()` (randomised quicksort) | `O(n log n)` average, `O(n^2)` worst | `O(log n)` auxiliary (recursion stack only, no extra array) | meets the "no auxiliary array of size n" requirement |
| `topKRequests(k)` | `O(n + k log n)` | `O(n)` for the temporary copy | live heap is untouched |
| `siftUp`, `siftDown` | `O(log n)` | `O(1)` | single root-to-leaf path |
| `findIndexByID` | `O(n)` | `O(1)` | linear scan — see optimisation note at the top |

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

`main()` runs four built-in test cases automatically (two valid, two
invalid) and prints all output to `stdout`.

---

## Test Cases

### Test Case 1 — Five requests, basic demo

After heapify the root is `ID = 104` with priority **90** (2 heapify steps, same
depth as Test Case 2). One `dispatchNext` call dispatches ID 104.
`updatePriority(103, 72)` lifts request 103 upward (sifted **UP**).
`updatePriority(999, 50)` is correctly rejected because ID 999 is not present
in the heap. `topKRequests(2)` both prints and returns `{102, 103}` without
touching the live heap. The record list is then sorted descending by
`"priority"` with the full pivot / partition trace printed.

### Test Case 2 — Five requests, rejection cases

Demonstrates `dispatchNext`, a successful "up" `updatePriority(203, 88)`,
and two **invalid** `updatePriority` calls — unknown ID `999` and
out-of-range priority `250` — both of which are correctly rejected with the
heap left unchanged. `topKRequests(2)` returns and prints the top two.
Finally the record list is sorted descending by priority.


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
