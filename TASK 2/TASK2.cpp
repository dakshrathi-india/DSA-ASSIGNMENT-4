#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>          //for rand and srand

using namespace std;


/*
this struct represents a single aid request
1) requestID  -> unique identifier of the request
2) priority   -> integer in [0, 100], higher value means more urgent
3) timestamp  -> the submission time of the request

the dispatch centre stores all the live requests inside a max-heap keyed on the priority field
*/
struct Request {
    int requestID;
    int priority;
    int timestamp;
};


/*
this CLASS contains the code for:
1) validating the input request list against the data handling policy
2) building the max-heap from a list of requests using bottom-up heapify
3) dispatching the highest-priority request and restoring the heap property
4) updating the priority of an existing request in-place
5) sorting the records in-place using randomised quicksort (works on either priority or timestamp)
6) returning the top-k highest-priority requests without modifying the live heap

private section contains the helper functions:
1) swapNodes - swaps two heap entries
2) siftUp / siftDown - the two routines that restore the heap property
3) findIndexByID - linear scan that returns the heap index of a request given its ID
4) getKey - returns the priority or timestamp of a request (used inside quicksort)
5) quicksortHelper - the recursive randomised quicksort with Lomuto partition
6) printHeap - prints the heap as an array (used after every modification for traceability)

public section contains the 6 required operations exactly as named in the task statement
*/
class DispatchCentre{
    private:
        //the actual max-heap is stored inside a vector that we use as a 1D array
        //array based indexing - parent = (i-1)/2, leftChild = 2i+1, rightChild = 2i+2
        vector<Request> heap;


        //swaps heap[i] and heap[j]
        void swapNodes(int i, int j){
            Request temp = heap[i];
            heap[i] = heap[j];
            heap[j] = temp;
        }


        //bubble heap[i] upwards while it is larger than its parent
        //used after a priority increase
        void siftUp(int i){
            while(i > 0){
                int parent = (i - 1) / 2;
                if(heap[i].priority > heap[parent].priority){
                    swapNodes(i, parent);
                    i = parent;
                }
                else
                    break;
            }
        }


        //push heap[i] downwards while it is smaller than one of its children
        //n -> the effective size of the heap (so we don't read past the live region)
        //used after replacing the root with the last element, or after a priority decrease
        void siftDown(int i, int n){
            while(true){
                int largest = i;
                int leftChild = 2 * i + 1;
                int rightChild = 2 * i + 2;

                if(leftChild < n && heap[leftChild].priority > heap[largest].priority)
                    largest = leftChild;

                if(rightChild < n && heap[rightChild].priority > heap[largest].priority)
                    largest = rightChild;

                if(largest != i){
                    swapNodes(i, largest);
                    i = largest;
                }
                else
                    break;
            }
        }


        //LINEAR SCAN -> returns the heap index of the request with the given ID
        //returns -1 if no request with this ID is present in the heap
        //we are intentionally not maintaining a separate ID -> index lookup table
        int findIndexByID(int reqID){
            for(int i=0; i<(int)heap.size(); i++){
                if(heap[i].requestID == reqID)
                    return i;
            }
            return -1;
        }


        //returns the comparison key for one request based on the sort key
        //key == "priority"   -> we sort by priority
        //key == "timestamp"  -> we sort by timestamp
        int getKey(Request& r, string& key){
            if(key == "priority")
                return r.priority;
            else
                return r.timestamp;
        }


        /*
        recursive RANDOMISED QUICKSORT with LOMUTO PARTITION
        - we pick a random pivot index in [lo, hi]
        - swap the pivot to arr[hi]
        - partition the elements around the pivot
        - recurse on the two halves

        purely IN-PLACE -> the only extra space is the recursion stack of depth O(log n) on average
        partitionCount is incremented at every partition step (just for the trace)
        */
        void quicksortHelper(vector<Request>& arr, int lo, int hi, string& key, int& partitionCount){
            //base case
            if(lo >= hi)
                return;

            //pick a random pivot index in [lo, hi] and move that element to arr[hi]
            int pivIdx = lo + (rand() % (hi - lo + 1));
            Request t1 = arr[pivIdx];
            arr[pivIdx] = arr[hi];
            arr[hi] = t1;

            int pivotVal = getKey(arr[hi], key);
            cout<<"pivot = "<<pivotVal<<" (ID = "<<arr[hi].requestID
                <<"), range = ["<<lo<<", "<<hi<<"]"<<endl;

            //LOMUTO PARTITION
            //i tracks the boundary of the "less than or equal to pivot" region
            int i = lo - 1;
            for(int j=lo; j<hi; j++){
                int val = getKey(arr[j], key);
                if(val <= pivotVal){
                    i++;
                    Request t = arr[i];
                    arr[i] = arr[j];
                    arr[j] = t;
                }
            }

            //place the pivot in its final position (boundary + 1)
            int p = i + 1;
            Request t2 = arr[p];
            arr[p] = arr[hi];
            arr[hi] = t2;

            partitionCount++;
            cout<<"   -> partition boundary at index "<<p<<endl;

            //recurse on the two halves (the pivot itself sits at index p, already in its final place)
            quicksortHelper(arr, lo, p - 1, key, partitionCount);
            quicksortHelper(arr, p + 1, hi, key, partitionCount);
        }


        //prints the current heap state as an array (used after every modification)
        void printHeap(){
            cout<<"   heap state -> [";
            for(int i=0; i<(int)heap.size(); i++){
                cout<<"(ID = "<<heap[i].requestID<<", P = "<<heap[i].priority<<")";
                if(i < (int)heap.size() - 1)
                    cout<<", ";
            }
            cout<<"]"<<endl;
        }


    public:

        //default constructor
        DispatchCentre(){}


        /*
        data handling policy:
        1) every request ID inside one input batch must be unique
        2) priority must be an integer in [0, 100]
        3) duplicate request IDs are rejected at input time, with the offending ID printed
        4) out-of-range priorities are rejected at input time, with the offending value printed
        5) inside updatePriority -> unknown ID and out-of-range new priority are both rejected with a clear error and the heap is left unchanged
        */

        bool validateInput(vector<Request>& requests){
            cout<<endl;
            cout<<"DATA HANDLING POLICY"<<endl;
            cout<<"1) request IDs must be unique"<<endl;
            cout<<"2) priority must be an integer in [0, 100]"<<endl;
            cout<<"3) duplicate IDs are rejected"<<endl;
            cout<<"4) out-of-range priorities are rejected"<<endl;
            cout<<"5) updatePriority rejects unknown IDs and out-of-range new priorities"<<endl;
            cout<<endl;

            for(int i=0; i<(int)requests.size(); i++){
                Request& r = requests[i];

                //rule (3) - duplicate ID check
                //simple O(n^2) double-loop scan over all earlier requests
                for(int j=0; j<i; j++){
                    if(requests[j].requestID == r.requestID){
                        cout<<"INVALID -> duplicate request ID "<<r.requestID
                            <<" found at position "<<i<<endl;
                        return false;
                    }
                }

                //rule (4) - priority range check
                if(r.priority < 0 || r.priority > 100){
                    cout<<"INVALID -> request ID "<<r.requestID<<" has priority "<<r.priority
                        <<" (must lie in [0, 100])"<<endl;
                    return false;
                }
            }

            cout<<"all "<<requests.size()<<" requests passed validation"<<endl;
            return true;
        }


        /*
        builds the max-heap from the input request list using BOTTOM-UP HEAPIFY (Floyd's algorithm)
        - we copy the input requests into the heap array
        - we start from the last non-leaf node and walk backwards to the root
        - at each step we sift the current node down so that the subtree rooted at it
          satisfies the max-heap property

        this approach takes O(n) overall, which is better than O(n log n) of repeated insertion
        every internal sift-down step is printed for traceability
        */

        void buildStructure(vector<Request>& requests){
            cout<<endl;
            cout<<"BUILDING MAX-HEAP USING BOTTOM-UP HEAPIFY"<<endl;

            //copy data into the heap array
            heap = requests;
            int n = heap.size();

            //start from the last non-leaf node (index n/2 - 1) and walk backwards to the root
            int start = n / 2 - 1;

            for(int i=start; i>=0; i--){
                cout<<"heapify at index "<<i<<" (ID = "<<heap[i].requestID
                    <<", priority = "<<heap[i].priority<<")"<<endl;

                //we re-implement sift-down here so that we can print every swap as it happens
                int pos = i;
                while(true){
                    int largest = pos;
                    int leftChild = 2 * pos + 1;
                    int rightChild = 2 * pos + 2;

                    if(leftChild < n && heap[leftChild].priority > heap[largest].priority)
                        largest = leftChild;

                    if(rightChild < n && heap[rightChild].priority > heap[largest].priority)
                        largest = rightChild;

                    if(largest != pos){
                        cout<<"   swap -> ID = "<<heap[pos].requestID<<" (P = "<<heap[pos].priority
                            <<")  with  ID = "<<heap[largest].requestID
                            <<" (P = "<<heap[largest].priority<<")"<<endl;
                        swapNodes(pos, largest);
                        pos = largest;
                    }
                    else{
                        cout<<"   no swap needed at this level"<<endl;
                        break;
                    }
                }
            }

            cout<<endl;
            cout<<"heap construction complete"<<endl;
            printHeap();
        }


        /*
        removes and returns the highest-priority request (the root of the heap)
        steps:
        1) save the root in a local variable (this will be returned)
        2) move the last heap element to the root position
        3) shrink the heap by 1
        4) sift the new root down to restore the heap property

        a sentinel Request {-1, -1, -1} is returned if the heap is empty
        */

        Request dispatchNext(){
            cout<<endl;
            cout<<"DISPATCH NEXT"<<endl;

            if(heap.empty()){
                cout<<"ERROR -> dispatch queue is empty, nothing to dispatch"<<endl;
                Request none = {-1, -1, -1};
                return none;
            }

            Request top = heap[0];
            cout<<"dispatched -> ID = "<<top.requestID<<", Priority = "<<top.priority
                <<", Timestamp = "<<top.timestamp<<endl;

            int n = heap.size();
            if(n > 1){
                heap[0] = heap[n-1];
                heap.pop_back();
                siftDown(0, (int)heap.size());
            }
            else{
                heap.pop_back();
            }

            cout<<"after restoration:"<<endl;
            printHeap();
            return top;
        }


        /*
        updates the priority of an existing request IN-PLACE without rebuilding the heap
        steps:
        1) locate the request by ID using a linear scan (returns its current heap index)
        2) save the old priority and overwrite it with the new value
        3) if the new priority is greater than the old one  -> sift the node UP
           if the new priority is smaller than the old one  -> sift the node DOWN
           if equal -> no movement is needed

        invalid update conditions:
        - the request ID is not present in the heap
        - the new priority lies outside [0, 100]
        both produce a specific error and leave the heap unchanged
        */

        void updatePriority(int reqID, int newPriority){
            cout<<endl;
            cout<<"UPDATE PRIORITY"<<endl;

            //rule (5) - range check on new priority
            if(newPriority < 0 || newPriority > 100){
                cout<<"ERROR -> new priority "<<newPriority
                    <<" is outside the valid range [0, 100]"<<endl;
                return;
            }

            //rule (5) - locate the request by ID
            int idx = findIndexByID(reqID);
            if(idx == -1){
                cout<<"ERROR -> request ID "<<reqID<<" is not present in the heap"<<endl;
                return;
            }

            int oldPriority = heap[idx].priority;
            heap[idx].priority = newPriority;

            cout<<"ID = "<<reqID<<" : old priority = "<<oldPriority
                <<", new priority = "<<newPriority;

            if(newPriority > oldPriority){
                cout<<"  -> moving UP"<<endl;
                siftUp(idx);
            }
            else if(newPriority < oldPriority){
                cout<<"  -> moving DOWN"<<endl;
                siftDown(idx, (int)heap.size());
            }
            else{
                cout<<"  -> no movement (priority unchanged)"<<endl;
            }

            printHeap();
        }


        /*
        sorts the records IN-PLACE by either "priority" or "timestamp"
        - uses RANDOMISED QUICKSORT with Lomuto partition
        - average time = O(n log n)
        - auxiliary space = O(log n) on average (only the recursion stack, no extra arrays)
        - prints the chosen pivot and the partition boundary at every recursive call

        we pass records by reference so the original vector is modified directly
        a fixed seed is used so the trace is reproducible from one run to the next
        */

        void sortRecords(vector<Request>& records, string key){
            cout<<endl;
            cout<<"SORTING RECORDS BY \""<<key<<"\" USING RANDOMISED QUICKSORT"<<endl;

            if(key != "priority" && key != "timestamp"){
                cout<<"ERROR -> sort key must be either \"priority\" or \"timestamp\""<<endl;
                return;
            }

            srand(42);          //fixed seed -> reproducible trace
            int partitionCount = 0;

            quicksortHelper(records, 0, (int)records.size() - 1, key, partitionCount);

            cout<<endl;
            cout<<"total partitions performed -> "<<partitionCount<<endl;
            cout<<"sorted result:"<<endl;
            for(int i=0; i<(int)records.size(); i++){
                cout<<"   ID = "<<records[i].requestID<<", priority = "<<records[i].priority
                    <<", timestamp = "<<records[i].timestamp<<endl;
            }
        }


        /*
        returns the k highest-priority requests in decreasing order
        the LIVE heap is NOT modified -> we copy heap[] into a temporary buffer
        and run k extract-max operations on the copy

        edge cases:
        - if k <= 0 -> nothing is shown
        - if k > current heap size -> all available requests are shown
        */

        void topKRequests(int k){
            cout<<endl;
            cout<<"TOP "<<k<<" REQUESTS"<<endl;

            if(k <= 0){
                cout<<"k <= 0, nothing to display"<<endl;
                return;
            }
            if(k > (int)heap.size()){
                cout<<"note -> k = "<<k<<" exceeds heap size ("<<heap.size()
                    <<"), showing all available"<<endl;
                k = (int)heap.size();
            }

            //work on a COPY so the live heap is not touched
            vector<Request> tempHeap = heap;
            int n = tempHeap.size();

            for(int t=0; t<k; t++){
                //the current max sits at index 0 of the temp heap
                cout<<"   "<<(t+1)<<". ID = "<<tempHeap[0].requestID
                    <<", Priority = "<<tempHeap[0].priority<<endl;

                //remove the max from the temp heap by replacing root with the last element and sifting down
                if(n > 1){
                    tempHeap[0] = tempHeap[n-1];
                    n--;

                    //local sift-down on tempHeap (the helper siftDown operates on this->heap, not on tempHeap, so we redo it here)
                    int pos = 0;
                    while(true){
                        int largest = pos;
                        int leftChild = 2 * pos + 1;
                        int rightChild = 2 * pos + 2;

                        if(leftChild < n && tempHeap[leftChild].priority > tempHeap[largest].priority)
                            largest = leftChild;

                        if(rightChild < n && tempHeap[rightChild].priority > tempHeap[largest].priority)
                            largest = rightChild;

                        if(largest != pos){
                            Request t3 = tempHeap[pos];
                            tempHeap[pos] = tempHeap[largest];
                            tempHeap[largest] = t3;
                            pos = largest;
                        }
                        else
                            break;
                    }
                }
                else{
                    n--;
                }
            }
        }
};


int main(){

    //TEST CASE 1
    cout<<"=========================================="<<endl;
    cout<<"               TEST CASE 1                "<<endl;
    cout<<"==========================================";

    vector<Request> reqs1 = {
        {101, 45, 1}, {102, 80, 2}, {103, 35, 3},
        {104, 90, 4}, {105, 60, 5}, {106, 75, 6}
    };

    DispatchCentre dc1;

    if(dc1.validateInput(reqs1)){
        dc1.buildStructure(reqs1);

        //dispatch the two highest-priority requests
        dc1.dispatchNext();
        dc1.dispatchNext();

        //raise a low-priority request -> should move UP
        dc1.updatePriority(103, 95);

        //lower a higher-priority request -> should move DOWN
        dc1.updatePriority(106, 20);

        //peek at the top 3 without modifying the live heap
        dc1.topKRequests(3);

        //sort an independent copy by priority
        vector<Request> sortCopy1 = reqs1;
        dc1.sortRecords(sortCopy1, "priority");

        //sort an independent copy by timestamp
        vector<Request> sortCopy2 = reqs1;
        dc1.sortRecords(sortCopy2, "timestamp");
    }


    //TEST CASE 2
    cout<<endl<<endl;
    cout<<"=========================================="<<endl;
    cout<<"               TEST CASE 2                "<<endl;
    cout<<"==========================================";

    vector<Request> reqs2 = {
        {201, 50, 10}, {202, 70, 11}, {203, 30, 12},
        {204, 95, 13}, {205, 85, 14}
    };

    DispatchCentre dc2;

    if(dc2.validateInput(reqs2)){
        dc2.buildStructure(reqs2);

        dc2.dispatchNext();

        //bump up a low-priority request
        dc2.updatePriority(203, 88);

        //try to update a non-existent ID (should be rejected)
        dc2.updatePriority(999, 70);

        //try to set an out-of-range priority (should be rejected)
        dc2.updatePriority(202, 250);

        dc2.topKRequests(2);

        vector<Request> sortCopy3 = reqs2;
        dc2.sortRecords(sortCopy3, "priority");
    }


    //TEST CASE 3 (invalid input - duplicate ID, will be rejected by rule 3)
    cout<<endl<<endl;
    cout<<"=========================================="<<endl;
    cout<<"   TEST CASE 3 (duplicate ID)             "<<endl;
    cout<<"==========================================";

    vector<Request> reqs3 = {
        {301, 50, 1},
        {301, 80, 2}     //duplicate ID
    };

    DispatchCentre dc3;
    dc3.validateInput(reqs3);


    //TEST CASE 4 (invalid input - out-of-range priority, will be rejected by rule 4)
    cout<<endl<<endl;
    cout<<"=========================================="<<endl;
    cout<<"   TEST CASE 4 (out-of-range priority)    "<<endl;
    cout<<"==========================================";

    vector<Request> reqs4 = {
        {401, 50, 1},
        {402, 150, 2}    //priority > 100
    };

    DispatchCentre dc4;
    dc4.validateInput(reqs4);

    return 0;
}
