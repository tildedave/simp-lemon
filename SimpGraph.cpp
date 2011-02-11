#include "SimpGraph.h"

#include <fstream>
#include <algorithm>
#include <iomanip>

#include <lemon/adaptors.h>

#include "FastDominators.h"
#include "TimeManager.h"
#include "TimeUtil.h"

void
SimpGraph::addNewNode(const std::string& name, bool canDecl) {
	if (!canDecl) {
		int nodeId = this->nextId;
		++this->nextId;

		//std::cout << "need to add " << name << " to graph with id " << nodeId << std::endl;

		idToName[nodeId] = name;
		nameToIncomingId[name] = nodeId;
		nameToOutgoingId[name] = nodeId;

		Node newNode(fg.addNode());
		nodeToId_[newNode] = nodeId;
		this->idToNode_[nodeId] = newNode;
	}
	else {
		int incomingId = this->nextId;
		int outgoingId = this->nextId + 1;
		this->nextId += 2;

		//std::cout << "need to add " << name << " to graph with inc/outgoing id (" << incomingId << "," << outgoingId << ")" << std::endl;

		idToName[incomingId] = name;
		idToName[outgoingId] = name;
		nameToIncomingId[name] = incomingId;
		nameToOutgoingId[name] = outgoingId;

		const Node& incNode = this->fg.addNode();
		const Node& outNode = this->fg.addNode();

		nodeToId_[incNode] = incomingId;
		nodeToId_[outNode] = outgoingId;

		this->idToNode_[incomingId] = incNode;
		this->idToNode_[outgoingId] = outNode;

		const Arc& arc = this->fg.addArc(incNode, outNode);
		this->fgCapacities[arc] = 1;
		
		declIds.insert(incomingId);
		expIds.insert(incomingId);
		expIds.insert(outgoingId);
	}
}

const std::string& 
SimpGraph::getNameForId(int id) {
	return idToName[id];
}

int 
SimpGraph::getOutgoingIdForName(const std::string& name, bool decl) {
	NameToIdMap::iterator itIdMap = this->nameToOutgoingId.find(name);
	if (itIdMap != this->nameToOutgoingId.end())
		return itIdMap->second;

	addNameToGraph(name, decl);
	return getOutgoingIdForName(name, decl);
}

int 
SimpGraph::getIncomingIdForName(const std::string& name, bool decl) {
	NameToIdMap::iterator itIdMap = this->nameToIncomingId.find(name);
	if (itIdMap != this->nameToIncomingId.end())
		return itIdMap->second;
	// add it to the graph

	addNameToGraph(name, decl);
	return getIncomingIdForName(name, decl);
}

void
SimpGraph::addNameToGraph(const std::string& name, bool canDecl) {
	if (nameToIncomingId.find(name) != nameToIncomingId.end())
		return;

	addNewNode(name, canDecl);
}

const FlowGraph& 
SimpGraph::getFlowGraph() {
	return this->fg;
}

void 
SimpGraph::addNameConnection(const std::string& name1, bool decl1, const std::string& name2, bool decl2) {
	// are name1 and name2 known in the graph?
	int id1 = getOutgoingIdForName(name1, decl1);
	int id2 = getIncomingIdForName(name2, decl2);

	//std::cout << name1 << "(" << id1 << ") and " << name2 << "(" << id2 << ")" << std::endl;

	const Node& n1 = this->idToNode_[id1];
	const Node& n2 = this->idToNode_[id2];

	const Arc& connection = fg.addArc(n1, n2);
	this->fgCapacities[connection] = INFINITY_HACK;
}

void 
SimpGraph::outputToFile(const std::string& fileName) {
	std::ofstream os(fileName.c_str());

	os << "digraph G {" << std::endl;
	for(NodeIt v(this->fg); v != INVALID; ++v) {
		os << "\tnode" << nodeToId_[v] << " [label=\"" << idToName[nodeToId_[v]] << "\"]" << std::endl;
	}
	for(ArcIt e(this->fg); e != INVALID; ++e) {
		Node source = this->fg.source(e);
		Node target = this->fg.target(e);
		os << "\tnode" << nodeToId_[source] << " -> node" << nodeToId_[target] << " [label=\"" << this->fgCapacities[e] << "\"]" << std::endl;
	}
	os << "}" << std::endl;

	os.close();
}

void
SimpGraph::copySimpGraph(SimpGraph& returnGraph) {
	DigraphCopy<FlowGraph, FlowGraph> copyGraph(this->fg, returnGraph.fg);
	FlowGraph::ArcMap<FlowGraph::Arc> acr(returnGraph.fg);
	FlowGraph::NodeMap<FlowGraph::Node> ncr(returnGraph.fg);
	FlowGraph::ArcMap<FlowGraph::Arc> ar(this->fg);
	FlowGraph::NodeMap<FlowGraph::Node> nr(this->fg);
	copyGraph.arcCrossRef(acr).nodeCrossRef(ncr).nodeRef(nr).arcRef(ar);
	copyGraph.run();

	// reconstruct 

	returnGraph.nodeToId_.clear();
	returnGraph.idToNode_.clear();

	for(NodeToIdMap::iterator itNodeToId(this->nodeToId_.begin());
	itNodeToId != this->nodeToId_.end();
	++itNodeToId) {
		returnGraph.nodeToId_[nr[itNodeToId->first]] = itNodeToId->second;
	}

	for(IdToNodeMap::iterator itIdToNode(this->idToNode_.begin());
	itIdToNode != this->idToNode_.end();
	++itIdToNode) {
		returnGraph.idToNode_[itIdToNode->first] = nr[itIdToNode->second];
	}

	returnGraph.idToName = this->idToName;
	returnGraph.nameToIncomingId = this->nameToIncomingId;
	returnGraph.nameToOutgoingId = this->nameToOutgoingId;
	returnGraph.declIds = this->declIds;
	returnGraph.expIds = this->expIds;
	returnGraph.nameToString = this->nameToString;
	returnGraph.nameToPositionMap = this->nameToPositionMap;
	returnGraph.tm_ = this->tm_;
	
	returnGraph.nextId = this->nextId;
	//returnGraph.fgCapacities = this->fgCapacities;
	for(ArcIt e(this->fg); e != INVALID; ++e) {
		returnGraph.fgCapacities[ar[e]] = fgCapacities[e];
	}
}

void 
SimpGraph::pruneFlowGraph(const std::string& name1, const std::set<std::string>& names) {
	Node source = this->idToNode_[getOutgoingIdForName(name1)];
	Node superSink = addSuperSink(names);


	Dfs<FlowGraph> dfsAgent(this->fg);
	dfsAgent.run(source);
	ReverseDigraph<FlowGraph> reverseFlowGraph(this->fg);
	Dfs<ReverseDigraph<FlowGraph> > reverseDfsAgent(reverseFlowGraph);
	reverseDfsAgent.run(superSink);

	std::set<Node> deleteNodes;

	for(NodeIt v(this->fg); v != INVALID; ++v) {
		bool keep = true;
		if (dfsAgent.reached(v) != true) {
			keep = false;
		}
		if (reverseDfsAgent.reached(v) != true) {
			keep = false;
		}
		if (!keep)
			deleteNodes.insert(v);
	}

	for(std::set<Node>::iterator itDeleteNodes = deleteNodes.begin(); 
	itDeleteNodes != deleteNodes.end();
	++itDeleteNodes) {
		this->fg.erase(*itDeleteNodes);
	}

	std::map< Node,std::set<Node> > dominators;
	std::map< Node, Node> idoms;
	if (true) {
		FastDominators<FlowGraph> fd(this->getNodeStringMap());
		tm_.start("compute dominators");
		fd.computeImmediateDominatorsFast(this->fg, source, idoms);
		tm_.stop("compute dominators");
		tm_.start("compact graph");
		compactGraphFromImmediateDominators(idoms);
		tm_.stop("compact graph");
	}
	
	//std::cout << "done computing immediate dominators!" << std::endl;
	
//	for(std::map< Node,Node >::iterator itIdoms = idoms.begin();
//		itIdoms != idoms.end();
//		++itIdoms) {
//		std::cout << nodeToString(itIdoms->first) << " immediately dominated by " << nodeToString(itIdoms->second) << std::endl;
//	}
	
	if (false) {
		computeDominators(dominators, source);
		
		// for each incoming expression vertex, see if it is dominated by another incoming expression vertex
		pruneGraphFromDominators(dominators);
	}
	
	
//	for(std::map<Node,std::set<Node> >::iterator itDominators = declIds.begin();
//		itDominators != dominators.end();
//		++itDominators) {
//		// not sure if this is accepted, I'm just way too used to 
//		// Java at this point to remove these as references
//		const Node& n = itDominators->first;
//		const std::set<Node>& domN = itDominators->second;
//		
//	}
	
	
}

void 
SimpGraph::performMinimumCut(const std::string& startName) {
	int sourceId = getOutgoingIdForName(startName);
	int targetId = getIncomingIdForName("#SUPERSINK");

	Node source = this->idToNode_[sourceId];
	Node target = this->idToNode_[targetId];

	PreflowType pft(fg, this->fgCapacities, source, target);
	tm_.start("minimum cut");
	pft.run();
	tm_.stop("minimum cut");
	//std::cout << "source: " << sourceId << " target: " << targetId << std::endl;

	//outputToFile(startName + ".dot");

	if (pft.flowValue() > 0) {
		outputToFile("out.dot");
		std::cout << "flow value " << pft.flowValue() << std::endl;

		FlowGraph saturatedGraph;
		DigraphCopy<FlowGraph, FlowGraph> copyGraph(this->fg, saturatedGraph);
		FlowGraph::ArcMap<FlowGraph::Arc> acr(saturatedGraph);
		FlowGraph::NodeMap<FlowGraph::Node> ncr(saturatedGraph);
		FlowGraph::ArcMap<FlowGraph::Arc> ar(this->fg);
		FlowGraph::NodeMap<FlowGraph::Node> nr(this->fg);
		copyGraph.arcCrossRef(acr).nodeCrossRef(ncr).nodeRef(nr).arcRef(ar);
		copyGraph.run();

		for(ArcIt e(this->fg); e != INVALID; ++e) {
			saturatedGraph.erase(ar[e]);
		}

		for(ArcIt e(this->fg); e != INVALID; ++e) {
			Node source = nr[this->fg.source(e)];
			Node target = nr[this->fg.target(e)];

			if (pft.flowMap()[e] < this->fgCapacities[e]) {
				saturatedGraph.addArc(source,target);
			}
			if (pft.flowMap()[e] > 0) {
				saturatedGraph.addArc(target,source);
			}
		}

		Dfs<FlowGraph> dfsAgent(saturatedGraph);
		//  dfsAgent.processedMap(processedMap);
		dfsAgent.run(nr[source]);
		
		std::multimap<std::string, std::string> positionAndNVMap;
		int maxLength = 0;

		for(ArcIt e(this->fg); e != INVALID; ++e) {
			Node source = this->fg.source(e);
			Node target = this->fg.target(e);

			//	  std::cout << "investigating edge: " << nodeStr[this->fg.source(e)] << " -> " << nodeStr[this->fg.target(e)] << std::endl;
			//	  std::cout << "\t(" << dfsAgent.reached(nr[source]) << "," << dfsAgent.reached(nr[target]) << ")" << std::endl; 

			if (dfsAgent.reached(nr[source]) == true && dfsAgent.reached(nr[target]) == false) {
//				std::cout << "cut: " << nodeToString(source) << " -> " << nodeToString(target) << " (" << nodeToId(source) << "," << nodeToId(target) << ")" << std::endl;
//				std::cout << "\t" << nameToString[nodeToString(source)] << "," << nameToString[nodeToString(target)] << " (" << nameToPositionMap[nodeToString(target)] << ")" << std::endl;
				std::string posString(nameToPositionMap[nodeToString(target)]);
				positionAndNVMap.insert(std::pair<std::string, std::string>(posString, nameToString[nodeToString(source)] + " ("+ nodeToString(source) + ")"));
				maxLength = std::max(maxLength, (int) posString.length());
			}
		}
		
		for(std::multimap<std::string, std::string>::iterator itPositionAndNVMap = positionAndNVMap.begin();
			itPositionAndNVMap != positionAndNVMap.end();
			++itPositionAndNVMap) {
			std::cout << std::left << std::setw(maxLength + 2) << itPositionAndNVMap->first << ": " << itPositionAndNVMap->second << std::endl;
		}
	}
}

Node
SimpGraph::addSuperSink(const std::set<std::string>& names) {
	addNameToGraph("#SUPERSINK",false);
	Node superSink = this->idToNode_[getOutgoingIdForName("#SUPERSINK")];

	for(std::set<std::string>::iterator itIncompatibleNames = names.begin();
	itIncompatibleNames != names.end();
	++itIncompatibleNames) {
		Node target = this->idToNode_[getOutgoingIdForName(*itIncompatibleNames)];
		const Arc& a = this->fg.addArc(target,superSink);
		this->fgCapacities[a] = INFINITY_HACK;
	}

	return superSink;
}

const std::string& 
SimpGraph::nodeToString(Node n) {
	return getNameForId(nodeToId(n));
}

int 
SimpGraph::nodeToId(Node n) {
	return nodeToId_[n];
}

void
SimpGraph::computeDominators(std::map< Node,std::set<Node> >& dominators, const Node& r) {
	std::set<Node> d, t;
	std::set<Node> allNodes;
	for(NodeIt n(this->fg); n != INVALID; ++n)
		allNodes.insert(n);

	bool change = true;
	dominators.clear();
	std::set<Node> rSet;
	rSet.insert(r);
	for(NodeIt n(this->fg); n != INVALID; ++n) {
		if (n != r) {
			dominators[n] = allNodes;
		}
	}
	dominators[r] = rSet;
	
	std::cout << "DOMINATOR SOURCE " << nodeToString(r) << std::endl;

	do {
		change = false;
		for (NodeIt n(this->fg); n != INVALID; ++n) {
			if (n == r)
				continue;
			
			std::set<Node> t(allNodes);
			for (FlowGraph::InArcIt e(this->fg, n); e != INVALID; ++e) {
				Node p = this->fg.source(e);
				// remove from t all nodes that do not occur in dominators[p]
				std::set<Node> nodesToDelete;
				for(std::set<Node>::iterator itT = t.begin();
					itT != t.end();
					++itT) {
					if (dominators[p].find(*itT) == dominators[p].end()) {
						nodesToDelete.insert(*itT);
					}
				}
				for(std::set<Node>::iterator itNodesToDelete = nodesToDelete.begin();
				itNodesToDelete != nodesToDelete.end();
				++itNodesToDelete) {
					t.erase(*itNodesToDelete);
				}
			}

			t.insert(n);
			for(std::set<Node>::iterator itDominators = dominators[n].begin();
				itDominators != dominators[n].end();
				++itDominators) {
				if (t.find(*itDominators) == t.end()) {
					change = true;
					dominators[n] = t;
					break;
				}
			}
		}
	} while (change);
	
//	for(NodeIt n(this->fg); n != INVALID; ++n) {
//		std::cout << "dominators for " << nodeToString(n) << ": ";
//		for(std::set<Node>::iterator itNodes = dominators[n].begin();
//			itNodes != dominators[n].end();
//			++itNodes) {
//			std::cout << nodeToString(*itNodes) << "; ";
//		}
//		std::cout << std::endl;
//	}
} 

void 
SimpGraph::addAsString(const std::string& name, const std::string& asString) 
{
//	std::cout << name << "|->" << asString << std::endl;
	this->nameToString[name] = asString;
}

bool
SimpGraph::isKeepNode(const Node& node) {
	bool keep = expIds.find(nodeToId(node)) != expIds.end() || nodeToString(node).find("pc") == 0 || nodeToString(node).find("LATTICE") == 0;
	std::cout << "should we keep " << nodeToString(node) << "? " << keep << std::endl;
	return keep;
}


void 
SimpGraph::compactGraphFromImmediateDominators(std::map< Node, Node>& idoms)
{
	// want to iterate over the nodes and adjust them accordingly -- 
	// merge nodes with those are are dominated by it
	
//	std::map<Node, Node> clusterMap;
//	for(NodeIt v(this->fg); v != INVALID; ++v) {
//		clusterMap[v] = v;
//	}
//	
//	bool changed = true;
//	
//	do {
//		changed = false;
//		for (ArcIt e(this->fg); e != INVALID; ++e) {
//			const Node& s = clusterMap[fg.source(e)];
//			const Node& t = clusterMap[fg.target(e)];
//			if (!isKeepNode(s) && isKeepNode(t)) {
//				std::cout << "compact " << nodeToString(s) << " with " << nodeToString(t) << std::endl;
//				clusterMap[fg.source(e)] = fg.target(e);
//				changed = true;
//			}
//		}
//	} while (changed == true);
//	
//	do {
//		changed = false;
//		for (NodeIt v(this->fg); v != INVALID; ++v) {
//			// is clusterMap[v] idomed by something?
//			if (idoms.find(v) != idoms.end()) {
//				Node idom = idoms.find(v)->second;
//				Node clusterDom = clusterMap[idom];
//				if (declIds.find(nodeToId(clusterDom)) != declIds.end() && nodeToString(v) != nodeToString(clusterDom)) {
//					std::cout << "COMPACT: " << nodeToString(v) << "(" << nodeToId_[v] << ") immediately dominated by " << nodeToString(clusterMap[idoms[v]]) << " (" << nodeToId_[clusterMap[idoms[v]]] << ")" << std::endl;
//					std::cout << nodeToString(v) << " dominated by " << nodeToString(idom) << " (" << nodeToString(clusterDom) << ")" << std::endl;
//					// it's a declassifiable ID, so it is an incoming node
//					if (clusterDom != clusterMap[v]&& !(nodeToString(clusterDom) == nodeToString(v))) {
//						clusterMap[v] = clusterDom;
//						std::cout << "\tclustering " << nodeToString(v) << " to " << nodeToString(clusterDom) << std::endl;
//						if (nameToOutgoingId[nodeToString(v)] == nodeToId_[v]) {
//							// outgoing node -- find associated incoming node
//							FlowGraph::InArcIt e(this->fg, v);
//							Node incoming(this->fg.source(e));
//							clusterMap[incoming] = clusterDom;
//						}
//						changed = true;
//					}
//				}
//				else {
//					std::cout << nodeToString(v) << " vs " << nodeToString(clusterDom) << std::endl;
//				}
//			}
//		}
//		// now we are all clustered!
//		
//	} while (changed == true);

	
	// TODO: only iterate like 10 times or something.  Use a list of adjacencies 
	// so we only look at nodes that we actually care about.
	// How to do this?  a multimap?  that we continually iterate over.
	
	// TODO: compute transitiev closure of imemdiate dominators, but only k step or so.
	// should be able to do this in n log n time -- for each node, step along its dominators k times.

	std::map<Node, std::set<Node> > iteratedDominators;
	
	for(NodeIt n(fg); n != INVALID; ++n) {
		iteratedDominators[n].insert(idoms[n]); 
	}
	
	int k = 10;
	for(int i = 0; i < k; ++i) {
		for(NodeIt n(fg); n!= INVALID; ++n) {
			std::set<Node> toAdd;
			for(std::set<Node>::iterator it(iteratedDominators[n].begin());
				it != iteratedDominators[n].end();
				++it) {
				const Node& current = *it;
				toAdd.insert(idoms[current]);
			}
			iteratedDominators[n].insert(toAdd.begin(), toAdd.end());
		}
	}
	
//	for(NodeIt n(fg); n != INVALID; ++n) {
//		std::cout << nodeToString(n) << ": ";
//		for(std::set<Node>::iterator it(iteratedDominators[n].begin());
//			it != iteratedDominators[n].end();
//			++it) {
//			std::cout << nodeToString(*it) << "; ";
//		}
//		std::cout << std::endl;
//	}
	
	pruneGraphFromDominators(iteratedDominators);

#ifdef SAVE_THIS_CODE
	std::cout << "starting TC" << std::endl;
	int n = this->idToNode_.size();
	//std::map< int, std::map<int, bool > > tc;
	bool** tc = new bool*[n];

	for(int i = 0; i < n; ++i) {
		tc[i] = new bool[n];
		for(int j = 0; j < n; ++j)
			tc[i][j] = false;
	}
	
	for(std::map<Node, Node>::const_iterator itIdoms = idoms.begin();
		itIdoms != idoms.end();
		++itIdoms) {
		int i = nodeToId_[itIdoms->first];
		int j = nodeToId_[itIdoms->second];
		tc[i][j] = true;
		//std::cout << "connect " << nodeToString(itIdoms->first) << "(" << i << ") and " << nodeToString(itIdoms->second) << " (" << j << ")" << std::endl;
	}
	
	for (int j = 0; j < n; ++j) {
		for(int i = 0; i < n; ++i) {
			if (tc[i][j]) {
				for(int k = 0; k < n; ++k) {
					if (tc[j][k])
						tc[i][k] = true;
				}
			}
		}
	}
	std::cout << "done with TC" << std::endl;

	std::map< Node, std::set<Node > > dominators;
	
	for(int i = 0; i < n; ++i) {
		Node iNode = idToNode_[i];
		for(int j = 0; j < n; ++j) {
			if (tc[i][j]) {
				//std::cout << nodeToString(iNode) << " (" << i << ")" << " and " << nodeToString(idToNode_[j]) << " (" << j << ")" << std::endl;
				dominators[iNode].insert(idToNode_[j]);
			}
		}
	}
	pruneGraphFromDominators(dominators);
#endif	
}

void 
SimpGraph::pruneGraphFromDominators(const std::map< Node, std::set< Node > >& dominators) {
	std::set<Node> incomingIds;
	std::set<Node> prunedNodes;
	for(std::set<int>::iterator itIncoming = this->declIds.begin(); 
		itIncoming != this->declIds.end();
		++itIncoming) {
		//		incomingIds.insert(idToNode_[*itIncoming]);
		Node n(idToNode_[*itIncoming]);
		if (dominators.find(n) != dominators.end()) {
			std::set<Node> domN(dominators.find(n)->second);
			// domN contains the nodes that n dominates.
			for(std::set<Node>::iterator itDomN = domN.begin();
			itDomN != domN.end();
			++itDomN) {
				if (*itDomN != n && this->declIds.find(nodeToId(*itDomN)) != this->declIds.end() && prunedNodes.find(*itDomN) == prunedNodes.end()) {
				        //std::cerr << "time to get rid of " << nodeToString(*itDomN) << " as a candidate declassifier! (dominated by " << nodeToString(n) << ")" << std::endl;
					for(FlowGraph::OutArcIt outgoingFromDominators = FlowGraph::OutArcIt(this->fg,*itDomN);
					outgoingFromDominators != INVALID;
					++outgoingFromDominators) {
						fgCapacities[outgoingFromDominators] = INFINITY_HACK;
					}
					prunedNodes.insert(*itDomN);
				}
			}
		}
	}
}

void 
SimpGraph::addNamePositionConnection(const std::string& name, const std::string& pos) {
  this->nameToPositionMap[name] = pos;
}

void 
SimpGraph::getStats(GraphStats& graphStats) {
  int nodes = 0;
  int edges = 0;
  for(NodeIt n(this->fg); n != INVALID; ++n) {
    nodes++;
  }
  for(ArcIt e(this->fg); e != INVALID; ++e) {
    edges++;
  }
  graphStats.num_nodes = nodes;
  graphStats.num_edges = edges;
}
