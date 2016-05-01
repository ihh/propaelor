#include "sampler.h"
#include "util.h"

TreeNodeIndex Sampler::randomInternalNode (const Tree& tree, random_engine& generator) {
  vguard<TreeNodeIndex> intNodes;
  intNodes.reserve (tree.nodes() / 2);
  for (TreeNodeIndex n = 0; n < tree.nodes(); ++n)
    if (!tree.isLeaf(n))
      intNodes.push_back (n);
  return random_element (intNodes, generator);
}

vguard<SeqIdx> Sampler::guideSeqPos (const AlignPath& path, AlignRowIndex row, AlignRowIndex guideRow) {
  vguard<SeqIdx> guidePos;
  const auto cols = alignPathColumns (path);
  guidePos.reserve (cols);
  const AlignRowPath& rowPath = path.at(row);
  const AlignRowPath& guideRowPath = path.at(guideRow);
  SeqIdx pos = 0;
  for (AlignColIndex col = 0; col < cols; ++col) {
    if (rowPath[col])
      guidePos.push_back (pos);
    if (guideRowPath[col])
      ++pos;
  }
  return guidePos;
}

TokSeq Sampler::removeGapsAndTokenize (const FastSeq& gapped) const {
  TokSeq tok;
  tok.reserve (gapped.length());
  for (auto c : gapped.seq)
    if (!Alignment::isGap (c))
      tok.push_back (tokenize (c, model.alphabet));
  return tok;
}

Sampler::Move::Move (Type type, const History& history)
  : type (type),
    oldHistory (history)
{ }

Sampler::SampleNodeMove::SampleNodeMove (const History& history, Sampler& sampler, random_engine& generator)
  : Move (SampleNode, history)
{
  node = Sampler::randomInternalNode (history.tree, generator);
  parent = history.tree.parentNode (node);
  Assert (history.tree.nChildren(node) == 2, "Non-binary tree");
  leftChild = history.tree.getChild (node, 0);
  rightChild = history.tree.getChild (node, 1);

  const TreeBranchLength lDist = history.tree.branchLength(node,leftChild);
  const TreeBranchLength rDist = history.tree.branchLength(node,rightChild);
  
  TreeBranchLength leftChildClosestLeafDistance, rightChildClosestLeafDistance;
  const TreeNodeIndex parentClosestLeaf = history.tree.closestLeaf (parent);
  const TreeNodeIndex leftChildClosestLeaf = history.tree.closestLeaf (leftChild, &leftChildClosestLeafDistance);
  const TreeNodeIndex rightChildClosestLeaf = history.tree.closestLeaf (rightChild, &rightChildClosestLeafDistance);

  const bool parentUsesLeftChildEnvelope = leftChildClosestLeafDistance + lDist < rightChildClosestLeafDistance + rDist;

  const vguard<SeqIdx> leftChildEnvPos = guideSeqPos (sampler.guide.path, leftChild, leftChildClosestLeaf);
  const vguard<SeqIdx> rightChildEnvPos = guideSeqPos (sampler.guide.path, rightChild, rightChildClosestLeaf);

  const GuideAlignmentEnvelope env (sampler.guide.path, leftChildClosestLeaf, rightChildClosestLeaf, sampler.maxDistanceFromGuide);

  const TokSeq lTok = sampler.removeGapsAndTokenize (history.gapped[leftChild]);
  const TokSeq rTok = sampler.removeGapsAndTokenize (history.gapped[rightChild]);
  const TokSeq pTok = sampler.removeGapsAndTokenize (history.gapped[parent]);

  AlignmentMatrix alignMatrix (sampler.model, lTok, rTok, lDist + rDist, env, leftChildEnvPos, rightChildEnvPos);

  // WRITE ME
}


Sampler::AlignmentMatrix::AlignmentMatrix (const RateModel& model, const TokSeq& xSeq, const TokSeq& ySeq, TreeBranchLength dist, const GuideAlignmentEnvelope& env, const vguard<SeqIdx>& xEnvPos, const vguard<SeqIdx>& yEnvPos)
  : model (model),
    xSeq (xSeq),
    ySeq (ySeq),
    dist (dist),
    env (env),
    xEnvPos (xEnvPos),
    yEnvPos (yEnvPos)
{
  // WRITE ME
}
