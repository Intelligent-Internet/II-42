# II-42 M1932 Cross-Corpus Semantic Budget Contract

## Question

M1931 proved that lexical and semantic postings can share one physical index
and one additive dot product. Its remaining gap to the unpruned M1914 parent is
mainly document-side semantic pruning: selecting atoms only by document impact
creates high-DF posting lists and discards some atoms useful to unseen queries.

M1932 asks whether the same exact semantic posting budget can be allocated more
effectively using only information available before relevance evaluation:

- query-side atom usage estimated from external corpora;
- document frequency measured while building the target corpus index;
- the original M1914 document impact.

No qrel, dataset-specific threshold, ANN result, or post-retrieval gate may
participate in atom selection.

## Evidence

The experiment follows two independent observations.

1. M1931 native replay matches offline quality exactly, so the scorer and
   engine are not the current bottleneck.
2. DF-FLOPS shows that per-document sparsity alone does not control corpus-wide
   posting cost: a few high-DF terms can dominate traversal even at moderate
   average nnz. The Unified Framework for Learned Sparse Retrieval similarly
   identifies document term weighting as the main effectiveness component.

M1932 therefore changes the document allocation policy, not the score values,
query calibration, encoder, or posting budget.

## Fixed Surface

- lexical substrate: exact M1930 BM25 posting cache;
- semantic parent: M1914 calibrated Granite surface;
- semantic budget: exactly `1.0x` lexical posting count per corpus;
- query policy: frozen M1931 `rms_m4` with its proven semantic floor;
- score: one additive dot product over disjoint lexical/semantic namespaces;
- rows: FiQA, ArguAna, NFCorpus, and SciFact.

The baseline is reproduced by balanced per-document top-impact pruning. A
candidate may only change which semantic atoms survive. Surviving postings keep
their original M1914 weights.

## Candidate Allocation Families

For a semantic posting with original impact `w`, cross-corpus query utility
`u`, and local semantic inverse document frequency `idf`, rank it within its
document by one of:

```text
impact:           w
idf:              w * idf^gamma
query_mean:       w * smooth(mean_query_usage)
query_rms:        w * smooth(rms_query_usage)
query_df:         w * smooth(query_activation_rate)
query_rms_idf:    w * smooth(rms_query_usage)^beta * idf^gamma
query_df_idf:     w * smooth(query_activation_rate)^beta * idf^gamma
```

Smoothing is global and fixed before evaluation. It prevents an atom absent
from a small external query sample from being assigned impossible utility.
`beta < 1` is a predeclared tempering control for heavy-tailed query usage;
`gamma` controls the corpus-DF penalty. All policies must retain the exact
posting count of the impact baseline.

## Generalization Protocol

The primary result is nested leave-one-dataset-out evaluation.

For each heldout corpus:

1. estimate query utility from the other three corpora without qrels;
2. select one global allocation formula using only the three training rows;
3. apply the same formula and utility vector to the heldout corpus;
4. use only heldout corpus document frequency as index-build metadata;
5. report heldout quality and posting-shape metrics.

The all-four-corpus result is diagnostic only because its utility statistics
see the evaluation query distribution. Promotion depends on the heldout result.

## Gates

Authorize the allocation policy for native replay only when all conditions
hold against frozen M1931:

1. heldout macro Recall@100 or candidate upper bound improves by at least
   `0.002` absolute;
2. heldout macro NDCG@10, MAP@100, and MRR@20 each retain at least `99.5%`;
3. no heldout row loses more than `0.01` Recall@100 or `1.5%` NDCG/MRR;
4. exact semantic posting counts are unchanged;
5. macro maxDF and head-1%-posting share do not worsen;
6. query-weighted posting touches and matched-document counts do not worsen;
7. the M1931 impact baseline is reproduced within `1e-5` on every quality
   metric before any candidate is interpreted.

If no formula passes, stop before neural residual training. The negative result
means the fixed M1914 support and budget, rather than output-head depth, is the
current limit.

## Training Boundary

Only a policy that passes the heldout and native gates can become a training
teacher. Training then starts with the smallest auditable object: an atom
utility/output calibrator that predicts the accepted allocation while keeping
M1914 impacts and M1931 query calibration frozen. It must use ClearML on an
available remote GPU node.

Longer or joint encoder training is authorized only if two independent heldout
corpora show simultaneous quality and posting-shape improvement. Loss reduction
alone is never a scale signal.

## References

- Porco et al., *An Alternative to FLOPS Regularization to Effectively
  Productionize SPLADE-Doc*, SIGIR 2025, arXiv:2505.15070.
- Nguyen et al., *A Unified Framework for Learned Sparse Retrieval*, ECIR
  2023, arXiv:2303.13416.
