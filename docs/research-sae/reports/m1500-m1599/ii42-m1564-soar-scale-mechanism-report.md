# M1564 SOAR Scale-Mechanism Report

## Decision

**Stop the SOAR posting-source branch.**

The unchanged SOAR assignment strongly improves residual geometry on the
official full FiQA corpus, but the improvement does not amplify into useful
candidate access or retrieval quality. Do not replicate it on SciFact, tune
`lambda`, increase fanout, train a compiler, or productize this source.

## Surface

- Official FiQA: 57,638 documents, 648 queries, and 1,706 qrel edges.
- Frozen BGE embeddings and a 3,603-cell spherical codebook.
- Fixed two posting edges per document and `lambda=1` SOAR assignment.
- Exact 15% unique candidate union and dense reranking of touched documents.
- Deterministic centroid traversal plus route-cover oracle.
- ClearML task: `c97ba5566f384fca9f06d3dd693005ec`.
- Runtime: 1,654.5 seconds.

The qrels loader selected the raw official namespace with 648 query hits and
1,706 document hits. The first attempt exposed a prefixed-ID namespace bug;
the rerun used the audited namespace fix and is the only result interpreted
here.

## Geometry

| Diagnostic | Nearest two centroids | SOAR dual | Change |
| --- | ---: | ---: | ---: |
| score-error correlation | 0.352246 | 0.022455 | -93.63% |
| mean best route rank | 24.4108 | 20.1764 | -17.35% |
| p95 best route rank | 102 | 83 | -18.63% |
| top1 pair coverage | 0.178627 | 0.175386 | -0.003241 |
| top4 pair coverage | 0.419182 | 0.425957 | +0.006775 |
| top8 pair coverage | 0.570864 | 0.583225 | +0.012361 |

Both predeclared geometry checks pass. SOAR also improves route-load entropy
from `0.952175` to `0.966434`, reduces maximum route load from 201 to 135,
and lowers mean posting reads from `0.211050x` to `0.195289x` at identical
storage cost.

## Retrieval

| Source | Policy | O@10 | O@100 | O@256 | Recall@100 | NDCG@10 | MAP@100 | MRR@20 | CUB |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| centroid dual | centroid | 0.997222 | 0.989738 | 0.981216 | 0.733148 | 0.399796 | 0.337321 | 0.485030 | 0.896979 |
| SOAR dual | centroid | 0.997685 | 0.994151 | 0.987142 | 0.732542 | 0.399796 | 0.337276 | 0.485023 | 0.899897 |
| centroid dual | oracle | 1.000000 | 1.000000 | 0.985050 | 0.733984 | 0.400339 | 0.337638 | 0.485244 | 0.898421 |
| SOAR dual | oracle | 1.000000 | 1.000000 | 0.989354 | 0.733984 | 0.400339 | 0.337638 | 0.485244 | 0.901559 |

Under deterministic traversal, SOAR gains only `+0.000463` O@10,
`+0.004414` O@100, and `+0.005926` O@256. The required gains were `+0.04`
and `+0.03` at O@100/O@256. O@100 is only 22% of the `+0.0200` NFCorpus
gain rather than the required 2x scale amplification.

The route-cover oracle already has O@100 `1.0` for both assignments, so SOAR
has no oracle O@100 headroom to recover. Candidate upper bound improves
`+0.002917`, but Recall@100 decreases `-0.000606`, MAP@100 decreases
`-0.000045`, and MRR@20 is effectively flat. Better residual geometry does
not change the final retrieval boundary in a useful way.

## Conclusion

M1563 and M1564 establish a clean negative result:

1. correlated route residuals were real and SOAR fixes them;
2. the fix reduces posting reads and balances the inverted lists;
3. on full FiQA, nearest-two centroid routing already preserves 98.97% of
   dense top100 at the fixed budget;
4. the remaining product gap is therefore not caused by correlated second
   route assignment.

This closes the M1542-to-M1564 corpus-route family at the source-capacity
gate. The next unified-index route must target information absent from the
centroid vocabulary or the fixed route-score function, not another way to
choose redundant centroid edges.
