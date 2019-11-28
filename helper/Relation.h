/*
 * Relation.h
 *
 *  Created on: Nov 27, 2019
 *      Author: Kadir Yanık
 *  Algorithm reference: https://www.geeksforgeeks.org/shortest-path-unweighted-graph
 */

#ifndef HELPER_RELATION_H_
#define HELPER_RELATION_H_

#include <bits/stdc++.h>
#include "conf.h"

using namespace std; 

class Relation {
public:
  Relation(int);
  virtual ~Relation();
  void addEdge(int src, int dest);
  vector<int> getShortestDistance(int src, int dest);
private:
  bool BFS(int src, int dest, int pred[], int dist[]);

  int v;
  vector<int> *adj;
};

/*------------------------------------------------------------------------------*/
int getIndexFromId(int id);
int getIdFromIndex(int index);

#endif /* HELPER_RELATION_H_ */
