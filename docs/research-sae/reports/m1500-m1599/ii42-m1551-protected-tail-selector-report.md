# M1551 Protected Tail Selector Report

## Result

Decision: **authorize a frozen dense-root lexical-residual capacity probe, not
full model training yet**.

M1551 fixes dense ranks 1-99 structurally and learns only the tail choice.  It
uses the same 1,271 native nfcorpus/scifact/fiqa queries and leave-one-dataset-
out protocol as M1550.  Teacher construction and model selection are qrels
free; qrels are used only for final retrieval metrics.

## Full-three results

| Policy | Boundary target@100 | Macro Recall gain kept | Gate |
| --- | ---: | ---: | --- |
| Deterministic lexical rank | 1.0000 | 1.0000 | pass, exact teacher parity |
| Pair-local learned tail | 0.5078 | 1.0240 | pass |
| Rank-context learned tail | 0.7529 | 0.8815 | pass |

All three protected policies preserve dense top99, NDCG@10, and MRR@20
exactly by construction.  The pair-local tail model retained:

- nfcorpus: `105.7%` of teacher Recall gain;
- scifact: `95.7%`;
- fiqa: `507.4%`, but the teacher gain on this row is only `0.000694`, so this
  ratio is not treated as strong standalone evidence;
- equal-row macro: `102.4%`.

The context model retained `88.1%` macro and remained row safe.  The raw
lexical order reproduced the exact teacher top100 and all quality metrics.

## What this proves

The M1550 failure was caused by asking one model to both preserve the dense
head and learn lexical movement.  Once head preservation becomes a hard index
invariant, qrels-free tail selection is learnable across held-out datasets.

This is a real positive signal, but it is not yet a unified encoder result.
The learned selector receives native BM25 score/rank features.  It proves that
the selection problem is tractable; it does not prove that a frozen dense-root
output head can generate those lexical postings or scores from text.

## Next experiment

M1552 must test that missing link directly:

1. freeze the 1024-dimensional dense-root embeddings and M549U semantic path;
2. train a low-rank query/document residual head only on M1549 tail targets;
3. use listwise candidate competition and dense/BM25 hard negatives;
4. evaluate by inserting one predicted tail document after frozen dense top99;
5. compare an unconstrained low-rank capacity ceiling with a sparse shared-head
   version suitable for unified postings.

The capacity ceiling must retain at least `50%` of teacher Recall gain on each
nontrivial held-out row and `80%` macro before sparse-head training is allowed.
If the dense-root ceiling fails, retain deterministic lexical terms as the
second head; distilling exact lexical identity from dense embeddings would be
both less reliable and less efficient than token publication.
