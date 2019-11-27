/*
 * Relation.cpp
 *
 *  Created on: Nov 27, 2019
 *      Author: Kadir Yanık
 *  Algorithm reference: https://www.geeksforgeeks.org/shortest-path-unweighted-graph
 */

#include "Relation.h"

/*------------------------------------------------------------------------------*/
Relation::Relation(int v){
    this->v = v;
    this->adj = new vector<int>[v];
}

/*------------------------------------------------------------------------------*/
Relation::~Relation(){
	if(adj != NULL){
	    delete [] adj;
	}
}

/*------------------------------------------------------------------------------*/
void Relation::addEdge(int src, int dest){
    this->adj[src].push_back(dest);
    this->adj[dest].push_back(src);
}

/*------------------------------------------------------------------------------*/
vector<int> Relation::getShortestDistance(int src, int dest){
    // predecessor[i] array stores predecessor of i and distance array stores distance of i from src
    int pred[this->v], dist[this->v];

    // vector path stores the shortest path
    vector<int> path;

    if(BFS(src, dest, pred, dist) == false){
        // Given source and destination are not connected;
        return path;
    }


    int crawl = dest;
    path.push_back(crawl);
    while(pred[crawl] != -1){
        path.push_back(pred[crawl]);
        crawl = pred[crawl];
    }

    return path;
}

/*------------------------------------------------------------------------------*/
bool Relation::BFS(int src, int dest, int pred[], int dist[]){
    // a queue to maintain queue of vertices whose adjacency list is to be scanned as per normal DFS algorithm
    list<int> queue;

    // boolean array visited[] which stores the information whether ith vertex is reached at least once in the Breadth first search
    bool visited[this->v];

    /* initially all vertices are unvisited so v[i] for all i is false
     * and as no path is yet constructed dist[i] for all i set to infinity */
    for(int i = 0; i < this->v; i++){
        visited[i] = false;
        dist[i] = INT_MAX;
        pred[i] = -1;
    }

    // now source is first to be visited and distance from source to itself should be 0
    visited[src] = true;
    dist[src] = 0;
    queue.push_back(src);

    // standard BFS algorithm
    while(!queue.empty()){
        int u = queue.front();
        queue.pop_front();
        for(int i = 0; i < this->adj[u].size(); i++){
            if(visited[this->adj[u][i]] == false){
                visited[this->adj[u][i]] = true;
                dist[this->adj[u][i]] = dist[u] + 1;
                pred[this->adj[u][i]] = u;
                queue.push_back(this->adj[u][i]);

                // We stop BFS when we find destination.
                if(this->adj[u][i] == dest)
                   return true;
            }
        }
    }

    return false;
}
