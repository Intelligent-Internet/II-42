# M1546-M1549 Native Recall Breakthrough

Date: 2026-07-10

## Decision

**Promote M1549 as the native hybrid recall-breakthrough milestone.**

On the complete official BEIR15 surface, the fixed M1549 policy improves
candidate upper bound, Recall@100, and MAP@100 on every dataset. It preserves
NDCG@10 and MRR@20 exactly and produces no row-level quality harm.

This is a product-architecture result, not a claim that one sparse posting
algebra has replaced dense access. M1549 uses the existing native VectorChord
and II42 BM25 access paths under one auditable retrieval lifecycle.

## Frozen Policy

M1549 does not train a selector or tune per dataset:

1. retrieve the native VectorChord dense top256;
2. add up to 16 unique native II42 BM25 candidates, for at most 272 candidates;
3. preserve dense ranks 1 through 99 exactly;
4. choose rank 100 from the remaining union by BM25 rank, then dense rank and
   document ID for deterministic tie-breaking.

The candidate increase is bounded to 6.25%. The ranking rule can only change
rank 100, so O@10 is 1.0 by construction and NDCG@10/MRR@20 cannot regress.
Recall and MAP remain subject to the strict measured row gate.

## Official15 Result

The matrix contains 46,417 official queries. Qrels are read only after native
candidate generation and ranking.

| Variant | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| VectorChord dense256 | 0.544887 | 0.412030 | 0.670904 | 0.650741 | 0.733426 | 1.000000 |
| II42 BM25 top256 | 0.374297 | 0.274629 | 0.563177 | 0.475641 | 0.637789 | 0.243894 |
| Additive append | 0.544887 | 0.412030 | 0.670904 | 0.650741 | 0.770347 | 1.000000 |
| **M1549 top99** | **0.544887** | **0.412400** | **0.684776** | **0.650741** | **0.770347** | **0.990046** |

M1549 versus dense:

- candidate upper bound: `+0.036921`;
- Recall@100: `+0.013873`;
- MAP@100: `+0.000371`;
- NDCG@10: `+0.000000`;
- MRR@20: `+0.000000`;
- dataset harms: none.

## Per-Dataset Deltas

Every row improves CUB, Recall, and MAP. NDCG and MRR deltas are zero on all
15 rows.

| Dataset | CUB delta | Recall@100 delta | MAP@100 delta |
| --- | ---: | ---: | ---: |
| arguana | +0.019272 | +0.004283 | +0.000043 |
| climate-fever | +0.005201 | +0.001488 | +0.000028 |
| cqadupstack | +0.027518 | +0.013950 | +0.000151 |
| dbpedia-entity | +0.084451 | +0.019554 | +0.000811 |
| fever | +0.055336 | +0.025778 | +0.000266 |
| fiqa | +0.011240 | +0.000694 | +0.000011 |
| hotpotqa | +0.060702 | +0.033558 | +0.000534 |
| msmarco | +0.057077 | +0.004787 | +0.001092 |
| nfcorpus | +0.050233 | +0.019433 | +0.000552 |
| nq | +0.027400 | +0.009053 | +0.000103 |
| quora | +0.011166 | +0.007585 | +0.000092 |
| scidocs | +0.014883 | +0.004300 | +0.000134 |
| scifact | +0.064444 | +0.051667 | +0.000533 |
| trec-covid | +0.013580 | +0.000201 | +0.000085 |
| webis-touche2020 | +0.051319 | +0.011761 | +0.001125 |

## Evidence Chain

M1546 first tested the missing architecture on exact-BGE reference candidates.
A dense240 plus lexical16 fixed-budget union increased canary macro CUB by
0.004104, authorizing native validation, but its scorer slightly reduced MAP
and NDCG.

M1547 moved the same question onto the local engineering path: official PPLX
embeddings, VectorChord, II42 BM25, and PostgreSQL. Replacement improved most
rows but reduced webis-touche2020 CUB by 0.025724. This isolated the error:
lexical rescue was useful, but consuming dense budget was not row-safe.

M1548 changed the source structurally instead of tuning a loss. The union
became additive, making its candidate capacity monotonic. A fixed top95 policy
passed the first 11 rows, but climate-fever had tiny Recall and MAP regressions.
The strict gate rejected it; no tolerance was loosened after observing the
result.

M1549 reused the pre-existing M674 top99 safety endpoint rather than sweeping
top96 through top99. It removed the climate harm, retained useful movement,
and then passed all 15 official rows.

## Why No New Training Was Run

The M1540-M1545 review established that candidate access, not scorer capacity,
was the limiting component. Historical selector work also repeatedly showed
that oracle-safe actions were not reliably observable from deployable query
features.

M1549 resolves the measured bottleneck with a deterministic global policy and
passes every predeclared quality constraint. Training another selector now
would add variance, qrels-leakage risk, and a new generalization surface without
an unresolved signal that requires learning. The evidence therefore rejects
blind training at this stage.

## Evaluator Engineering Fix

The initial heavy-row evaluator alternated VectorChord and BM25 for every
query. On fever, the first 250 queries took 321.7 seconds because the two large
index families repeatedly displaced each other's cache.

M1549 now retrieves one source family at a time and restores the original
query order before applying the identical policy. A 20-query NFCorpus
equivalence smoke matched every non-latency aggregate exactly. The batched
evaluator completed full fever in 1,320 seconds and made complete official15
validation practical without sampling, changing probes, or using an offline
scan.

## Boundaries And Next Product Step

The following claims are supported:

- an exact native semantic/lexical candidate union contains complementary
  relevant documents on all official15 rows;
- one globally fixed top99 policy converts part of that capacity into
  row-safe Recall and MAP;
- the result survives the native database path and full official query set.

The following claim is not supported:

- a single URSI/SAE/unified-posting access path now replaces VectorChord.

The next product task is narrow: expose the frozen top99 tail policy through a
database-native fusion operator and benchmark end-to-end index size, QPS, and
latency. It must not reopen top-k, alpha, threshold, or learned-gate searches.

## Artifacts And Commits

- `runs/m1549f-native-official15-v1/summary.json`
- `runs/m1549f-native-official15-v1/summary.md`
- M1546 capacity gate: `5d202956`
- M1547 native evaluator: `b9956e6f`
- M1548 additive rescue: `5f1e5356`
- M1549 top99 endpoint: `45ebacb5`
- official15 compiler: `b201e03a`
- source-batched native evaluator: `66a5abfb`

M1546 reference audit ClearML task:
`318c7443ea984098bdaee8033f31bed9`.
