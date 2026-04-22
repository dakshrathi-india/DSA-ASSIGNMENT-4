#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>       //for rand and srand

using namespace std;


/*
this struct represents one aid request
1) requestID  -> unique identifier of the request
2) priority   -> integer in [0, 100] , higher means more urgent
3) timestamp  -> submission time of the request

requests are stored inside a max-heap keyed on the priority field
*/
struct Request{
    int requestID;
    int priority;
    int timestamp;
};



/*
this CLASS contains the code for:
1) validating the input request list against the data handling policy
2) building a max-heap from the input using bottom-up heapify
3) dispatching the highest-priority request and restoring the heap property
4) updating the priority of an existing request , in-place (no rebuild)
5) sorting the records in-place using randomised quicksort (by priority or timestamp)
6) returning the top k highest-priority requests WITHOUT modifying the live heap

data members:
1) heap -> the max-heap is stored inside a vector which we treat as a 1D array
          array based indexing: parent = (i-1)/2 , leftChild = 2i+1 , rightChild = 2i+2

helper functions (private):
1) swapNodes                 -> swaps two heap entries
2) siftUp , siftDown         -> restore heap property after a single change
3) findIndexByID             -> linear scan that returns the heap index of a request given its ID
4) getKey                    -> returns priority or timestamp (used by quicksort)
5) quicksortHelper           -> recursive randomised quicksort with Lomuto partition
6) printHeap                 -> prints the heap as an array (used after every modification)

SORT ORDER CONVENTION (sortRecords):
    sortRecords sorts in DESCENDING order of the chosen key
    (highest priority first for "priority" , latest timestamp first for "timestamp")
    this matches the natural reading of a situation report at shift-end
    enforced by the comparison (val >= pivotVal) inside the Lomuto partition

TOP-K CONVENTION (topKRequests):
    topKRequests returns a vector<Request> of the k highest-priority requests in decreasing order
    the live heap (data member heap) is NEVER modified
    we achieve this by copying heap into a local tempHeap and extracting the max k times from the copy
*/
class DispatchCentre{
    private:
        vector<Request> heap;


        //swap heap[i] and heap[j]
        void swapNodes(int i, int j){
            Request temp = heap[i];
            heap[i] = heap[j];
            heap[j] = temp;
        }


        //bubble heap[i] upwards as long as it is greater than its parent
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


        //push heap[i] downwards as long as one of its children is greater than it
        //n -> effective size of the heap (so we don't read past the live region)
        //used after replacing the root with the last element , or after a priority decrease
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


        //LINEAR SCAN , returns the heap index of the request with the given ID
        //returns -1 if no such request is present
        //we do NOT maintain a separate ID -> index lookup table , this is a deliberate simplicity choice
        int findIndexByID(int reqID){
            for(int i=0; i<(int)heap.size(); i++){
                if(heap[i].requestID == reqID)
                    return i;
            }
            return -1;
        }


        //returns the comparison key for one Request based on the sort key string
        //used inside quicksortHelper
        int getKey(Request& r, string& key){
            if(key == "priority")
                return r.priority;
            else
                return r.timestamp;
        }


        /*
        recursive RANDOMISED QUICKSORT using LOMUTO partition
        - pick a random pivot index in [lo, hi]
        - swap the pivot to arr[hi]
        - scan from left to right and push elements GREATER THAN OR EQUAL TO the pivot to the front
          (this is why the sort ends up in DESCENDING order)
        - place the pivot in its final position and recurse on the two halves

        purely IN-PLACE , no auxiliary array of size n is allocated
        extra space is just the recursion stack , O(log n) on average

        partitionCount is incremented at every partition step (only for the trace)
        */
        void quicksortHelper(vector<Request>& arr, int lo, int hi, string& key, int& partitionCount){
            //base case
            if(lo >= hi)
                return;

            //pick a random pivot index in [lo, hi] and move it to arr[hi]
            int pivIdx = lo + (rand() % (hi - lo + 1));
            Request t1 = arr[pivIdx];
            arr[pivIdx] = arr[hi];
            arr[hi] = t1;

            int pivotVal = getKey(arr[hi], key);
            cout<<"pivot = "<<pivotVal<<" (ID = "<<arr[hi].requestID
                <<"), range = ["<<lo<<", "<<hi<<"]"<<endl;

            //LOMUTO PARTITION for DESCENDING order
            //we push elements >= pivot to the front of the window [lo, hi-1]
            int i = lo - 1;
            for(int j=lo; j<hi; j++){
                int val = getKey(arr[j], key);
                if(val >= pivotVal){
                    i++;
                    Request t = arr[i];
                    arr[i] = arr[j];
                    arr[j] = t;
                }
            }

            //place the pivot at its final position (i+1)
            int p = i + 1;
            Request t2 = arr[p];
            arr[p] = arr[hi];
            arr[hi] = t2;

            partitionCount++;
            cout<<"   -> partition boundary at index "<<p<<endl;

            //recurse on the two halves , pivot is already at its final position p
            quicksortHelper(arr, lo, p - 1, key, partitionCount);
            quicksortHelper(arr, p + 1, hi, key, partitionCount);
        }


        //prints the current heap as an array (used after every modification for traceability)
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
        DATA HANDLING POLICY:
        1) every request ID inside one input batch must be unique
        2) priority must be an integer in [0, 100]
        3) duplicate request IDs are rejected at input time , with the offending ID reported
        4) out-of-range priorities are rejected at input time , with the offending value reported
        5) inside updatePriority -> unknown ID and out-of-range new priority are both rejected
           with a clear error message , the heap is left unchanged in both cases
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
                //O(n^2) double-loop scan over earlier requests , fine for assignment-size inputs
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
        builds the max-heap from the input list using BOTTOM-UP HEAPIFY (Floyd's method)
        - copy the input into the heap array
        - start from the last non-leaf node (index n/2 - 1) and walk backwards to the root
        - at every step , sift the current node downwards so that the subtree rooted at it is a valid max-heap

        overall time is O(n) , much better than the O(n log n) of repeated insertion
        every internal swap is printed for traceability
        */

        void buildStructure(vector<Request>& requests){
            cout<<endl;
            cout<<"BUILDING MAX-HEAP USING BOTTOM-UP HEAPIFY"<<endl;

            //copy the input into the heap array
            heap = requests;
            int n = heap.size();

            //last non-leaf node is at index n/2 - 1 , start there and walk backwards
            int start = n / 2 - 1;

            for(int i=start; i>=0; i--){
                cout<<"heapify at index "<<i<<" (ID = "<<heap[i].requestID
                    <<", priority = "<<heap[i].priority<<")"<<endl;

                //verbose sift-down that prints every swap
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
        removes and returns the highest-priority request (root of the heap)
        steps:
        1) save the root in a local variable (this will be returned)
        2) replace heap[0] with the last element
        3) shrink the heap by one
        4) sift the new root down to restore the heap

        returns the sentinel Request {-1, -1, -1} if the heap is empty
        */

        Request dispatchNext(){
            cout<<endl;
            cout<<"DISPATCH NEXT"<<endl;

            if(heap.empty()){
                cout<<"ERROR -> dispatch queue is empty , nothing to dispatch"<<endl;
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
        updates the priority of an existing request IN-PLACE (no rebuild)
        steps:
        1) validate the new priority against [0, 100] (rule 5)
        2) locate the request by ID via findIndexByID (rule 5)
        3) save the old priority , overwrite with the new value
        4) if newPriority > oldPriority -> sift UP
           if newPriority < oldPriority -> sift DOWN
           if equal                     -> no movement needed

        both invalid conditions (unknown ID , out-of-range priority) leave the heap unchanged
        */

        void updatePriority(int reqID, int newPriority){
            cout<<endl;
            cout<<"UPDATE PRIORITY"<<endl;

            //rule (5) - new priority range check
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
        sorts the records IN-PLACE in DESCENDING order of the chosen key
        - key = "priority"   -> highest priority first
        - key = "timestamp"  -> latest timestamp first

        implementation uses randomised quicksort with Lomuto partition
        - average time O(n log n) , worst case O(n^2)
        - auxiliary space O(log n) from the recursion stack ONLY (no auxiliary array of size n)

        records are passed BY REFERENCE so the caller's vector is modified directly
        a fixed seed is used so that the pivot/partition trace is reproducible
        */

        void sortRecords(vector<Request>& records, string key){
            cout<<endl;
            cout<<"SORTING RECORDS BY \""<<key<<"\" USING RANDOMISED QUICKSORT (DESCENDING ORDER)"<<endl;

            if(key != "priority" && key != "timestamp"){
                cout<<"ERROR -> sort key must be either \"priority\" or \"timestamp\""<<endl;
                return;
            }

            srand(42);       //fixed seed for reproducibility of the trace
            int partitionCount = 0;

            quicksortHelper(records, 0, (int)records.size() - 1, key, partitionCount);

            cout<<endl;
            cout<<"total partitions performed -> "<<partitionCount<<endl;
            cout<<"sorted result (descending on "<<key<<"):"<<endl;
            for(int i=0; i<(int)records.size(); i++){
                cout<<"   ID = "<<records[i].requestID<<", priority = "<<records[i].priority
                    <<", timestamp = "<<records[i].timestamp<<endl;
            }
        }


        /*
        returns the k highest-priority requests in DECREASING order of priority
        the live heap (data member heap) is NOT modified

        mechanism:
        1) copy the live heap into a local tempHeap
        2) run k extract-max operations on tempHeap
        3) collect the extracted elements into the returned result vector
        4) also print the same list (problem statement asks for both)

        edge cases:
        - k <= 0                -> empty vector is returned , a note is printed
        - k > current heap size -> k is clamped to heap.size() , a note is printed
        */

        vector<Request> topKRequests(int k){
            cout<<endl;
            cout<<"TOP "<<k<<" REQUESTS"<<endl;

            vector<Request> result;

            if(k <= 0){
                cout<<"k <= 0 , nothing to return"<<endl;
                return result;
            }
            if(k > (int)heap.size()){
                cout<<"note -> k = "<<k<<" exceeds current heap size ("<<heap.size()
                    <<"), returning all available"<<endl;
                k = (int)heap.size();
            }

            //work on a COPY so the live heap is not modified
            vector<Request> tempHeap = heap;
            int n = tempHeap.size();

            for(int t=0; t<k; t++){
                //the current max sits at index 0 of tempHeap
                result.push_back(tempHeap[0]);
                cout<<"   "<<(t+1)<<". ID = "<<tempHeap[0].requestID
                    <<", Priority = "<<tempHeap[0].priority<<endl;

                //remove it by replacing root with the last element and sifting down
                if(n > 1){
                    tempHeap[0] = tempHeap[n-1];
                    n--;

                    //local sift-down on tempHeap (siftDown helper operates on this->heap so we redo it here)
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

            return result;
        }
};



int main(){

    //TEST CASE 1 - full demo , six requests
    cout<<"TEST CASE 1\n";

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

        //raise a low-priority request , should move UP
        dc1.updatePriority(103, 95);

        //lower a higher-priority request , should move DOWN
        dc1.updatePriority(106, 20);

        //peek at the top 3 without modifying the live heap
        vector<Request> top3 = dc1.topKRequests(3);
        cout<<"   (returned vector contains "<<top3.size()<<" requests)"<<endl;

        //sort an independent copy by priority , descending
        vector<Request> sortCopy1 = reqs1;
        dc1.sortRecords(sortCopy1, "priority");

        //sort an independent copy by timestamp , descending
        vector<Request> sortCopy2 = reqs1;
        dc1.sortRecords(sortCopy2, "timestamp");
    }


    //TEST CASE 2 - updatePriority rejection cases
    cout<<"\n\nTEST CASE 2\n";

    vector<Request> reqs2 = {
        {201, 50, 10}, {202, 70, 11}, {203, 30, 12},
        {204, 95, 13}, {205, 85, 14}
    };

    DispatchCentre dc2;

    if(dc2.validateInput(reqs2)){
        dc2.buildStructure(reqs2);

        dc2.dispatchNext();

        //bump a low-priority request up
        dc2.updatePriority(203, 88);

        //try to update an ID that is not present (should be rejected)
        dc2.updatePriority(999, 70);

        //try to set an out-of-range priority (should be rejected)
        dc2.updatePriority(202, 250);

        vector<Request> top2 = dc2.topKRequests(2);
        cout<<"   (returned vector contains "<<top2.size()<<" requests)"<<endl;

        vector<Request> sortCopy3 = reqs2;
        dc2.sortRecords(sortCopy3, "priority");
    }


    //TEST CASE 3 - duplicate ID , rejected at validation
    cout<<"\n\nTEST CASE 3 (duplicate ID)\n";

    vector<Request> reqs3 = {
        {301, 50, 1},
        {301, 80, 2}          //duplicate ID
    };

    DispatchCentre dc3;
    dc3.validateInput(reqs3);


    //TEST CASE 4 - out-of-range priority , rejected at validation
    cout<<"\n\nTEST CASE 4 (out-of-range priority)\n";

    vector<Request> reqs4 = {
        {401, 50, 1},
        {402, 150, 2}         //priority > 100
    };

    DispatchCentre dc4;
    dc4.validateInput(reqs4);

    return 0;
}
