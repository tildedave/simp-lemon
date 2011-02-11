#pragma once
            
#include <lemon/dfs.h> 
#include <set>
#include <string>

template<class G> 
class FastDominators
{
	typedef typename G::Node Node;
	typedef typename G::NodeIt NodeIt;
	typedef typename G::Arc Arc;
	typedef typename G::ArcIt ArcIt;
	typedef typename G::InArcIt InArcIt;
	typedef typename G::OutArcIt OutArcIt;
	typedef std::map<Node,Node> NodeMap;
	typedef std::map<Node,int> NodeIntMap;
	typedef std::map<Node,std::string> NodeStringMap;
	
private:
	class FastDominVisitor: public lemon::DfsVisitor<G> {
		
	protected:
		const G& g;
		
	public:
		
		FastDominVisitor(const G& g_) : g(g_) { }
		
		void reach(const Node& node) {
			numReached++;
			sdno[node] = numReached;
			ndfs[numReached] = node;
			label[node] = node;
			ancestor[node] = dummyNode; 
			child[node] = dummyNode;
			size[node] = 1;
		}
		
		void discover(const Arc& arc) {
			parent[g.target(arc)] = g.source(arc); 
		}
	};
	
protected:

	int numReached;
	Node dummyNode;
	NodeMap ancestor;
	NodeMap child;
	NodeMap label;
	NodeMap parent;
	NodeIntMap sdno;
	NodeIntMap size;
	NodeStringMap nsm;

	std::map<int,typename G::Node> ndfs;
	
	void compress(const Node& v) {
		if (ancestor[ancestor[v]] != dummyNode) {
			compress(ancestor[v]);
			if (sdno[label[ancestor[v]]] < sdno[label[v]]) {
				label[v] = label[ancestor[v]];
			}
			ancestor[v] = ancestor[ancestor[v]];
		}
	}
	
	const Node& eval(const Node& v) {
		if (ancestor[v] == dummyNode) {
			return label[v];
		}
		else {
			compress(v);
			if (sdno[label[ancestor[v]]] >= sdno[label[v]])
				return label[v];
			else
				return label[ancestor[v]];
		}
	}
	
	void link(const Node& v, const Node& w) {
		Node s = w;
		Node tmp;
		while (sdno[label[w]] < sdno[label[child[s]]]) {
			if (size[s] + size[child[child[s]]] >= 2 * size[child[s]]) {
				ancestor[child[s]] = s;
				child[s] = child[child[s]];
			}
			else {
				size[child[s]] = size[s];
				ancestor[s] = child[s];
				s = ancestor[s];
			}
		}
		label[s] = label[w];
		size[v] += size[w];
		if (size[v] < 2 * size[w]) {
			tmp = s;
			s = child[v];
			child[v] = tmp;
		}
		while (s != dummyNode) {
			ancestor[s] = v;
			s = child[s];
		}
	}
	
	void dfs(const G& graph, const Node& node) {
		numReached++;
		sdno[node] = numReached;
		ndfs[numReached] = node;
		label[node] = node;
		ancestor[node] = dummyNode; 
		child[node] = dummyNode;
		size[node] = 1;
		for(OutArcIt e(graph, node); e != INVALID; ++e) {
			Node w = graph.target(e);
			if (sdno[w] == 0) {
				parent[w] = node;
				dfs(graph, w);
			}
		}
	
	}
		
public:
	FastDominators(const NodeStringMap& nsm_) : nsm(nsm_) { }
	virtual ~FastDominators() { }
	
	void computeImmediateDominatorsFast(const G& graph, const Node& r, std::map< Node, Node >& idom) {
//		std::cout << "starting fast dominator computation" << std::endl;
		std::map<Node, std::set<Node> > bucket;
		std::map<Node, Node> ancestor, label;
		
		for(NodeIt va(graph); va != INVALID; ++va) {
			bucket[va].clear();
			sdno[va] = 0;
		}
		bucket[dummyNode].clear();
		sdno[dummyNode] = 0;
		
		size[dummyNode] = 0;
		sdno[dummyNode] = 0;
		ancestor[dummyNode] = dummyNode;
		label[dummyNode] = dummyNode;
//		std::cout << "about to run DFS" << std::endl;
//		FastDominVisitor fdv(graph);
//		DfsVisit<G> dfsAgent(graph, fdv);
		numReached = 0;
		dfs(graph, r);
//		dfsAgent.run(r);
		
//		std::cout << "done running DFS " << std::endl;
		for(int i = numReached; i >= 2; i--) {
			Node w = ndfs[i];
			//std::cout << i << ": " << nsm[w] << std::endl;
			for(InArcIt inc(graph, w); inc != INVALID; ++inc) {
				Node v = graph.source(inc);
				Node u = eval(v);
				if (sdno[u] < sdno[w])
					sdno[w] = sdno[u];
			}

			bucket[ndfs[sdno[w]]].insert(w);
			Node wParent = parent[w];

			link(wParent, w);
			while (!bucket[wParent].empty()) {
				Node v = *(bucket[wParent].begin());
				bucket[wParent].erase(v);
				Node u = eval(v);
				if (sdno[u] < sdno[v])
					idom[v] = u;
				else 
					idom[v] = wParent;
			}
		}

		for(int i = 2; i <= numReached; i++) {
			Node w = ndfs[i];
			if (idom[w] != ndfs[sdno[w]])
				idom[w] = idom[idom[w]];
		}
		
		for(int i = 0; i <= numReached; ++i) {
			Node w = ndfs[i];
			nsm[w];
			nsm[idom[w]];
			//so, commenting out the following line causes things to 
			//break (dominators failed).  what?
			//std::cout << nsm[w] << " DOM " << nsm[idom[w]] << std::endl;
		}
		//std::cout << "all done" << std::endl;
	}
};
