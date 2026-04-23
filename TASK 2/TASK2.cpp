#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>

using namespace std;


/*
this struct represents one aid request
1) requestID  -> unique identifier of the request
2) priority   -> integer in [0, 100] , higher means more urgent
3) timestamp  -> submission time of the request
*/
struct Request{
    int requestID;
    int priority;
    int timestamp;
};



/*
this CLASS contains the code for:
1) validating the input request list
2) building the max-heap using bottom-up heapify
3) dispatching the highest-priority request
4) updating the priority of an existing request in-place
5) sorting the records in-place using randomised quicksort (descending order)
6) returning the top k highest-priority requests without touching the live heap

data members:
1) heap -> the max-heap stored inside a vector , treated as a 1D array
   parent = (i-1)/2 , leftChild = 2i+1 , rightChild = 2i+2

helper functions:
1) swapNodes            -> swaps two heap entries
2) siftUp , siftDown    -> restore heap property after a single change
3) findIndexByID        -> linear scan , returns the heap index of a given ID
4) getKey               -> returns priority or timestamp (used by quicksort)
5) quicksortHelper      -> recursive randomised quicksort with lomuto partition
6) printHeap            -> prints the heap state after every modification
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


        //FOR SIFTING A NODE UP WHILE IT IS GREATER THAN ITS PARENT
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


        //FOR SIFTING A NODE DOWN WHILE IT IS SMALLER THAN ONE OF ITS CHILDREN
        //n -> effective size of the heap
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


        //FOR FINDING THE HEAP INDEX OF A REQUEST (LINEAR SCAN)
        //returns -1 if not found
        int findIndexByID(int reqID){
            for(int i=0; i<(int)heap.size(); i++){
                if(heap[i].requestID == reqID)
                    return i;
            }
            return -1;
        }


        //this function returns the sort key (priority or timestamp)
        int getKey(Request& r, string& key){
            if(key == "priority")
                return r.priority;
            else
                return r.timestamp;
        }


        //RECURSIVE RANDOMISED QUICKSORT WITH LOMUTO PARTITION (DESCENDING ORDER)
        //elements >= pivot are pushed to the front , thus result is descending
        //purely in-place , only the recursion stack is used as extra space
        void quicksortHelper(vector<Request>& arr, int lo, int hi, string& key, int& partitionCount){
            //base case
            if(lo >= hi)
                return;

            //pick a random pivot and move it to arr[hi]
            int pivIdx = lo + (rand() % (hi - lo + 1));
            Request t1 = arr[pivIdx];
            arr[pivIdx] = arr[hi];
            arr[hi] = t1;

            int pivotVal = getKey(arr[hi], key);
            cout<<"pivot = "<<pivotVal<<" (ID = "<<arr[hi].requestID
                <<"), range = ["<<lo<<", "<<hi<<"]"<<endl;

            //lomuto partition for descending order
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

            //place the pivot at its final position
            int p = i + 1;
            Request t2 = arr[p];
            arr[p] = arr[hi];
            arr[hi] = t2;

            partitionCount++;
            cout<<"   -> partition boundary at index "<<p<<endl;

            //recurse on the two halves
            quicksortHelper(arr, lo, p - 1, key, partitionCount);
            quicksortHelper(arr, p + 1, hi, key, partitionCount);
        }


        //this function prints the heap as an array
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
        1) every requestID inside one input batch must be unique
        2) priority must be an integer in [0, 100]
        3) duplicate request IDs are rejected at input time , with the offending ID reported
        4) out-of-range priorities are rejected at input time , with the offending value reported
        5) updatePriority rejects unknown IDs and out-of-range new priorities ,
           the heap is left unchanged in both cases
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


        //FOR BUILDING THE MAX-HEAP USING BOTTOM-UP HEAPIFY (FLOYD'S METHOD)
        //START FROM LAST NON-LEAF NODE AND WALK BACKWARDS TO THE ROOT
        //OVERALL TIME IS O(n) , BETTER THAN O(n log n) OF REPEATED INSERTION
        void buildStructure(vector<Request>& requests){
            cout<<endl;
            cout<<"BUILDING MAX-HEAP USING BOTTOM-UP HEAPIFY"<<endl;

            heap = requests;
            int n = heap.size();

            //start from the last non-leaf node and walk backwards
            int start = n / 2 - 1;

            for(int i=start; i>=0; i--){
                cout<<"heapify at index "<<i<<" (ID = "<<heap[i].requestID
                    <<", priority = "<<heap[i].priority<<")"<<endl;

                //sift-down with tracing
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


        //FOR DISPATCHING THE HIGHEST PRIORITY REQUEST (ROOT OF THE HEAP)
        //REPLACE ROOT WITH LAST ELEMENT , SHRINK THE HEAP , SIFT DOWN TO RESTORE
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


        //FOR UPDATING THE PRIORITY OF AN EXISTING REQUEST IN-PLACE (NO REBUILD)
        //LOCATE BY ID -> OVERWRITE PRIORITY -> SIFT UP OR DOWN
        void updatePriority(int reqID, int newPriority){
            cout<<endl;
            cout<<"UPDATE PRIORITY"<<endl;

            //rule (5) - range check
            if(newPriority < 0 || newPriority > 100){
                cout<<"ERROR -> new priority "<<newPriority
                    <<" is outside the valid range [0, 100]"<<endl;
                return;
            }

            //rule (5) - find the request by ID
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
                //no change in priority , thus no movement
                cout<<"  -> no movement (priority unchanged)"<<endl;
            }

            printHeap();
        }


        //FOR SORTING RECORDS IN-PLACE BY "priority" OR "timestamp" (DESCENDING ORDER)
        //RANDOMISED QUICKSORT WITH LOMUTO PARTITION
        //AVERAGE TIME O(n log n) , O(log n) AUXILIARY SPACE FROM RECURSION STACK ONLY
        void sortRecords(vector<Request>& records, string key){
            cout<<endl;
            cout<<"SORTING RECORDS BY \""<<key<<"\" USING RANDOMISED QUICKSORT (DESCENDING ORDER)"<<endl;

            if(key != "priority" && key != "timestamp"){
                cout<<"ERROR -> sort key must be either \"priority\" or \"timestamp\""<<endl;
                return;
            }

            srand(42);       //fixed seed , trace will be same every run
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


        //FOR RETURNING THE k HIGHEST PRIORITY REQUESTS WITHOUT TOUCHING THE LIVE HEAP
        //we work on a local copy tempHeap and extract the max k times from it
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

            //work on a copy , live heap is not touched
            vector<Request> tempHeap = heap;
            int n = tempHeap.size();

            for(int t=0; t<k; t++){
                //current max is at index 0
                result.push_back(tempHeap[0]);
                cout<<"   "<<(t+1)<<". ID = "<<tempHeap[0].requestID
                    <<", Priority = "<<tempHeap[0].priority<<endl;

                //replace root with last element and sift down
                if(n > 1){
                    tempHeap[0] = tempHeap[n-1];
                    n--;

                    //local sift-down on tempHeap
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

    //TEST CASE 1 - basic demo , five requests
    cout<<"TEST CASE 1\n";

    vector<Request> reqs1 = {
        {101, 45, 1}, {102, 80, 2}, {103, 35, 3},
        {104, 90, 4}, {105, 60, 5}
    };

    DispatchCentre dc1;

    if(dc1.validateInput(reqs1)){
        dc1.buildStructure(reqs1);

        //dispatch the highest-priority request
        dc1.dispatchNext();

        //raise a low-priority request , should move UP
        dc1.updatePriority(103, 72);

        //try to update an ID that is not present (should be rejected)
        dc1.updatePriority(999, 50);

        //peek at the top 2 without modifying the live heap
        vector<Request> top2 = dc1.topKRequests(2);
        cout<<"   (returned vector contains "<<top2.size()<<" requests)"<<endl;

        //sort by priority , descending
        vector<Request> sortCopy1 = reqs1;
        dc1.sortRecords(sortCopy1, "priority");

        //sort by timestamp , descending
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

    return 0;
}
