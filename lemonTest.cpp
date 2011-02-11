#define TIXML_USE_STL
#include "tinyxml.h"
#include "SimpGraph.h"
#include "TimeUtil.h"
#include "TimeManager.h"
#include "GraphStats.h"

#include <iostream>
#include <iomanip> 
#include <fstream>
#include <utility>
#include <map>
#include <string>
#include <stdlib.h>
#include <stdio.h>

using namespace lemon; 

void do_minimum_cut_on_lgf_graph(const std::string& filename);
void do_xml_read(const std::string& filename);

std::map<std::string, GraphStats> graphStats;

int main(int argc, char *argv[]) { 

	if (argc != 3) {
		std::cout << "missing argument: specify a filename with either -xml or -lgf" << std::endl;
		return 0;
	}

	std::string arg(argv[1]);
	std::string fileName(argv[2]);

	std::cout  << "read from " << argv[2] << std::endl;

	if (arg == "-lgf") {
		do_minimum_cut_on_lgf_graph(fileName);
	}
	else if (arg == "-xml") {
		do_xml_read(fileName);
	}

	std::cout << "all done!" << std::endl; 
}

void do_minimum_cut_on_lgf_graph(const std::string& filename) {
	std::ifstream fileStream(filename.c_str());
	std::stringstream lgf_stringstream;
	std::string line;

	while(!fileStream.eof()) {
		getline(fileStream, line);
		lgf_stringstream << line << std::endl;
	}

	FlowGraph gr; 
	FlowGraph::ArcMap<int> c(gr);

	Node s, t;
	CapMap cap(gr);
	FlowGraph::ArcMap<int> arcLabelMap(gr);
	FlowGraph::NodeMap<int> nodeLabelMap(gr);
	ArcStringMap arcStr(gr);
	NodeStringMap nodeStr(gr);

	//  std::istringstream input(lgf_stringstream.str());
	//std::cout << lgf_stringstream.str() << std::endl;
	DigraphReader<FlowGraph>(gr,lgf_stringstream).
	node("source", s).
	node("target", t).
	arcMap("capacity", cap).
	arcMap("label", arcLabelMap).
	arcMap("string",arcStr).
	nodeMap("string",nodeStr).
	nodeMap("label",nodeLabelMap).
	run();

	PreflowType preflow_test(gr, cap, s, t);
	preflow_test.run();

	//FlowMap flow(preflow_test.flowMap());

	//  for(ArcIt e(gr); e != INVALID; ++e) {
	//	  std::cout << "edge: " << preflow_test.flowMap()[e] << " vs " << cap[e] << std::endl; 
	//  }
	//  for(ArcIt e(gr); e != INVALID; ++e) {
	std::cout << "flow value: " << preflow_test.flowValue() << std::endl;
	//  }
	//  
	//  for(NodeIt v(gr); v != INVALID; ++v) {
	//	  if (v == s || v == t) continue;
	//	  for(FlowGraph::OutArcIt e(g,v); e != INVALID; ++e) {
	//
	//	  }
	//  }

	FlowGraph saturatedGraph;
	DigraphCopy<FlowGraph, FlowGraph> copyGraph(gr, saturatedGraph);
	FlowGraph::ArcMap<FlowGraph::Arc> acr(saturatedGraph);
	FlowGraph::NodeMap<FlowGraph::Node> ncr(saturatedGraph);
	FlowGraph::ArcMap<FlowGraph::Arc> ar(gr);
	FlowGraph::NodeMap<FlowGraph::Node> nr(gr);
	copyGraph.arcCrossRef(acr).nodeCrossRef(ncr).nodeRef(nr).arcRef(ar);
	copyGraph.run();

	for(ArcIt e(gr); e != INVALID; ++e) {
		saturatedGraph.erase(ar[e]);
	}

	for(ArcIt e(gr); e != INVALID; ++e) {
		Node source = nr[gr.source(e)];
		Node target = nr[gr.target(e)];

		if (preflow_test.flowMap()[e] < cap[e]) {
			saturatedGraph.addArc(source,target);
		}
		if (preflow_test.flowMap()[e] > 0) {
			saturatedGraph.addArc(target,source);
		}
	}

	Dfs<FlowGraph> dfsAgent(saturatedGraph);
	//  dfsAgent.processedMap(processedMap);
	dfsAgent.run(nr[s]);

	for(ArcIt e(gr); e != INVALID; ++e) {
		Node source = gr.source(e);
		Node target = gr.target(e);

		//	  std::cout << "investigating edge: " << nodeStr[gr.source(e)] << " -> " << nodeStr[gr.target(e)] << std::endl;
		//	  std::cout << "\t(" << dfsAgent.reached(nr[source]) << "," << dfsAgent.reached(nr[target]) << ")" << std::endl; 

		if (dfsAgent.reached(nr[source]) == true && dfsAgent.reached(nr[target]) == false) {
			std::cout << "cut: " << nodeStr[source] << " -> " << nodeStr[target] << " (" << nodeLabelMap[source] << "," << nodeLabelMap[target] << ")" << std::endl;
		}
	}

	//  Node currentNode(nr[t]);
	//  std::vector<std::string> pathBack;
	//  do {
	//	  std::cout << "trace: " << nodeStr[ncr[currentNode]] << std::endl;
	//	  pathBack.push_back(nodeStr[ncr[currentNode]]);
	//	  currentNode = dfsAgent.predNode(currentNode);
	//  } while (currentNode != nr[s]);
	//  pathBack.push_back(nodeStr[s]);
	//  
	//  for(std::vector<std::string>::reverse_iterator ri(pathBack.rbegin());
	//  	  ri != pathBack.rend();
	//  	  ++ri) {
	//	std::cout << " " << *ri << std::endl;  
	//  }

	std::cout << std::endl;
}

void do_xml_read(const std::string& filename) {
  int numConstraints = 0;
	TiXmlDocument doc(filename.c_str());
	TimeManager tm;
	tm.start("total time");
	tm.start("read XML file");

	bool loaded = doc.LoadFile(); 
	if (!loaded) { 
		std::cout << "could not load " << filename << std::endl; 
	}

	SimpGraph flowGraph(tm);	
	std::map<std::string,int> latticeIds;
	std::set< std::pair<int, int> > latticeLeqs;
	std::multimap<int, int> checkNotLeq;
	int maxId = 0;

	TiXmlElement* root = doc.FirstChildElement( "constraint-set" );

	if (!root) {
		std::cout << "no root" << std::endl;
		return;
	}

	//std::cout << "we got a root!" << std::endl;
	// children of root are constraints
	for (TiXmlNode* child = root->FirstChild();
	child != 0;
	child = child->NextSibling()) {
	  if (child->Value() != NULL && std::string(child->Value()) == "con") {
			//std::cout << "con: " << child->Value() << std::endl;
	    ++numConstraints;
			TiXmlElement* lhs = child->FirstChildElement("lhs");
			TiXmlElement* rhs = child->FirstChildElement("rhs");

			//			if (lhs)
			//				lhs->
			//				std::cout << lhs->Attribute("name") << std::endl;
			//			if (rhs)
			//				std::cout << rhs->Attribute("name") << std::endl;
			if (lhs->FirstChildElement()->Attribute("name") == NULL || rhs->Attribute("name") == NULL) {
			      std::cout << "skipping a constraint without names" << std::endl;
			      continue;
			    }

			std::string lhsName(lhs->FirstChildElement()->Attribute("name"));
			std::string rhsName(rhs->Attribute("name"));
			//std::cout << lhsName << " <= " << rhsName << std::endl;

			bool rhsDecl = false;
			bool lhsDecl = false;

			if (rhs->Attribute("canDecl") != NULL || rhsName.find("NV") != std::string::npos)
				rhsDecl = true;
			if (lhs->FirstChildElement()->Attribute("canDecl") != NULL  || lhsName.find("NV") != std::string::npos)
				lhsDecl = true;

			flowGraph.addNameConnection(lhsName, lhsDecl, rhsName, rhsDecl);

			if (rhsDecl && child->FirstChildElement("asString") != NULL) {
				TiXmlElement* asStringElem = child->FirstChildElement("asString");
				std::string constraintString(asStringElem->FirstChild()->Value());
				
				TiXmlElement* becauseElem = child->FirstChildElement("because");
				std::string becauseString(becauseElem->FirstChild()->Value());

				std::string posString(child->FirstChildElement("pos")->FirstChild()->Value());

				if (constraintString.find("_{def}") > 0) {
				  flowGraph.addNamePositionConnection(rhsName, posString);
				  flowGraph.addAsString(rhsName, becauseString);
				}
			}

			 

			// for now ignore the other stuff
		}
		else if (child->Value() != NULL && std::string(child->Value()) == "lattice") {
			for (TiXmlElement* latticeChild = child->FirstChildElement();
			latticeChild != 0;
			latticeChild = latticeChild->NextSiblingElement()) {
				std::string latticeChildValue(latticeChild->Value());
				if (latticeChildValue == "label") {
					int id;
					if (latticeChild->QueryIntAttribute("id", &id) == TIXML_SUCCESS)
						latticeIds[latticeChild->Attribute("name")] = id;
				}
				if (latticeChildValue == "lt") {
					int lhsId, rhsId;
					if (latticeChild->QueryIntAttribute("lhs", &lhsId) == TIXML_SUCCESS &&
							latticeChild->QueryIntAttribute("rhs", &rhsId) == TIXML_SUCCESS) {
						if (lhsId > maxId)
							maxId = lhsId;
						if (rhsId > maxId)
							maxId = rhsId;
						latticeLeqs.insert(std::pair<int,int>(lhsId,rhsId));
					}
					else {
						std::cout << "failure!" << std::endl;
					}
				}
			}
		}
		else {
			//std::cout << "unmatched: " << child->Value() << std::endl;
		}

		std::set<std::pair<int,int> > pairsToCheck;
		for(int i = 0; i <= maxId; ++i) {
			for(int j = 0; j <= maxId; ++j) {
				if (i != j && latticeLeqs.find(std::pair<int,int>(i,j)) == latticeLeqs.end())
					checkNotLeq.insert(std::pair<int,int>(i,j));
			}
		}

		//std::cout << "done with this one" << std::endl;
		//flowGraph.outputToFile("out.dot");
	}
	tm.stop("read XML file");

	std::cout << "read " << numConstraints << " constraints" << std::endl;

	for(int i = 0; i <= maxId; ++i) {
		char buffer[10];
		sprintf(buffer,"%d",i);
		std::string iString("LATTICE#" + std::string(buffer));
		std::string incompString;

		std::set<std::string> incompNames;
		std::multimap<int,int>::iterator itUnequal = checkNotLeq.find(i);
		std::string currentName = iString;
		if (itUnequal != checkNotLeq.end()) {
			do {
				//std::cout << itUnequal->first << "," << itUnequal->second << std::endl;
				sprintf(buffer,"%d",itUnequal->second);
				std::string name = "LATTICE#" + std::string(buffer);
				if (incompNames.find(name) == incompNames.end())
					incompString += name + " ";
				incompNames.insert(name);
				++itUnequal;
			} while (itUnequal != checkNotLeq.upper_bound(i));
		}

		std::cout << "------------------------------------------------" << std::endl;
		std::cout << iString << " ~> " << incompString << std::endl;
		std::cout << "------------------------------------------------" << std::endl;
		
		GraphStats thisGraphStats;
		flowGraph.getStats(thisGraphStats);
		graphStats[iString] = thisGraphStats;

		tm.setName(currentName);
		SimpGraph copyGraph(tm);
		flowGraph.copySimpGraph(copyGraph);
		copyGraph.pruneFlowGraph(iString,incompNames);

		GraphStats prunedGraphStats;
		copyGraph.getStats(prunedGraphStats);
		graphStats[iString + " (pruned)"] = prunedGraphStats;

		//copyGraph.outputToFile("out.dot");
		copyGraph.performMinimumCut(iString);
		tm.unsetName();
		//       int firstId = flowGraph.getOutgoingIdForName(latticeFirst,false);
		//       int secondId = flowGraph.getOutgoingIdForName(latticeSecond,false);

		//       std::cout << "hi " << latticeFirst << " (" << firstId 
		// 		<< ") not comparable with " << latticeSecond << " (" << secondId << ")" 
		// 		<< std::endl;

	}
	tm.stop("total time");
	
	std::cout << std::fixed << std::setprecision(4);
	std::cout << std::setfill('-') << std::setw(60) << "" << std::endl;
	std::cout << std::setfill(' ') << std::right << std::setw(30) << "STATS" << std::endl;
	std::cout << std::setfill('-') << std::setw(60) << "" << std::endl;
	std::cout << std::setfill(' ');
	for(std::map<std::string,GraphStats>::iterator itStats = graphStats.begin();
	    itStats != graphStats.end();
	    ++itStats) {
	  std::cout << std::left << std::setw(25) << itStats->first  << ": " << itStats ->second.num_nodes << " nodes, " << itStats->second.num_edges << " edges" << std::endl;
	}
	tm.outputTimes();
}
