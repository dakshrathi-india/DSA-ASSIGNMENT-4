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
this class is the max-heap for storing Request objects
keyed on the priority field
*/
class MaxHeap{
    private:
        vector<Request> data;

    public:

        MaxHeap(){}

        int size(){
            return data.size();
        }

        bool empty(){
            return data.empty();
        }

        Request& top(){
            return data[0];
        }

        Request& at(int i){
            return data[i];
        }

        void popBack(){
            data.pop_back();
        }

        //returns a copy of the internal data (used by topKRequests)
        vector<Request> getData(){
            return data;
        }

        //swap data[i] and data[j]
        void swapNodes(int i, int j){
            Request temp = data[i];
            data[i] = data[j];
            data[j] = temp;
        }

        //FOR SIFTING A NODE UP WHILE IT IS GREATER THAN ITS PARENT
        void siftUp(int i){
            while(i > 0){
                int par = (i - 1) / 2;
                if(data[i].priority > data[par].priority){
                    swapNodes(i, par);
                    i = par;
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

                if(leftChild < n && data[leftChild].priority > data[largest].priority)
                    largest = leftChild;
                if(rightChild < n && data[rightChild].priority > data[largest].priority)
                    largest = rightChild;

                if(largest != i){
                    swapNodes(i, largest);
                    i = largest;
                }
                else
                    break;
            }
        }

        //FOR FINDING THE INDEX OF A REQUEST GIVEN ITS ID (Linear scan)
        //returns -1 if not found
        int findByID(int reqID){
            int n = data.size();
            for(int i=0; i<n; i++){
                if(data[i].requestID == reqID)
                    return i;
            }
            return -1;
        }

        //this function prints the heap as an array
        void printState(){
            cout<<"   heap state -> [";
            int n = data.size();
            for(int i=0; i<n; i++){
                cout<<"(ID = "<<data[i].requestID<<", P = "<<data[i].priority<<")";
                if(i < n-1)
                    cout<<", ";
            }
            cout<<"]"<<endl;
        }

        //FOR BUILDING THE MAX-HEAP USING BOTTOM-UP HEAPIFY
        //START FROM LAST NON-LEAF NODE AND WALK BACKWARDS TO THE ROOT
        //OVERALL TIME IS O(n) , BETTER THAN O(n log n) OF REPEATED INSERTION
        void buildHeap(vector<Request>& requests){
            cout<<endl;
            cout<<"BUILDING MAX-HEAP USING BOTTOM-UP HEAPIFY"<<endl;

            data = requests;
            int n = size();

            //start from the last non-leaf node and walk backwards
            int start = n / 2 - 1;

            for(int i=start; i>=0; i--){
                cout<<"heapify at index "<<i<<" (ID = "<<at(i).requestID
                    <<", priority = "<<at(i).priority<<")"<<endl;

                //sift-down with tracing
                int pos = i;
                while(true){
                    int largest = pos;
                    int leftChild = 2 * pos + 1;
                    int rightChild = 2 * pos + 2;

                    if(leftChild < n && at(leftChild).priority > at(largest).priority)
                        largest = leftChild;

                    if(rightChild < n && at(rightChild).priority > at(largest).priority)
                        largest = rightChild;

                    if(largest != pos){
                        cout<<"   swap -> ID = "<<at(pos).requestID<<" (P = "<<at(pos).priority
                            <<")  with  ID = "<<at(largest).requestID
                            <<" (P = "<<at(largest).priority<<")"<<endl;
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
            printState();
        }
};



/*
this CLASS contains the code for:
1) validating the input request list
2) building the max-heap using bottom-up heapify
3) dispatching the highest-priority request
4) updating the priority of an existing request in-place
5) sorting the records in-place using randomised quicksort (descending order)
6) returning the top k highest-priority requests without touching the live heap
*/
class DispatchCentre{
    private:
        MaxHeap heap;


        //this function returns the sort key (priority or timestamp)
        int getKey(Request& r, string& key){
            if(key == "priority")
                return r.priority;
            else
                return r.timestamp;
        }


        //FOR REVERSING AN ARRAY IN PLACE
        void reverseArr(vector<Request>& arr){
            int lo = 0;
            int hi = arr.size() - 1;
            while(lo < hi){
                Request temp = arr[lo];
                arr[lo] = arr[hi];
                arr[hi] = temp;
                lo++;
                hi--;
            }
        }


        //RECURSIVE RANDOMISED QUICKSORT (ASCENDING ORDER)
        //after this we reverse the array to get descending order
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
            int n = requests.size();
            for(int i=0; i<n; i++){
                Request& r = requests[i];

                //rule (3)
                for(int j=0; j<i; j++){
                    if(requests[j].requestID == r.requestID){
                        cout<<"INVALID -> duplicate request ID "<<r.requestID
                            <<" found at position "<<i<<endl;
                        return false;
                    }
                }

                //rule (4)
                if(r.priority < 0 || r.priority > 100){
                    cout<<"INVALID -> request ID "<<r.requestID<<" has priority "<<r.priority
                        <<" (must lie in [0, 100])"<<endl;
                    return false;
                }
            }

            cout<<"all "<<requests.size()<<" requests passed validation"<<endl;
            return true;
        }


        //FOR BUILDING THE MAX-HEAP USING BOTTOM-UP HEAPIFY
        void buildStructure(vector<Request>& requests){
            heap.buildHeap(requests);
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

            Request top = heap.top();
            cout<<"dispatched -> ID = "<<top.requestID<<", Priority = "<<top.priority
                <<", Timestamp = "<<top.timestamp<<endl;

            int n = heap.size();
            if(n > 1){
                heap.at(0) = heap.at(n-1);
                heap.popBack();
                heap.siftDown(0, heap.size());
            }
            else{
                heap.popBack();
            }

            cout<<"after restoration:"<<endl;
            heap.printState();
            return top;
        }


        //FOR UPDATING THE PRIORITY OF AN EXISTING REQUEST IN-PLACE
        //LOCATE BY ID -> OVERWRITE PRIORITY -> SIFT UP OR DOWN
        void updatePriority(int reqID, int newPriority){
            cout<<endl;
            cout<<"UPDATE PRIORITY"<<endl;

            if(newPriority < 0 || newPriority > 100){
                cout<<"ERROR -> new priority "<<newPriority
                    <<" is outside the valid range [0, 100]"<<endl;
                return;
            }

            int idx = heap.findByID(reqID);
            if(idx == -1){
                cout<<"ERROR -> request ID "<<reqID<<" is not present in the heap"<<endl;
                return;
            }

            int oldPriority = heap.at(idx).priority;
            heap.at(idx).priority = newPriority;

            cout<<"ID = "<<reqID<<" : old priority = "<<oldPriority
                <<", new priority = "<<newPriority;

            if(newPriority > oldPriority){
                cout<<"  -> moving UP"<<endl;
                heap.siftUp(idx);
            }
            else if(newPriority < oldPriority){
                cout<<"  -> moving DOWN"<<endl;
                heap.siftDown(idx, heap.size());
            }
            else{
                //no change in priority , thus no movement
                cout<<"  -> no movement (priority unchanged)"<<endl;
            }

            heap.printState();
        }


        //FOR SORTING RECORDS IN-PLACE BY "priority" OR "timestamp" (DESCENDING ORDER)
        void sortRecords(vector<Request>& records, string key){
            cout<<endl;
            cout<<"SORTING RECORDS BY \""<<key<<"\" USING RANDOMISED QUICKSORT (DESCENDING ORDER)"<<endl;

            if(key != "priority" && key != "timestamp"){
                cout<<"ERROR -> sort key must be either \"priority\" or \"timestamp\""<<endl;
                return;
            }

            srand(42);       //fixed seed
            int partitionCount = 0;

            quicksortHelper(records, 0, records.size()-1, key, partitionCount);
            //reverse in-place to convert ascending result to descending
            reverseArr(records);

            cout<<endl;
            cout<<"total partitions performed -> "<<partitionCount<<endl;
            cout<<"sorted result (descending on "<<key<<"):"<<endl;
            int n = records.size();
            for(int i=0; i<n; i++){
                cout<<"   ID = "<<records[i].requestID<<", priority = "<<records[i].priority
                    <<", timestamp = "<<records[i].timestamp<<endl;
            }
        }


        //FOR RETURNING THE k HIGHEST PRIORITY REQUESTS
        vector<Request> topKRequests(int k){
            cout<<endl;
            cout<<"TOP "<<k<<" REQUESTS"<<endl;

            vector<Request> result;

            if(k <= 0){
                cout<<"k <= 0 , nothing to return"<<endl;
                return result;
            }
            int heapSz = heap.size();
            if(k > heapSz){
                cout<<"note -> k = "<<k<<" exceeds current heap size ("<<heapSz
                    <<"), returning all available"<<endl;
                k = heapSz;
            }

            MaxHeap tempHeap = heap;
            int n = tempHeap.size();

            for(int t=0; t<k; t++){
                //current max is at index 0
                result.push_back(tempHeap.top());
                cout<<"   "<<(t+1)<<". ID = "<<tempHeap.top().requestID
                    <<", Priority = "<<tempHeap.top().priority<<endl;

                //replace root with last element and sift down
                if(n > 1){
                    tempHeap.at(0) = tempHeap.at(n-1);
                    n--;
                    tempHeap.siftDown(0, n);
                }
                else{
                    n--;
                }
            }

            return result;
        }
};



int main(){

    //TEST CASE 1
    cout<<"TEST CASE 1\n";

    vector<Request> reqs1 = {
        {101, 45, 1}, {102, 80, 2}, {103, 35, 3},
        {104, 90, 4}, {105, 60, 5}
    };

    DispatchCentre dc1;

    if(dc1.validateInput(reqs1)){
        dc1.buildStructure(reqs1);
        dc1.dispatchNext();
        dc1.updatePriority(103, 72);
        dc1.updatePriority(999, 50);

        vector<Request> top2 = dc1.topKRequests(2);
        cout<<"   (returned vector contains "<<top2.size()<<" requests)"<<endl;

        vector<Request> sortCopy1 = reqs1;
        dc1.sortRecords(sortCopy1, "priority");

        vector<Request> sortCopy2 = reqs1;
        dc1.sortRecords(sortCopy2, "timestamp");
    }


    //TEST CASE 2
    cout<<"\n\nTEST CASE 2\n";

    vector<Request> reqs2 = {
        {201, 50, 10}, {202, 70, 11}, {203, 30, 12},
        {204, 95, 13}, {205, 85, 14}
    };

    DispatchCentre dc2;

    if(dc2.validateInput(reqs2)){
        dc2.buildStructure(reqs2);
        dc2.dispatchNext();
        dc2.updatePriority(203, 88);
        dc2.updatePriority(999, 70);
        dc2.updatePriority(202, 250);

        vector<Request> top2 = dc2.topKRequests(2);
        cout<<"   (returned vector contains "<<top2.size()<<" requests)"<<endl;

        vector<Request> sortCopy3 = reqs2;
        dc2.sortRecords(sortCopy3, "priority");
    }

    return 0;
}