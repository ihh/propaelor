#include "span.h"
#include "logger.h"

AlignGraph::Partition::Partition (size_t n)
  : seqSetIdx (n),
    seqSet (n),
    nSets (n)
{
  for (size_t i = 0; i < n; ++i) {
    seqSetIdx[i] = i;
    seqSet[i].insert (i);
  }
}

bool AlignGraph::Partition::inSameSet (const AlignGraph::Edge& e) const {
  return seqSetIdx[e.row1] == seqSetIdx[e.row2];
}

void AlignGraph::Partition::merge (const AlignGraph::Edge& e) {
  if (!inSameSet(e)) {
    size_t idx1 = seqSetIdx[e.row1];
    size_t idx2 = seqSetIdx[e.row2];
    if (idx1 > idx2)
      swap (idx1, idx2);
    set<size_t>& set1 = seqSet[idx1];
    set<size_t>& set2 = seqSet[idx2];
    for (auto n2 : set2)
      seqSetIdx[n2] = idx1;
    set1.insert (set2.begin(), set2.end());
    set2.clear();
    --nSets;
  }
}

AlignGraph::AlignGraph (const vguard<FastSeq>& seqs, const RateModel& model, const double time, ForwardMatrix::random_engine& generator)
  : seqs (seqs),
    model (model),
    time (time),
    edges (seqs.size()),
    edgePath (seqs.size())
{
  Partition part (seqs.size());

  const size_t nEdges = min ((size_t) (seqs.size() * (seqs.size() - 1) / 2),
			     (size_t) ceil (log(seqs.size()) * (double) seqs.size() / log(2)));
  
  uniform_int_distribution<size_t> dist (0, seqs.size() - 1);
  for (size_t n = 0; n < nEdges || part.nSets > 1; ++n) {
    size_t src, dest;
    do {
      src = dist (generator);
      dest = dist (generator);
      if (dest < src)
	swap (src, dest);
    } while (src == dest || edgePath[src].find(dest) != edgePath[src].end());

    DiagonalEnvelope env (seqs[src], seqs[dest]);
    env.initFull();

    QuickAlignMatrix mx (env, model, time);
    AlignPath quickPath = mx.alignment();

    AlignPath& path = edgePath[src][dest];
    path[src] = quickPath[0];
    path[dest] = quickPath[1];
    
    Edge e;
    e.row1 = src;
    e.row2 = dest;
    e.lp = mx.end;

    edges[src].push (e);
    edges[dest].push (e);

    part.merge (e);

    LogThisAt(3,"Aligned " << seqs[src].name << " and " << seqs[dest].name << " (" << plural(n+1,"edge") << ", " << plural(part.nSets,"disconnected set") << ")" << endl);
  }
}

list<AlignPath> AlignGraph::minSpanTree() {
  list<AlignPath> paths;
  Partition part (seqs.size());
  while (part.nSets > 1) {
    Edge best;
    bool foundBest = false;
    for (auto src : part.seqSet.front()) {
      while (!edges[src].empty() && part.inSameSet (edges[src].top()))
	edges[src].pop();
      if (!edges[src].empty() && (!foundBest || best < edges[src].top())) {
	best = edges[src].top();
	foundBest = true;
      }
    }
    Assert (foundBest, "Found no valid edge");
    paths.push_back (edgePath[best.row1][best.row2]);
    part.merge (best);

    LogThisAt(3,"Joined " << seqs[best.row1].name << " and " << seqs[best.row2].name << " (" << plural(paths.size(),"edge") << ", " << plural(part.nSets,"disconnected set") << ")" << endl);
  }

  return paths;
}

AlignPath AlignGraph::mstPath() {
  const list<AlignPath> pathList = minSpanTree();
  const vguard<AlignPath> pathVec (pathList.begin(), pathList.end());
  return alignPathMerge (pathVec);
}
