#include <iostream>
#include <vector>
#include <climits>
#include <algorithm>
using namespace std;

/*struct represents one road in the city graph
1)u, v -> two intersections the road connects
2)cost -> construction cost (for MST)
3)travelTime -> time to travel this road (for shortest path)
the graph is undirected so the order of u and v doesn't matter
*/
struct Edge{
    int u;
    int v;
    int cost;
    int travelTime;
};

/*class implements a min-heap for dijkstra's algorithm
stores (dist , node) pairs , the pair with the smallest dist is always at the top
*/
class MinHeap{
    private:
        vector<pair<int,int>> data;

        void swapPairs(int i, int j){
            pair<int,int> temp = data[i];
            data[i] = data[j];
            data[j] = temp;
        }

        //SIFTING A Pair UP WHILE IT IS SMALLER THAN ITS PARENT
        void siftUp(int i){
            while(i > 0){
                int par = (i-1)/2;
                if(data[i].first < data[par].first){
                    swapPairs(i, par);
                    i = par;
                }
                else
                    break;
            }
        }

        //SIFTING A PAIR DOWN WHILE IT IS LARGER THAN ONE OF ITS CHILDREN
        void siftDown(int i){
            int n = data.size();
            while(true){
                int smallest = i;
                int leftChild = 2*i+1;
                int rightChild = 2*i+2;

                if(leftChild < n && data[leftChild].first < data[smallest].first)
                    smallest = leftChild;
                if(rightChild < n && data[rightChild].first < data[smallest].first)
                    smallest = rightChild;

                if(smallest != i){
                    swapPairs(i, smallest);
                    i = smallest;
                }
                else
                    break;
            }
        }

    public:
        MinHeap(){}

        bool isEmpty(){
            return data.empty();
        }

        void push(int dist, int node){
            data.push_back(make_pair(dist, node));
            siftUp(data.size() - 1);
        }

        pair<int,int> extractMin(){
            pair<int,int> top = data[0];
            int n = data.size();
            if(n > 1){
                data[0] = data[n-1];
                data.pop_back();
                siftDown(0);
            }
            else{
                data.pop_back();
            }
            return top;
        }
};

class CityGraph{
    private:
        int N;
        vector<Edge> edges;
        vector<Edge> mstEdges;
        int mstCost;
        vector<vector<pair<int,int>>> adjFull;
        vector<vector<pair<int,int>>> adjMST;
        bool mstBuilt;
        vector<int> parent;
        vector<int> setSize;

        void initDSU(int n){
            parent.assign(n, 0);
            setSize.assign(n, 1);
            for(int i=0; i<n; i++)
                parent[i] = i;
        }

        //FINDING THE ROOT OF THE SET THAT CONTAINS x
        int findParent(int x){
            //base case
            if(parent[x] == x)
                return x;
            parent[x] = findParent(parent[x]);
            return parent[x];
        }

        //UNIONING TWO SETS (UNION BY SIZE)
        //attach the smaller tree under the larger one, returns true if a real union happened
        bool unionSets(int x, int y){
            int rootX = findParent(x);
            int rootY = findParent(y);

            //already in the same set
            if(rootX == rootY)
                return false;
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

        //STL SORT ON THE EDGE INDICES, ASCENDING ON COST
        //we sort indices not the edges so the original input order is not disturbed
        void sortEdgeIndicesByCost(vector<int>& indexList){
            sort(indexList.begin(), indexList.end(), [&](int a, int b){
                return edges[a].cost < edges[b].cost;
            });
        }

        //FINDING SHORTEST DISTANCES FROM S TO ALL NODES USING DIJKSTRA, using the MinHeap class 
        void dijkstra(vector<vector<pair<int,int>>>& adj, int S, vector<int>& dist, vector<int>& prev){
            dist.assign(N, INT_MAX);
            prev.assign(N, -1);
            dist[S] = 0;
            MinHeap pq;
            pq.push(0, S);

            while(!pq.isEmpty()){
                pair<int,int> cur = pq.extractMin();
                int d = cur.first;
                int u = cur.second;
                if(d > dist[u])
                    continue;
                int adjSz = adj[u].size();

                for(int i=0; i<adjSz; i++){
                    int v = adj[u][i].first;
                    int w = adj[u][i].second;
                    if(dist[u] + w < dist[v]){
                        dist[v] = dist[u] + w;
                        prev[v] = u;
                        pq.push(dist[v], v);
                    }
                }
            }
        }

        //REBUILDING THE ACTUAL PATH FROM S TO T USING prev[], returns an empty vector if T was unreachable
        vector<int> reconstructPath(int T, vector<int>& prev, vector<int>& dist){
            vector<int> path;

            if(dist[T] == INT_MAX)
                return path;

            //walk backwards from T till we reach the source
            int cur = T;
            while(cur != -1){
                path.push_back(cur);
                cur = prev[cur];
            }

            //reverse so path goes S -> T
            int sz = path.size();
            for(int i=0; i<sz/2; i++){
                int temp = path[i];
                path[i] = path[sz-1-i];
                path[sz-1-i] = temp;
            }
            return path;
        }

        //returns the shortest travel time from S to T, returns -1 if T is unreachable
        int getPathTravelTime(vector<vector<pair<int,int>>>& adj, int S, int T){
            vector<int> dist, prev;
            dijkstra(adj, S, dist, prev);
            if(dist[T] == INT_MAX)
                return -1;
            return dist[T];
        }

        //Building adjFull FROM edges[]
        void buildFullAdjacency(){
            adjFull.assign(N, vector<pair<int,int>>());
            int n = edges.size();
            for(int i=0; i<n; i++){
                Edge& e = edges[i];
                adjFull[e.u].push_back(make_pair(e.v, e.travelTime));
                adjFull[e.v].push_back(make_pair(e.u, e.travelTime));
            }
        }

        //BUILDING adjMST FROM mstEdges[]
        void buildMSTAdjacency(){
            adjMST.assign(N, vector<pair<int,int>>());
            int n = mstEdges.size();
            for(int i=0; i<n; i++){
                Edge& e = mstEdges[i];
                adjMST[e.u].push_back(make_pair(e.v, e.travelTime));
                adjMST[e.v].push_back(make_pair(e.u, e.travelTime));
            }
        }

        //CHECKING CONNECTIVITY VIA BFS FROM NODE 0, returns how many nodes are rechable from node 0
        int countReachableFromZero(){
            vector<vector<int>> tempAdj(N);
            int edgeSz = edges.size();
            for(int i=0; i<edgeSz; i++){
                tempAdj[edges[i].u].push_back(edges[i].v);
                tempAdj[edges[i].v].push_back(edges[i].u);
            }

            vector<bool> visited(N, false);
            vector<int> bfsQueue;
            bfsQueue.push_back(0);
            visited[0] = true;
            int reachedCount = 1;
            int qSz = 1;
            int front = 0;

            while(front < qSz){
                int cur = bfsQueue[front];
                front++;
                int adjSz = tempAdj[cur].size();
                for(int i=0; i<adjSz; i++){
                    int neighbour = tempAdj[cur][i];
                    if(!visited[neighbour]){
                        visited[neighbour] = true;
                        reachedCount++;
                        bfsQueue.push_back(neighbour);
                        qSz++;
                    }
                }
            }
            return reachedCount;
        }

    public:
        CityGraph(int N, vector<Edge>& edges){
            this->N = N;
            this->edges = edges;
            mstCost = 0;
            mstBuilt = false;
            buildFullAdjacency();
        }

        /*GRAPH VALIDATION POLICY:
        1)self-loops are forbidden (u == v is rejected)
        2)duplicate roads between same pair are forbidden
        3)cost and travelTime must both be non-negative
        4)every node id must lie in [0, N-1]
        5)graph must be connected (BFS from node 0 must reach all N nodes)
        */
        bool validateGraph(){
            int edgeSz = edges.size();
            for(int i=0; i<edgeSz; i++){
                int u = edges[i].u;
                int v = edges[i].v;

                if(u == v){
                    cout<<"INVALID -> edge "<<i<<" is a self-loop ("<<u<<" - "<<v<<")"<<endl;
                    return false;
                }
                if(u < 0 || u >= N || v < 0 || v >= N){
                    cout<<"INVALID -> edge "<<i<<" has a node id outside [0, "<<N-1<<"]"<<endl;
                    return false;
                }
                if(edges[i].cost < 0){
                    cout<<"INVALID -> edge "<<i<<" ("<<u<<"-"<<v<<") has negative construction cost"<<endl;
                    return false;
                }
                if(edges[i].travelTime < 0){
                    cout<<"INVALID -> edge "<<i<<" ("<<u<<"-"<<v<<") has negative travel time"<<endl;
                    return false;
                }

                for(int j=0; j<i; j++){
                    int prevU = edges[j].u;
                    int prevV = edges[j].v;
                    if((prevU == u && prevV == v) || (prevU == v && prevV == u)){
                        cout<<"INVALID -> duplicate road between "<<u<<" and "<<v<<endl;
                        return false;
                    }
                }
            }
            int reached = countReachableFromZero();
            if(reached < N){
                cout<<"INVALID -> graph is disconnected ("<<reached<<"/"<<N
                    <<" nodes reachable from node 0)"<<endl;
                return false;
            }
            cout<<"the input graph is VALID -> "<<N<<" intersections, "<<edges.size()<<" roads"<<endl;
            return true;
        }

        //BUILDING THE MST USING KRUSKAL'S ALGORITHM
        //SORT EDGES BY COST -> INIT DSU -> PROCESS EACH EDGE IN ORDER, ACCEPT IF IN DIFFERENT SETS , REJECT IF SAME SET (WOULD FORM A CYCLE)
        void buildMST(){
            cout<<endl;
            cout<<"BUILDING MST USING KRUSKAL'S ALGORITHM"<<endl;

            int edgeSz = edges.size();
            vector<int> indexList(edgeSz);
            for(int i=0; i<edgeSz; i++)
                indexList[i] = i;

            sortEdgeIndicesByCost(indexList);
            initDSU(N);
            mstCost = 0;
            mstEdges.clear();

            for(int k=0; k<edgeSz; k++){
                Edge& e = edges[indexList[k]];
                //same set means this edge would form a cyle
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
            int mstSz = mstEdges.size();
            for(int i=0; i<mstSz; i++){
                Edge& e = mstEdges[i];
                cout<<"   ("<<e.u<<" - "<<e.v<<") cost = "<<e.cost
                    <<", travel_time = "<<e.travelTime<<endl;
            }
        }

        //FINDING THE SHORTEST TRAVEL TIME PATH FROM S TO T ON THE FULL GRAPH
        void shortestPath(int S, int T){
            cout<<endl;
            cout<<"SHORTEST PATH FROM "<<S<<" TO "<<T<<endl;

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

            int pathSz = path.size();
            cout<<"path -> ";
            for(int i=0; i<pathSz; i++){
                cout<<path[i];
                if(i < pathSz-1)
                    cout<<" -> ";
            }
            cout<<endl;
            cout<<"per-hop travel time breakdown:"<<endl;
            int total = 0;

            for(int i=0; i<pathSz-1; i++){
                int from = path[i];
                int to = path[i+1];
                //look up edge weight in adjFull
                int hopTime = 0;
                int adjSz = adjFull[from].size();

                for(int j=0; j<adjSz; j++){
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

        //COMPARING THE MST PATH TRAVEL TIME VS THE SHORTEST PATH TRAVEL TIME, if the MST path is slower we print the difference and the percentage
        void compareNetworks(int S, int T){
            cout<<endl;
            cout<<"COMPARISON OF MST PATH vs SHORTEST PATH (S = "<<S<<", T = "<<T<<")"<<endl;

            if(S < 0 || S >= N || T < 0 || T >= N){
                cout<<"ERROR -> S or T is outside the valid range [0, "<<N-1<<"]"<<endl;
                return;
            }
            if(!mstBuilt){
                cout<<"ERROR -> MST has not been built yet, call buildMST() first"<<endl;
                return;
            }
            int spTime = getPathTravelTime(adjFull, S, T);
            int mstTime = getPathTravelTime(adjMST, S, T);

            if(spTime < 0 || mstTime < 0){
                cout<<"comparison not possible (no path exists in one of the two structures)"<<endl;
                return;
            }
            cout<<"shortest path travel time on full graph -> "<<spTime<<endl;
            cout<<"MST path travel time -> "<<mstTime<<endl;

            if(mstTime == spTime){
                cout<<"result -> the MST path is exactly as fast as the shortest path"<<endl;
            }
            else{
                int diff = mstTime - spTime;
                double pct = ((double)diff / spTime) * 100.0;
                cout<<"result -> the MST path is slower by "<<diff<<" units ("<<pct<<"% more)"<<endl;
            }
        }

        //FINDING THE CRITICAL ROADS IN THE MST
        //a road is critical if removing it increases the MST cost (or disconnects the graph), we remove each MST edge one by one and rerun kruskal's to check
        void criticalRoads(){
            cout<<endl;
            cout<<"CRITICAL ROADS ANALYSIS"<<endl;

            if(!mstBuilt){
                cout<<"ERROR -> MST has not been built yet , call buildMST() first"<<endl;
                return;
            }
            cout<<"removing each MST edge in turn and rebuilding the MST..."<<endl;

            int edgeSz = edges.size();
            vector<int> indexList(edgeSz);
            for(int i=0; i<edgeSz; i++)
                indexList[i] = i;
            sortEdgeIndicesByCost(indexList); 
            bool anyCritical = false;
            int mstSz = mstEdges.size();

            for(int k=0; k<mstSz; k++){
                Edge& removedEdge = mstEdges[k];
                initDSU(N);
                int newCost = 0;
                int addedEdges = 0;

                for(int i=0; i<edgeSz; i++){
                    Edge& e = edges[indexList[i]];
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
                    //graph becomes disconnected without this edge , thus critical
                    cout<<"   ("<<removedEdge.u<<" - "<<removedEdge.v<<") cost = "<<removedEdge.cost<<" -> CRITICAL (graph becomes disconnected without this road)"<<endl;
                    anyCritical = true;
                }
                else if(newCost > mstCost){
                    int impact = newCost - mstCost;
                    cout<<"   ("<<removedEdge.u<<" - "<<removedEdge.v<<") cost = "<<removedEdge.cost<<" -> CRITICAL (MST cost increases by "<<impact<<", new MST cost = "<<newCost<<")"<<endl;
                    anyCritical = true;
                }
                else{
                    //same cost alternative exists, thus not critical
                    cout<<"   ("<<removedEdge.u<<" - "<<removedEdge.v<<") cost = "<<removedEdge.cost<<" -> NOT CRITICAL (a same-cost alternative exists)"<<endl;
                }
            }

            if(!anyCritical){
                cout<<"   no critical roads were found in this MST"<<endl;
            }
        }
};

int main(){
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

    cout<<"\n\nTEST CASE 2\n";
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
    return 0;
}