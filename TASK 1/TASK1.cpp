#include <iostream>
#include <vector>
#include <climits>

using namespace std;


/*
this struct represents one road in the city graph
each road carries two endpoints (u and v), a construction cost and a travel time
the graph is undirected, so the order of u and v doesn't matter

1) u, v       -> the two intersections the road connects
2) cost       -> construction cost of the road (used in MST)
3) travelTime -> time to travel on this road (used in shortest path)
*/
struct Edge{
    int u;
    int v;
    int cost;
    int travelTime;
};



/*
this CLASS contains the code for:
1) validating the input graph against the graph validation policy
2) building the MST using Kruskal's algorithm
3) finding the shortest travel-time path using Dijkstra's algorithm
4) comparing the MST-path travel time vs the shortest-path travel time
5) finding the critical roads in the MST

data members:
1) N              -> number of intersections
2) edges          -> list of all input roads (never modified after constructor)
3) mstEdges       -> roads accepted into the MST (filled by buildMST)
4) mstCost        -> total construction cost of the MST
5) adjFull        -> adjacency list of the full graph (weighted by travelTime)
6) adjMST         -> adjacency list of the MST only (weighted by travelTime)
7) mstBuilt       -> flag set to true once buildMST has been called successfully

helper functions (private):
1) initDSU, findParent, unionSets   -> DSU helpers used inside Kruskal's
2) sortEdgeIndicesByCost            -> selection sort on edge indices
3) dijkstra                         -> O(N^2) Dijkstra working on any adj list
4) reconstructPath                  -> rebuild S->T path using prev[] from dijkstra
5) getPathTravelTime                -> wrapper that returns the shortest time only
6) buildFullAdjacency               -> fills adjFull from edges (called in constructor)
7) buildMSTAdjacency                -> fills adjMST from mstEdges (called after MST build)
8) countReachableFromZero           -> BFS-based connectivity check used in validation

important design choice:
    adjFull is built in the CONSTRUCTOR itself, not inside buildMST
    this way shortestPath / compareNetworks do not need buildMST to have been called first
    (shortest path on the full graph is mathematically independent of MST construction,
    so the code structure reflects that)
*/
class CityGraph{
    private:
        int N;
        vector<Edge> edges;
        vector<Edge> mstEdges;
        int mstCost;

        vector<vector<pair<int,int>>> adjFull;
        vector<vector<pair<int,int>>> adjMST;

        bool mstBuilt;

        //DSU arrays , reused across buildMST call and the rebuild loop inside criticalRoads
        vector<int> parent;
        vector<int> setSize;


        //initialize the DSU , every node is its own set of size 1
        void initDSU(int n){
            parent.assign(n, 0);
            setSize.assign(n, 1);
            for(int i=0; i<n; i++)
                parent[i] = i;
        }


        //find the root of the set that contains x , uses path compression
        int findParent(int x){
            //base case
            if(parent[x] == x)
                return x;
            parent[x] = findParent(parent[x]);
            return parent[x];
        }


        //union the two sets by attaching the smaller one under the larger one (union by size)
        //returns true if a real union actually happened
        bool unionSets(int x, int y){
            int rootX = findParent(x);
            int rootY = findParent(y);

            if(rootX == rootY)
                return false;

            //attach the smaller tree under the larger one to keep the tree shallow
            if(setSize[rootX] < setSize[rootY]){
                parent[rootX] = rootY;
                setSize[rootY] += setSize[rootX];
            }
            else{
                parent[rootY] = rootX;
                setSize[rootX] += setSize[rootY];
            }
            return true;
        }


        //SELECTION SORT on the indices into edges[] , ascending on cost
        //we sort indices instead of the edges themselves so the original input is not disturbed
        void sortEdgeIndicesByCost(vector<int>& indexList){
            int n = indexList.size();
            for(int i=0; i<n-1; i++){
                int minIdx = i;
                for(int j=i+1; j<n; j++){
                    if(edges[indexList[j]].cost < edges[indexList[minIdx]].cost)
                        minIdx = j;
                }
                if(minIdx != i){
                    int temp = indexList[i];
                    indexList[i] = indexList[minIdx];
                    indexList[minIdx] = temp;
                }
            }
        }


        /*
        DIJKSTRA's algorithm , O(N^2) version
        - we don't use a priority queue , instead at every step we linearly scan dist[] to pick the unvisited node with the smallest distance
        - dist[]    -> shortest distance from S to every node , INT_MAX if unreachable
        - prev[]    -> predecessor pointer used later to reconstruct the path
        - visited[] -> tracks nodes which are already finalised
        */
        void dijkstra(vector<vector<pair<int,int>>>& adj, int S, vector<int>& dist, vector<int>& prev){
            dist.assign(N, INT_MAX);
            prev.assign(N, -1);
            vector<bool> visited(N, false);

            dist[S] = 0;

            //we will finalise at most N nodes
            for(int count=0; count<N; count++){

                //pick the unvisited node with the smallest current distance
                int u = -1;
                int minDist = INT_MAX;
                for(int i=0; i<N; i++){
                    if(!visited[i] && dist[i] < minDist){
                        minDist = dist[i];
                        u = i;
                    }
                }

                //no more reachable nodes , break out
                if(u == -1)
                    break;

                visited[u] = true;

                //relax all edges going out of u
                for(int i=0; i<(int)adj[u].size(); i++){
                    int v = adj[u][i].first;
                    int w = adj[u][i].second;

                    if(!visited[v] && dist[u] + w < dist[v]){
                        dist[v] = dist[u] + w;
                        prev[v] = u;
                    }
                }
            }
        }


        //reconstruct the path from S to T using the prev[] filled by dijkstra
        //returns an empty vector if T was unreachable
        vector<int> reconstructPath(int T, vector<int>& prev, vector<int>& dist){
            vector<int> path;

            //edge case
            if(dist[T] == INT_MAX)
                return path;

            //walk backwards from T using prev[] till we hit the source (prev == -1)
            int cur = T;
            while(cur != -1){
                path.push_back(cur);
                cur = prev[cur];
            }

            //reverse the path so that it goes S -> T
            int sz = path.size();
            for(int i=0; i<sz/2; i++){
                int temp = path[i];
                path[i] = path[sz-1-i];
                path[sz-1-i] = temp;
            }

            return path;
        }


        //returns the shortest travel time from S to T on the given adjacency list
        //returns -1 if T is unreachable from S
        int getPathTravelTime(vector<vector<pair<int,int>>>& adj, int S, int T){
            vector<int> dist, prev;
            dijkstra(adj, S, dist, prev);

            if(dist[T] == INT_MAX)
                return -1;
            return dist[T];
        }


        //build adjFull from the stored edges[]
        //called once from the constructor , adjFull does not change after that
        void buildFullAdjacency(){
            adjFull.assign(N, vector<pair<int,int>>());
            for(int i=0; i<(int)edges.size(); i++){
                Edge& e = edges[i];
                adjFull[e.u].push_back(make_pair(e.v, e.travelTime));
                adjFull[e.v].push_back(make_pair(e.u, e.travelTime));
            }
        }


        //build adjMST from mstEdges[] , called at the end of buildMST
        void buildMSTAdjacency(){
            adjMST.assign(N, vector<pair<int,int>>());
            for(int i=0; i<(int)mstEdges.size(); i++){
                Edge& e = mstEdges[i];
                adjMST[e.u].push_back(make_pair(e.v, e.travelTime));
                adjMST[e.v].push_back(make_pair(e.u, e.travelTime));
            }
        }


        //BFS based connectivity check used during graph validation
        //returns the count of nodes reachable from node 0
        //we use a vector<int> with a front index as a simple queue (no <queue> header needed)
        int countReachableFromZero(){
            //build a temporary unweighted adjacency to run BFS on
            vector<vector<int>> tempAdj(N);
            for(int i=0; i<(int)edges.size(); i++){
                tempAdj[edges[i].u].push_back(edges[i].v);
                tempAdj[edges[i].v].push_back(edges[i].u);
            }

            vector<bool> visited(N, false);
            vector<int> bfsQueue;
            bfsQueue.push_back(0);
            visited[0] = true;
            int reachedCount = 1;
            int front = 0;

            while(front < (int)bfsQueue.size()){
                int cur = bfsQueue[front];
                front++;

                for(int i=0; i<(int)tempAdj[cur].size(); i++){
                    int neighbour = tempAdj[cur][i];
                    if(!visited[neighbour]){
                        visited[neighbour] = true;
                        reachedCount++;
                        bfsQueue.push_back(neighbour);
                    }
                }
            }

            return reachedCount;
        }


    public:

        //parameterised constructor
        //we build adjFull here itself so that shortestPath / compareNetworks can run even if buildMST hasn't been called yet
        CityGraph(int N, vector<Edge>& edges){
            this->N = N;
            this->edges = edges;
            mstCost = 0;
            mstBuilt = false;

            buildFullAdjacency();
        }


        /*
        GRAPH VALIDATION POLICY:
        1) self-loops are forbidden (edge with u == v is rejected)
        2) duplicate roads between the same pair of intersections are forbidden
        3) construction cost and travel time must both be non-negative
        4) every node id used by an edge must lie in [0, N-1]
        5) the graph must be connected (a BFS from node 0 must reach all N nodes)

        ordering of checks:  (1) -> (4) -> (3) -> (2) -> (5)
        the very FIRST rule that fails causes the whole input to be rejected with a specific reason

        invalid source/destination node ids in shortestPath / compareNetworks are caught
        at the start of those functions (runtime check , see inside the function)
        */

        bool validateGraph(){
            cout<<endl;
            cout<<"GRAPH VALIDATION POLICY"<<endl;
            cout<<"1) self-loops are forbidden"<<endl;
            cout<<"2) duplicate roads between same pair are forbidden"<<endl;
            cout<<"3) cost and travel time must be non-negative"<<endl;
            cout<<"4) node ids must lie in [0, "<<N-1<<"]"<<endl;
            cout<<"5) graph must be connected (checked via BFS from node 0)"<<endl;
            cout<<endl;

            //check rules (1), (4), (3), (2) per edge
            for(int i=0; i<(int)edges.size(); i++){
                int u = edges[i].u;
                int v = edges[i].v;

                //rule (1) - self-loop
                if(u == v){
                    cout<<"INVALID -> edge "<<i<<" is a self-loop ("<<u<<" - "<<v<<")"<<endl;
                    return false;
                }

                //rule (4) - node id range
                if(u < 0 || u >= N || v < 0 || v >= N){
                    cout<<"INVALID -> edge "<<i<<" has a node id outside [0, "<<N-1<<"]"<<endl;
                    return false;
                }

                //rule (3) - non-negative weights
                if(edges[i].cost < 0){
                    cout<<"INVALID -> edge "<<i<<" ("<<u<<"-"<<v<<") has negative construction cost"<<endl;
                    return false;
                }
                if(edges[i].travelTime < 0){
                    cout<<"INVALID -> edge "<<i<<" ("<<u<<"-"<<v<<") has negative travel time"<<endl;
                    return false;
                }

                //rule (2) - duplicate road check (simple O(M) scan over earlier edges)
                for(int j=0; j<i; j++){
                    int prevU = edges[j].u;
                    int prevV = edges[j].v;
                    if((prevU == u && prevV == v) || (prevU == v && prevV == u)){
                        cout<<"INVALID -> duplicate road between "<<u<<" and "<<v<<endl;
                        return false;
                    }
                }
            }

            //rule (5) - connectivity via BFS from node 0
            int reached = countReachableFromZero();
            if(reached < N){
                cout<<"INVALID -> graph is disconnected ("<<reached<<"/"<<N
                    <<" nodes reachable from node 0)"<<endl;
                return false;
            }

            cout<<"the input graph is VALID -> "<<N<<" intersections, "<<edges.size()<<" roads"<<endl;
            return true;
        }


        /*
        builds the MST using KRUSKAL'S ALGORITHM:
        1) sort the edges in ascending order of construction cost (selection sort on indices)
        2) initialise the DSU where every node is its own set
        3) for each edge in sorted order:
            - if u and v lie in different sets -> ACCEPT the edge , union the sets , add cost
            - else (same set)                  -> REJECT the edge (it would form a cycle)
        4) running MST cost is printed after every acceptance

        at the end we also fill adjMST so compareNetworks can query the MST's shortest path
        */

        void buildMST(){
            cout<<endl;
            cout<<"BUILDING MST USING KRUSKAL'S ALGORITHM"<<endl;

            //list of indices into edges[] , we sort these instead of the edges themselves
            vector<int> indexList(edges.size());
            for(int i=0; i<(int)indexList.size(); i++)
                indexList[i] = i;

            sortEdgeIndicesByCost(indexList);

            initDSU(N);
            mstCost = 0;
            mstEdges.clear();

            for(int k=0; k<(int)indexList.size(); k++){
                Edge& e = edges[indexList[k]];

                //if u and v are already in the same set , accepting this edge would form a cycle
                if(findParent(e.u) == findParent(e.v)){
                    cout<<"edge ("<<e.u<<" - "<<e.v<<") cost = "<<e.cost
                        <<" -> REJECTED (would form a cycle)"<<endl;
                }
                else{
                    unionSets(e.u, e.v);
                    mstEdges.push_back(e);
                    mstCost += e.cost;
                    cout<<"edge ("<<e.u<<" - "<<e.v<<") cost = "<<e.cost
                        <<" -> ACCEPTED [running MST cost = "<<mstCost<<"]"<<endl;
                }
            }

            //fill adjMST so compareNetworks can use it
            buildMSTAdjacency();
            mstBuilt = true;

            cout<<endl;
            cout<<"FINAL MST -> total construction cost = "<<mstCost<<endl;
            for(int i=0; i<(int)mstEdges.size(); i++){
                Edge& e = mstEdges[i];
                cout<<"   ("<<e.u<<" - "<<e.v<<") cost = "<<e.cost
                    <<", travel_time = "<<e.travelTime<<endl;
            }
        }


        /*
        SHORTEST PATH from S to T on the FULL graph , weighted by travel time
        - this operation is independent of buildMST , adjFull is already built in the constructor
        - first validates that S and T lie inside [0, N-1] (runtime check)
        - runs dijkstra on adjFull , reconstructs the path using prev[]
        - prints the path with a per-hop travel time breakdown and the total
        */

        void shortestPath(int S, int T){
            cout<<endl;
            cout<<"SHORTEST PATH FROM "<<S<<" TO "<<T<<endl;

            //runtime validation of source / destination node ids
            if(S < 0 || S >= N){
                cout<<"ERROR -> source node "<<S<<" is invalid (must be in [0, "<<N-1<<"])"<<endl;
                return;
            }
            if(T < 0 || T >= N){
                cout<<"ERROR -> destination node "<<T<<" is invalid (must be in [0, "<<N-1<<"])"<<endl;
                return;
            }

            vector<int> dist, prev;
            dijkstra(adjFull, S, dist, prev);

            vector<int> path = reconstructPath(T, prev, dist);

            if(path.empty()){
                cout<<"no path exists from "<<S<<" to "<<T<<endl;
                return;
            }

            //print the path itself
            cout<<"path -> ";
            for(int i=0; i<(int)path.size(); i++){
                cout<<path[i];
                if(i < (int)path.size()-1)
                    cout<<" -> ";
            }
            cout<<endl;

            //print the per-hop travel time breakdown
            cout<<"per-hop travel time breakdown:"<<endl;
            int total = 0;
            for(int i=0; i<(int)path.size()-1; i++){
                int from = path[i];
                int to = path[i+1];

                //look up the edge weight between 'from' and 'to' inside adjFull
                int hopTime = 0;
                for(int j=0; j<(int)adjFull[from].size(); j++){
                    if(adjFull[from][j].first == to){
                        hopTime = adjFull[from][j].second;
                        break;
                    }
                }

                total += hopTime;
                cout<<"   "<<from<<" -> "<<to<<" : "<<hopTime<<endl;
            }
            cout<<"total travel time -> "<<total<<endl;
        }


        /*
        compares the travel time of S-to-T on the MST vs on the FULL graph
        if the MST path is slower, the difference and the percentage (relative to the shortest path) are reported

        note: buildMST must have been called before this , otherwise adjMST is empty
        we guard against that with the mstBuilt flag
        */

        void compareNetworks(int S, int T){
            cout<<endl;
            cout<<"COMPARISON OF MST PATH vs SHORTEST PATH (S = "<<S<<", T = "<<T<<")"<<endl;

            //runtime validation of source / destination
            if(S < 0 || S >= N || T < 0 || T >= N){
                cout<<"ERROR -> S or T is outside the valid range [0, "<<N-1<<"]"<<endl;
                return;
            }

            if(!mstBuilt){
                cout<<"ERROR -> MST has not been built yet , call buildMST() first"<<endl;
                return;
            }

            int spTime = getPathTravelTime(adjFull, S, T);
            int mstTime = getPathTravelTime(adjMST, S, T);

            if(spTime < 0 || mstTime < 0){
                cout<<"comparison not possible (no path exists in one of the two structures)"<<endl;
                return;
            }

            cout<<"shortest path travel time on full graph -> "<<spTime<<endl;
            cout<<"MST path travel time                    -> "<<mstTime<<endl;

            if(mstTime == spTime){
                cout<<"result -> the MST path is exactly as fast as the shortest path"<<endl;
            }
            else{
                int diff = mstTime - spTime;
                double pct = ((double)diff / spTime) * 100.0;
                cout<<"result -> the MST path is slower by "<<diff
                    <<" units ("<<pct<<"% more)"<<endl;
            }
        }


        /*
        a road is CRITICAL if removing it from the graph increases the MST cost
        (or in the extreme case , makes the MST impossible to build at all)

        for every edge in mstEdges:
        1) reinitialise the DSU
        2) rerun Kruskal's pass while skipping this one edge
        3) if fewer than N-1 edges get added   -> the graph becomes disconnected -> CRITICAL
           else if new MST cost > original    -> CRITICAL (with cost impact)
           else                               -> NOT CRITICAL (a same-cost alternative exists)
        */

        void criticalRoads(){
            cout<<endl;
            cout<<"CRITICAL ROADS ANALYSIS"<<endl;

            if(!mstBuilt){
                cout<<"ERROR -> MST has not been built yet , call buildMST() first"<<endl;
                return;
            }

            cout<<"removing each MST edge in turn and rebuilding the MST..."<<endl;

            //sort edge indices by cost once , reuse for every rebuild
            vector<int> indexList(edges.size());
            for(int i=0; i<(int)indexList.size(); i++)
                indexList[i] = i;
            sortEdgeIndicesByCost(indexList);

            bool anyCritical = false;

            for(int k=0; k<(int)mstEdges.size(); k++){
                Edge& removedEdge = mstEdges[k];

                initDSU(N);
                int newCost = 0;
                int addedEdges = 0;

                for(int i=0; i<(int)indexList.size(); i++){
                    Edge& e = edges[indexList[i]];

                    //skip the removed edge , match by endpoints in either direction
                    if((e.u == removedEdge.u && e.v == removedEdge.v) ||
                       (e.u == removedEdge.v && e.v == removedEdge.u))
                        continue;

                    if(findParent(e.u) != findParent(e.v)){
                        unionSets(e.u, e.v);
                        newCost += e.cost;
                        addedEdges++;
                    }
                }

                if(addedEdges < N - 1){
                    cout<<"   ("<<removedEdge.u<<" - "<<removedEdge.v<<") cost = "<<removedEdge.cost
                        <<" -> CRITICAL (graph becomes disconnected without this road)"<<endl;
                    anyCritical = true;
                }
                else if(newCost > mstCost){
                    int impact = newCost - mstCost;
                    cout<<"   ("<<removedEdge.u<<" - "<<removedEdge.v<<") cost = "<<removedEdge.cost
                        <<" -> CRITICAL (MST cost increases by "<<impact
                        <<", new MST cost = "<<newCost<<")"<<endl;
                    anyCritical = true;
                }
                else{
                    cout<<"   ("<<removedEdge.u<<" - "<<removedEdge.v<<") cost = "<<removedEdge.cost
                        <<" -> NOT CRITICAL (a same-cost alternative exists)"<<endl;
                }
            }

            if(!anyCritical){
                cout<<"   no critical roads were found in this MST"<<endl;
            }
        }
};



int main(){

    //TEST CASE 1
    cout<<"TEST CASE 1\n";

    int N1 = 6;
    vector<Edge> edges1 = {
        {0, 1, 4, 2}, {0, 2, 3, 5}, {1, 2, 1, 3},
        {1, 3, 2, 7}, {2, 3, 4, 1}, {2, 4, 5, 8},
        {3, 4, 7, 4}, {3, 5, 6, 3}, {4, 5, 2, 6}
    };

    CityGraph g1(N1, edges1);

    if(g1.validateGraph()){
        g1.buildMST();
        g1.shortestPath(0, 5);
        g1.compareNetworks(0, 5);
        g1.criticalRoads();
    }


    //TEST CASE 2
    cout<<"\n\nTEST CASE 2\n";

    int N2 = 4;
    vector<Edge> edges2 = {
        {0, 1, 10, 1}, {0, 2,  6, 4}, {0, 3, 5, 8},
        {1, 3, 15, 2}, {2, 3,  4, 3}
    };

    CityGraph g2(N2, edges2);

    if(g2.validateGraph()){
        g2.buildMST();

        //shortestPath called BEFORE any of the following operations , this works fine because adjFull was built in the constructor
        g2.shortestPath(0, 3);
        g2.compareNetworks(0, 3);
        g2.criticalRoads();
    }


    //TEST CASE 3 - demonstrates that shortestPath works without calling buildMST first
    cout<<"\n\nTEST CASE 3 (shortestPath called BEFORE buildMST)\n";

    int N3 = 5;
    vector<Edge> edges3 = {
        {0, 1, 2, 3}, {0, 2, 5, 1}, {1, 2, 1, 2},
        {1, 3, 4, 5}, {2, 4, 3, 2}, {3, 4, 2, 4}
    };

    CityGraph g3(N3, edges3);

    if(g3.validateGraph()){
        //call shortestPath FIRST , before buildMST
        g3.shortestPath(0, 4);

        //now build the MST and call the rest
        g3.buildMST();
        g3.compareNetworks(0, 4);
        g3.criticalRoads();
    }


    //TEST CASE 4 - invalid graph , rejected by rule (1) self-loop
    cout<<"\n\nTEST CASE 4 (invalid graph)\n";

    int N4 = 3;
    vector<Edge> edges4 = {
        {0, 0, 5, 2},     //self-loop
        {0, 1, 3, 1}
    };

    CityGraph g4(N4, edges4);
    g4.validateGraph();

    return 0;
}
