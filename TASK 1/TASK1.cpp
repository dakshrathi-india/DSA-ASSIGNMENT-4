#include <iostream>
#include <vector>
#include <climits>

using namespace std;


/*
this struct represents a single road in the city graph
each road carries:
1) two endpoint intersections (u, v)
2) construction cost (used while building the MST)
3) travel time (used while finding the shortest path)

we treat the graph as undirected, so the order of u and v does not matter
*/
struct Edge {
    int u;
    int v;
    int cost;
    int travelTime;
};


/*
this CLASS contains the code for:
1) validating the input graph against the validation policy
2) building the MST using Kruskal's algorithm
3) finding the shortest travel-time path using Dijkstra's algorithm
4) comparing the MST path travel time against the shortest path travel time
5) finding the critical roads of the MST

private section contains the helper functions:
1) DSU helpers (initDSU, findParent, unionSets) used inside Kruskal's
2) sortEdgeIndicesByCost - selection sort over edge indices used by Kruskal's
3) dijkstra - the actual single source shortest path routine, takes any adjacency chain
4) reconstructPath - walks the prev[] array to rebuild the actual S -> T path
5) getPathTravelTime - returns the travel time of the shortest S -> T path on a given adjacency chain
6) buildAdjacency - builds the two adjacency chains for the full graph and for the MST
7) countReachableFromZero - BFS based connectivity check used during validation

public section contains the 5 required operations exactly as named in the task statement
*/
class CityGraph{
    private:
        int N;                          //number of intersections
        vector<Edge> edges;             //list of all roads (the original input)
        vector<Edge> mstEdges;          //roads that were accepted into the MST
        int mstCost;                    //total construction cost of the MST

        //adjacency CHAINS, weighted by travel time
        //each entry of adjFull[v] is a pair {neighbour, travelTime}
        vector<vector<pair<int,int>>> adjFull;
        vector<vector<pair<int,int>>> adjMST;

        //DSU arrays - reused across the buildMST call and the criticalRoads rebuild loop
        vector<int> parent;
        vector<int> setSize;


        //initialize the DSU - every node starts as its own set of size 1
        void initDSU(int n){
            parent.assign(n, 0);
            setSize.assign(n, 1);
            for(int i=0; i<n; i++)
                parent[i] = i;
        }


        //find the representative (root) of the set that contains x
        //we use path compression so that the next find on the same node is essentially O(1)
        int findParent(int x){
            //base case
            if(parent[x] == x)
                return x;
            parent[x] = findParent(parent[x]);
            return parent[x];
        }


        //union the sets that contain x and y by attaching the smaller tree under the larger one
        //this is the union-by-size variant which keeps the trees shallow
        //returns true if a real union actually happened
        bool unionSets(int x, int y){
            int rootX = findParent(x);
            int rootY = findParent(y);

            //already in the same set
            if(rootX == rootY)
                return false;

            //attach the smaller tree under the larger one
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


        //SELECTION SORT over the indices into edges[]
        //we sort the indices (not the edges themselves) so the original input order is preserved
        //ascending order on construction cost
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
        DIJKSTRA's algorithm in the simple O(N^2) form
        - we don't use a heap here; at every step we just scan dist[] for the unvisited node with the smallest distance
        - dist[]   -> shortest distance from S to every node, INT_MAX if unreachable
        - prev[]   -> predecessor pointer used later to reconstruct the actual path
        - visited  -> tracks which nodes are already finalised
        */
        void dijkstra(vector<vector<pair<int,int>>>& adj, int S, vector<int>& dist, vector<int>& prev){
            dist.assign(N, INT_MAX);
            prev.assign(N, -1);
            vector<bool> visited(N, false);

            dist[S] = 0;

            //we will finalise N nodes in the worst case
            for(int count=0; count<N; count++){
                //pick the unvisited node with the smallest distance
                int u = -1;
                int minDist = INT_MAX;
                for(int i=0; i<N; i++){
                    if(!visited[i] && dist[i] < minDist){
                        minDist = dist[i];
                        u = i;
                    }
                }

                //no more reachable nodes -> we can stop
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


        //rebuilds the actual path from S to T using the prev[] array filled by dijkstra
        //returns an empty vector if T was unreachable
        vector<int> reconstructPath(int T, vector<int>& prev, vector<int>& dist){
            vector<int> path;

            //edge case
            if(dist[T] == INT_MAX)
                return path;

            //walk backwards from T using prev[] until we reach the source (prev = -1)
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


        //returns the shortest travel time from S to T on the given adjacency chain
        //returns -1 if no path exists
        int getPathTravelTime(vector<vector<pair<int,int>>>& adj, int S, int T){
            vector<int> dist, prev;
            dijkstra(adj, S, dist, prev);

            if(dist[T] == INT_MAX)
                return -1;
            return dist[T];
        }


        //builds the two adjacency chains - one for the full graph, one for the MST
        //each chain entry is a pair {neighbour, travelTime}
        void buildAdjacency(){
            adjFull.assign(N, vector<pair<int,int>>());
            adjMST.assign(N, vector<pair<int,int>>());

            for(int i=0; i<(int)edges.size(); i++){
                Edge& e = edges[i];
                adjFull[e.u].push_back(make_pair(e.v, e.travelTime));
                adjFull[e.v].push_back(make_pair(e.u, e.travelTime));
            }

            for(int i=0; i<(int)mstEdges.size(); i++){
                Edge& e = mstEdges[i];
                adjMST[e.u].push_back(make_pair(e.v, e.travelTime));
                adjMST[e.v].push_back(make_pair(e.u, e.travelTime));
            }
        }


        //BFS based connectivity check - used during graph validation
        //returns the count of nodes reachable from node 0
        //we use a vector<int> with a front index as a simple queue (no <queue> header needed)
        int countReachableFromZero(){
            //build a temporary unweighted adjacency so we can BFS
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
        //we take the edges and N once, store them, and all later operations work on these stored copies
        CityGraph(int N, vector<Edge>& edges){
            this->N = N;
            this->edges = edges;
            mstCost = 0;
        }


        /*
        graph validation policy:
        1) self-loops are forbidden (an edge with u == v is rejected)
        2) duplicate roads between the same pair of intersections are forbidden
        3) construction cost and travel time both must be non-negative
        4) every node id used by an edge must lie in [0, N-1]
        5) the graph must be connected (a BFS from node 0 must reach every node)

        the very first rule that fails causes the whole input to be rejected
        and a specific reason is printed
        for runtime queries, invalid source / destination node ids in shortestPath
        and compareNetworks are caught at the start of the call
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

            //check rules (1), (4), (3) per edge and detect duplicates inside the same loop
            for(int i=0; i<(int)edges.size(); i++){
                int u = edges[i].u;
                int v = edges[i].v;

                //rule (1) - self-loop check
                if(u == v){
                    cout<<"INVALID -> edge "<<i<<" is a self-loop ("<<u<<" - "<<v<<")"<<endl;
                    return false;
                }

                //rule (4) - node id range check
                if(u < 0 || u >= N || v < 0 || v >= N){
                    cout<<"INVALID -> edge "<<i<<" has a node id outside [0, "<<N-1<<"]"<<endl;
                    return false;
                }

                //rule (3) - non-negative weight check
                if(edges[i].cost < 0){
                    cout<<"INVALID -> edge "<<i<<" ("<<u<<"-"<<v<<") has negative construction cost"<<endl;
                    return false;
                }
                if(edges[i].travelTime < 0){
                    cout<<"INVALID -> edge "<<i<<" ("<<u<<"-"<<v<<") has negative travel time"<<endl;
                    return false;
                }

                //rule (2) - duplicate check
                //for each new edge we scan all earlier edges and check if the unordered pair {u, v} matches
                for(int j=0; j<i; j++){
                    int prevU = edges[j].u;
                    int prevV = edges[j].v;
                    if((prevU == u && prevV == v) || (prevU == v && prevV == u)){
                        cout<<"INVALID -> duplicate road between "<<u<<" and "<<v<<endl;
                        return false;
                    }
                }
            }

            //rule (5) - connectivity check using BFS from node 0
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
        builds the MST using KRUSKAL'S ALGORITHM (a greedy approach):
        1) sort the edges in non-decreasing order of construction cost
           (we sort the indices, not the edges themselves)
        2) initialise a DSU where each node is in its own set
        3) walk through the sorted edges:
            - if the two endpoints lie in different sets -> ACCEPT the edge into the MST and union the two sets
            - else (they are already in the same set)    -> REJECT the edge as it would form a cycle
        4) the running MST cost is printed after every acceptance
        */

        void buildMST(){
            cout<<endl;
            cout<<"BUILDING MST USING KRUSKAL'S ALGORITHM"<<endl;

            //list of indices into edges[] - we sort these so the original input is left untouched
            vector<int> indexList(edges.size());
            for(int i=0; i<(int)indexList.size(); i++)
                indexList[i] = i;

            sortEdgeIndicesByCost(indexList);

            initDSU(N);
            mstCost = 0;
            mstEdges.clear();

            for(int k=0; k<(int)indexList.size(); k++){
                Edge& e = edges[indexList[k]];

                //if u and v are already in the same set, accepting this edge would form a cycle
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

            //build the two adjacency chains so that shortestPath / compareNetworks have something to query
            buildAdjacency();

            cout<<endl;
            cout<<"FINAL MST -> total construction cost = "<<mstCost<<endl;
            for(int i=0; i<(int)mstEdges.size(); i++){
                Edge& e = mstEdges[i];
                cout<<"   ("<<e.u<<" - "<<e.v<<") cost = "<<e.cost
                    <<", travel_time = "<<e.travelTime<<endl;
            }
        }


        /*
        finds the SHORTEST TRAVEL TIME PATH from S to T on the FULL graph
        - first validates that S and T lie inside [0, N-1]
        - runs the dijkstra helper and reconstructs the actual path using prev[]
        - prints the path along with the per-hop travel time breakdown and the total
        */

        void shortestPath(int S, int T){
            cout<<endl;
            cout<<"SHORTEST PATH FROM "<<S<<" TO "<<T<<endl;

            //runtime validation of source / destination
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

                //look up the actual edge weight between 'from' and 'to' inside the full adjacency chain
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
        if the MST path is slower, the absolute difference and the percentage difference
        (relative to the optimal shortest-path travel time) are reported
        */

        void compareNetworks(int S, int T){
            cout<<endl;
            cout<<"COMPARISON OF MST PATH vs SHORTEST PATH (S = "<<S<<", T = "<<T<<")"<<endl;

            //runtime validation of source / destination
            if(S < 0 || S >= N || T < 0 || T >= N){
                cout<<"ERROR -> S or T is outside the valid range [0, "<<N-1<<"]"<<endl;
                return;
            }

            int spTime = getPathTravelTime(adjFull, S, T);
            int mstTime = getPathTravelTime(adjMST, S, T);

            if(spTime < 0 || mstTime < 0){
                cout<<"comparison not possible (no path exists in one of the structures)"<<endl;
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
        a road is CRITICAL if removing it from the input increases the MST cost
        (or, in the extreme case, makes the MST impossible to build at all)

        for every MST edge:
        1) we rebuild the MST while skipping this one edge during Kruskal's pass
        2) if fewer than N-1 edges can be added -> the graph would become disconnected -> CRITICAL
        3) else if the new MST cost is greater than the original mstCost -> CRITICAL with cost impact
        4) else -> NOT CRITICAL (a same-cost alternative exists)
        */

        void criticalRoads(){
            cout<<endl;
            cout<<"CRITICAL ROADS ANALYSIS"<<endl;
            cout<<"removing each MST edge in turn and rebuilding the MST..."<<endl;

            //sort edge indices by cost once and reuse for every rebuild
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

                    //skip the removed edge - we match by endpoints in either direction
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
    cout<<"=========================================="<<endl;
    cout<<"               TEST CASE 1                "<<endl;
    cout<<"==========================================";

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
    cout<<endl<<endl;
    cout<<"=========================================="<<endl;
    cout<<"               TEST CASE 2                "<<endl;
    cout<<"==========================================";

    int N2 = 4;
    vector<Edge> edges2 = {
        {0, 1, 10, 1}, {0, 2,  6, 4}, {0, 3, 5, 8},
        {1, 3, 15, 2}, {2, 3,  4, 3}
    };

    CityGraph g2(N2, edges2);

    if(g2.validateGraph()){
        g2.buildMST();
        g2.shortestPath(0, 3);
        g2.compareNetworks(0, 3);
        g2.criticalRoads();
    }


    //TEST CASE 3 (invalid graph - self-loop, will be rejected by rule 1)
    cout<<endl<<endl;
    cout<<"=========================================="<<endl;
    cout<<"   TEST CASE 3 (invalid graph)            "<<endl;
    cout<<"==========================================";

    int N3 = 3;
    vector<Edge> edges3 = {
        {0, 0, 5, 2},   //self-loop
        {0, 1, 3, 1}
    };

    CityGraph g3(N3, edges3);
    g3.validateGraph();

    return 0;
}
