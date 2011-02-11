#pragma once
#include <lemon/lgf_reader.h> 
#include <lemon/list_graph.h> 
#include <lemon/smart_graph.h> 
#include <lemon/concepts/digraph.h>
#include <lemon/dfs.h> 
#include <lemon/preflow.h>
#include <lemon/elevator.h>
#include "TimeManager.h"
#include "GraphStats.h"

#include <string>
#include <set>

using namespace lemon;

typedef ListDigraph FlowGraph;

typedef int VType;
typedef FlowGraph::Node Node;
typedef FlowGraph::NodeIt NodeIt;
typedef FlowGraph::Arc Arc;
typedef FlowGraph::ArcIt ArcIt;
typedef FlowGraph::ArcMap<int> CapMap;
typedef FlowGraph::ArcMap<int> FlowMap;
typedef FlowGraph::NodeMap<bool> CutMap;
typedef FlowGraph::ArcMap<std::string> ArcStringMap;
typedef FlowGraph::NodeMap<std::string> NodeStringMap;

typedef Preflow<FlowGraph,CapMap> PreflowType;

class SimpGraph {

protected: 
  FlowGraph fg;
  int nextId; 
  CapMap fgCapacities;
  TimeManager& tm_;

  typedef std::map<int, std::string> IdToNameMap;
  typedef std::map<std::string, int> NameToIdMap;
	
  typedef std::map<Node, int> NodeToIdMap;
  typedef std::map<int, Node> IdToNodeMap;

  std::set<int> declIds;
  std::set<int> expIds;
  std::map<std::string, std::string> nameToPositionMap;
  
  IdToNameMap idToName;
  NameToIdMap nameToOutgoingId;
  NameToIdMap nameToIncomingId;
  std::map<std::string, std::string> nameToString;
  
  NodeToIdMap nodeToId_;
  IdToNodeMap idToNode_;

  void addNewNode(const std::string& name, bool decl);
  void addNameToGraph(const std::string& name, bool canDecl);
  Node addSuperSink(const std::set<std::string>& names);
  
  void computeDominators(std::map< Node,std::set<Node> >& dominators, const Node& r);
  void computeImmediateDominatorsFast(std::map< Node,std::set<Node> >& dominators, const Node& r);
  void compactGraphFromImmediateDominators(std::map< Node, Node>& idoms);
  void pruneGraphFromDominators(const std::map< Node, std::set< Node > >& dominators);
  bool isKeepNode(const Node& node);
  
  const std::string& nodeToString(Node n);
  int nodeToId(Node n);

  static const int INFINITY_HACK = 1000;
  
public:
	
  SimpGraph(TimeManager& tm) : nextId(0), fgCapacities(fg), tm_(tm) { }

  const std::string& getNameForId(int id);
  int getOutgoingIdForName(const std::string& name, bool decl = false);
  int getIncomingIdForName(const std::string& name, bool decl = false);
  void addNameConnection(const std::string& name1, bool decl1, const std::string& name2, bool decl2);
  void addNamePositionConnection(const std::string& name, const std::string& pos);
  void addAsString(const std::string& name, const std::string& asString);
  const FlowGraph& getFlowGraph();
  void outputToFile(const std::string& fileName);
  void pruneFlowGraph(const std::string& name1, const std::set<std::string>& names);
  void copySimpGraph(SimpGraph& simpGraph);
  void performMinimumCut(const std::string& startName);
  void getStats(GraphStats& graphStats);
  std::map<Node,std::string> getNodeStringMap() {
	  // fix the copy-on-return thing
	  std::map<Node,std::string> returnMap;
	  for(NodeIt v(this->fg); v != INVALID; ++v) {
		  returnMap[v] = nodeToString(v);
	  }
	  return returnMap;
  }
};
