# II-42 M1933 Semantic Budget Frontier Contract

## Question

M1932 showed that a static cross-corpus atom prior does not generalize across
FiQA, ArguAna, NFCorpus, and SciFact. It also exposed unequal pruning pressure:
the M1931 `1.0x lexical` semantic budget retains only 44% of the full M1914
FiQA support and 55% of ArguAna, versus 75-78% for NFCorpus and SciFact.

M1933 tests whether the current product budget, rather than the allocation
formula or training depth, is the active bottleneck.

## Fixed Variables

- M1914 document impacts and support;
- balanced per-document top-impact pruning;
- M1931 `rms_m4` qrels-free query calibration and semantic floor;
- exact lexical BM25 postings;
- one disjoint-namespace vector and one additive posting traversal;
- one global budget ratio for every corpus.

No atom prior, new loss, qrel-derived policy, per-dataset budget, or learned
reranker is allowed.

## Budget Grid

Evaluate semantic posting budgets of `1.0`, `1.125`, `1.25`, `1.375`, and
`1.5` times the lexical posting count, plus the unpruned M1914 support as a
diagnostic ceiling. The product gate is capped at `1.5x` semantic, or `2.5x`
total lexical-plus-semantic postings.

Report for every point:

- NDCG@10, MAP@100, Recall@100, MRR@20, and CUB@1000;
- exact semantic and total posting ratios;
- maxDF and head-1% posting share;
- query-weighted posting touches and matched documents.

## Generalization

Use leave-one-dataset-out selection. On each fold, select the smallest global
budget that passes the three training rows, then evaluate that fixed ratio on
the heldout row. A candidate must improve training macro Recall@100 or CUB by
at least `0.002`, retain `99.5%` of macro NDCG/MAP/MRR, and keep every training
row within `0.01` Recall and `1.5%` NDCG/MRR of M1931.

Authorize native replay only when all heldout rows remain safe and heldout
macro Recall or CUB improves by at least `0.002`. The baseline must reproduce
M1931 exactly before interpretation.

## Stop Rule

If no budget at or below `1.5x` passes LODO, stop budget expansion. The next
route must change the semantic support generator or document-local model; it
must not continue scalar budget or global atom-prior sweeps.

If a budget passes, native latency and compact-index bytes decide promotion.
Neural training is still deferred because the deterministic publisher already
implements the accepted policy exactly.
