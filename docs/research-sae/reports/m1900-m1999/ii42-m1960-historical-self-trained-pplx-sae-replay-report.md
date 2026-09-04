# M1960 Historical Self-Trained PPLX-SAE Native Replay

## Decision

**Broad9 quality gate: PASS; full15 fallback gate: REJECT.** M190 is
credible evidence that the historical self-trained route learned useful
retrieval structure, but it is not a deployable fallback for P2.1 or PPLX
dense.

This replay uses the exact audited M190 checkpoint, the same PPLX
1,024-dimensional source lineage as current VectorChord, complete
per-dataset corpora, and native PostgreSQL postings. Qrel labels
define the evaluation query set and final metrics only; they do not
influence atom impacts, parameters, candidate generation, or scoring.

M1963 closes the exact-BMP surface at 15/15 datasets and 46,417 official
queries. Raw M190 passes the dense Recall-retention floor (`0.959912`) but
fails the dense MAP-retention floor (`0.887020`). It beats P2.1 on macro
NDCG@10 and MRR@20, but loses MAP@100 and Recall@100; this does not satisfy
the frozen fallback contract.

The MSMARCO row uses the checksum-verified physical M150 PPLX artifact
recovered from Betty, not PostgreSQL half precision and not cross-hardware
rematerialization. The recovered source has 8,841,823 rows and document
SHA-256
`fa6e515b03add6cff23d3e4f19c91b7061f5b805dfeefaabbad94057dca9c4ba`.
Line count, schema, 1,024-dimensional vectors, document anchors, queries,
qrels, and all content signatures passed before the old recomputation was
stopped.

## Full15 Closure

| Method | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB@1000 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.374297 | 0.274610 | 0.562964 | 0.475641 | 0.735619 |
| PPLX dense | 0.544873 | 0.412036 | 0.670880 | 0.650711 | 0.810525 |
| P2.1 | 0.490809 | 0.371455 | 0.666885 | 0.595950 | 0.839658 |
| Raw M190 exact BMP | 0.492105 | 0.365484 | 0.643985 | 0.598575 | 0.812235 |

Fixed hybrid:

```text
minmax(latent_binary_bm25) + 0.5 * minmax(lexical_bm25)
```

The fixed hybrid is a quality-ceiling and complementarity diagnostic.
It currently retrieves from separate semantic and lexical candidate
sources, so it is not evidence for a single physical unified index.
Raw M190 is the product-shaped self-trained semantic backup surface.

## Historical Candidate Audit

These rows explain candidate selection; they are not one comparable
leaderboard. Continuity, official9, old full15, and common4 use
different corpora or query sets. Only the current native matrix below
can decide promotion.

| Route | Historical surface | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Decision |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| M130 raw SAE | continuity | 0.3394 | 0.3653 | 0.2570 | 0.1642 | dominated |
| M150 A1 C6 raw SAE | continuity | 0.3899 | 0.4363 | 0.3120 | 0.2107 | strong but original data non-canonical |
| M160 raw SAE | official9 | 0.6436 | 0.5483 | 0.4791 | 0.3382 | secondary control |
| M160 BM25+SAE | official9 | 0.6574 | 0.5764 | 0.5055 | 0.3573 | below dense on same surface |
| PPLX dense | old corrected full15 | 0.7092 | 0.6670 | 0.5874 | 0.4318 | reference only |
| M190 raw SAE | old corrected full15 | 0.6436 | 0.5979 | 0.5122 | 0.3656 | canonical self-trained checkpoint |
| M190 latent+BM25 w=0.5 | old corrected full15 | 0.6835 | 0.6245 | 0.5466 | 0.3973 | frozen replay candidate |
| M1914 compact learned sparse | common4 native | 0.7252 | 0.4906 | 0.4527 | 0.3539 | compact external control |

M180 and the M210-M222 scorer/calibrator families were eliminated
before new GPU work: M180 was dominated on its official partial
surface, while later score, gate, and tail variants produced only
small or unstable gains.

## Training And Generalization Contract

M190 is a 134,306,453-byte sparse head over the frozen 0.6B PPLX
trunk. Stage A trained a shared 16,384-feature hard-TopK96 SAE for
three epochs (`query_repeat=2`, 35,902,812 expected records/epoch)
with reconstruction, cosine, neighborhood KL/MSE, and fanout losses.
Stage B then ran 5,000 retrieval-refinement steps using recall,
multi/single-candidate CE, teacher KL, complement, reconstruction,
and K-budget terms.

Training excluded official test queries and qrel labels, but used
complete BEIR15 document corpora plus safe and synthetic queries.
Consequently this replay tests held-out queries and labels, not
unseen-corpus generalization. A later held-out corpus is required
before calling M190 broadly generalizable.

The literature also fixes the interpretation. DeepImpact and uniCOIL
make contextual learned impacts a first-class sparse representation,
so replacing M190 impacts with IDF is an ablation rather than an
automatic modernization. SPLADE-v3 makes mature pretraining and
hard-negative/multi-teacher supervision the relevant quality control;
Two-Step SPLADE separates retrieval quality from traversal; SPLATE
motivates testing a frozen dense interface before changing the trunk;
DF-FLOPS requires corpus-wide DF rather than average nnz as the cost
constraint.

## Macro Summary

| Method | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB@1000 | Dense overlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.408480 | 0.283779 | 0.562437 | 0.490586 | 0.730225 | n/a |
| PPLX dense / VectorChord | 0.513783 | 0.363891 | 0.645722 | 0.590565 | 0.809445 | n/a |
| P2.1 | 0.483010 | 0.342733 | 0.641242 | 0.559377 | 0.831665 | n/a |
| M190 raw SAE | 0.473581 | 0.337562 | 0.632040 | 0.553915 | 0.820250 | 0.560204 |
| M190 latent binary-BM25 | 0.467395 | 0.333997 | 0.633048 | 0.542682 | 0.822946 | 0.534671 |
| M190 fixed hybrid | 0.501901 | 0.358807 | 0.656244 | 0.577732 | 0.829037 | 0.539262 |

The evaluator calls CUB the relevant-document coverage of the final
top-1000 ranking. For hybrid retrieval it is not the pre-fusion
2,000-document semantic/lexical union, so it also reflects scorer
truncation.

## Per-Dataset Recall@100

| Dataset | BM25 | PPLX dense | P2.1 | M190 raw SAE | M190 latent | M190 fixed hybrid |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | 0.233236 | 0.269111 | 0.299101 | 0.304480 | 0.300856 | 0.309210 |
| scifact | 0.882556 | 0.898222 | 0.956000 | 0.957667 | 0.961000 | 0.976667 |
| arguana | 0.952891 | 0.972877 | 0.987866 | 0.993576 | 0.990721 | 0.994290 |
| fiqa | 0.510764 | 0.804102 | 0.662176 | 0.742807 | 0.738509 | 0.758549 |
| scidocs | 0.348567 | 0.484483 | 0.467100 | 0.446833 | 0.438783 | 0.457583 |
| trec-covid | 0.100074 | 0.157343 | 0.161646 | 0.138784 | 0.141246 | 0.151856 |
| webis-touche2020 | 0.561303 | 0.493851 | 0.517975 | 0.430421 | 0.448363 | 0.547143 |
| quora | 0.947661 | 0.984258 | 0.988816 | 0.992247 | 0.993061 | 0.995658 |
| cqadupstack | 0.524878 | 0.747247 | 0.730500 | 0.681549 | 0.684890 | 0.715242 |
| **Macro** | 0.562437 | 0.645722 | 0.641242 | 0.632040 | 0.633048 | 0.656244 |

## Per-Dataset NDCG@10

| Dataset | BM25 | PPLX dense | P2.1 | M190 raw SAE | M190 latent | M190 fixed hybrid |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | 0.306793 | 0.322832 | 0.347391 | 0.321756 | 0.310469 | 0.344179 |
| scifact | 0.663931 | 0.714060 | 0.723722 | 0.767013 | 0.751639 | 0.788953 |
| arguana | 0.344115 | 0.429182 | 0.411456 | 0.395946 | 0.394148 | 0.408751 |
| fiqa | 0.231073 | 0.510599 | 0.358516 | 0.430176 | 0.418763 | 0.441852 |
| scidocs | 0.150534 | 0.225314 | 0.200236 | 0.195328 | 0.194047 | 0.204478 |
| trec-covid | 0.571959 | 0.833468 | 0.752852 | 0.720287 | 0.701545 | 0.752536 |
| webis-touche2020 | 0.377718 | 0.276631 | 0.289785 | 0.216396 | 0.217902 | 0.303602 |
| quora | 0.738212 | 0.880790 | 0.844995 | 0.865859 | 0.867742 | 0.882656 |
| cqadupstack | 0.291987 | 0.431176 | 0.418136 | 0.349469 | 0.350298 | 0.390104 |
| **Macro** | 0.408480 | 0.513783 | 0.483010 | 0.473581 | 0.467395 | 0.501901 |

## Per-Dataset MAP@100

| Dataset | BM25 | PPLX dense | P2.1 | M190 raw SAE | M190 latent | M190 fixed hybrid |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | 0.137331 | 0.139780 | 0.169076 | 0.140717 | 0.136734 | 0.157616 |
| scifact | 0.626833 | 0.685142 | 0.679675 | 0.726629 | 0.707249 | 0.747588 |
| arguana | 0.237186 | 0.300000 | 0.286486 | 0.277471 | 0.272944 | 0.282370 |
| fiqa | 0.183892 | 0.451725 | 0.297809 | 0.374315 | 0.363484 | 0.378378 |
| scidocs | 0.103194 | 0.159333 | 0.142215 | 0.136642 | 0.134994 | 0.143535 |
| trec-covid | 0.066611 | 0.131466 | 0.131082 | 0.105223 | 0.107142 | 0.120174 |
| webis-touche2020 | 0.237831 | 0.166837 | 0.190253 | 0.131239 | 0.135747 | 0.197264 |
| quora | 0.695173 | 0.851136 | 0.807626 | 0.834031 | 0.835732 | 0.851105 |
| cqadupstack | 0.265961 | 0.389596 | 0.380379 | 0.311792 | 0.311946 | 0.351237 |
| **Macro** | 0.283779 | 0.363891 | 0.342733 | 0.337562 | 0.333997 | 0.358807 |

## Per-Dataset MRR@20

| Dataset | BM25 | PPLX dense | P2.1 | M190 raw SAE | M190 latent | M190 fixed hybrid |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | 0.515219 | 0.548789 | 0.567236 | 0.528902 | 0.506789 | 0.540291 |
| scifact | 0.634838 | 0.694454 | 0.688295 | 0.741248 | 0.721392 | 0.759633 |
| arguana | 0.233893 | 0.298787 | 0.284977 | 0.275451 | 0.270845 | 0.280597 |
| fiqa | 0.292597 | 0.599713 | 0.439020 | 0.514972 | 0.500607 | 0.518007 |
| scidocs | 0.277201 | 0.385142 | 0.348686 | 0.346620 | 0.345181 | 0.358198 |
| trec-covid | 0.817222 | 0.970000 | 0.930000 | 0.936667 | 0.903333 | 0.930667 |
| webis-touche2020 | 0.612666 | 0.517772 | 0.520760 | 0.437215 | 0.430151 | 0.548073 |
| quora | 0.735647 | 0.874570 | 0.838923 | 0.859232 | 0.860989 | 0.877385 |
| cqadupstack | 0.295992 | 0.425859 | 0.416496 | 0.344925 | 0.344854 | 0.386740 |
| **Macro** | 0.490586 | 0.590565 | 0.559377 | 0.553915 | 0.542682 | 0.577732 |

## Native Cost

| Dataset | Docs | Postings | Mean touches/query | p95 touches/query | maxDF ratio | Index bytes | Raw p50 | Raw p95 | Hybrid p50 | Hybrid p95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | 3633 | 232512 | 5402.2 | 13707.1 | 0.657859 | 18251776 | 2.344 ms | 3.827 ms | 10.156 ms | 12.291 ms |
| scifact | 5183 | 331712 | 12503.4 | 20262.3 | 0.470384 | 26370048 | 8.315 ms | 12.379 ms | 17.570 ms | 21.939 ms |
| arguana | 8674 | 555136 | 18356.5 | 25381.0 | 0.289371 | 43237376 | 5.049 ms | 6.207 ms | 15.021 ms | 16.590 ms |
| fiqa | 57638 | 3688832 | 113429.3 | 188731.9 | 0.285228 | 284803072 | 86.642 ms | 147.104 ms | 38.774 ms | 52.330 ms |
| scidocs | 25657 | 1642048 | 46165.0 | 67961.8 | 0.261254 | 127877120 | 34.200 ms | 48.527 ms | 22.007 ms | 25.749 ms |
| trec-covid | 171331 | 10965184 | 713731.6 | 865465.2 | 0.445810 | 847380480 | 640.162 ms | 778.550 ms | 168.376 ms | 187.969 ms |
| webis-touche2020 | 382545 | 24482880 | 807285.1 | 1107785.0 | 0.464641 | 1902870528 | 848.019 ms | 1180.429 ms | 463.852 ms | 1468.027 ms |
| quora | 522931 | 33467584 | 600622.2 | 976699.4 | 0.199151 | 2580135936 | 148.622 ms | 227.182 ms | 220.200 ms | 308.606 ms |
| cqadupstack | 457199 | 29260736 | 480594.3 | 689110.2 | 0.127531 | 2260557824 | 125.744 ms | 166.530 ms | 193.037 ms | 246.141 ms |

The frozen replay clips each document to at most 64 semantic
postings and each query to at most 80. maxDF is measured on each
complete corpus rather than inferred from row nnz. PostgreSQL
normalized storage includes row/index
overhead and is therefore not a compressed production byte estimate.

For query support `A_q`, exhaustive inverted traversal is
`T(q) = sum_{a in A_q} df(a)`. Bounding every document to 64 atoms
bounds index nnz but does not bound `T(q)` when a few selected atoms
have high corpus DF. maxDF and measured touches remain required
observability, but they cannot replace a dynamic-pruning engine gate.

## Exact BMP Engine Diagnostic

- decision: `retain_raw_m190_skip_representation_df_pruning`
- FiQA quantized Recall retention: `1.002562`
- exact score/strict-boundary parity: `1.000000` / `1.000000`
- BMP p50/p95: `5.647` / `7.203 ms`
- index bytes/document: `1200.1`
- BMP minus float Recall@100: `+0.001903`

M1962 reused unchanged raw M190 impacts and achieved exact BMP
parity. PostgreSQL posting-union latency is therefore a backend
diagnostic, not a reason to prune the representation. The fixed
M1961 query-DF policy is paused as a representation route.

## Findings

1. **The self-trained route is real but not promotable.** Raw M190 is not
   merely an old custom-evaluator artifact. It remains competitive through
   exact BMP across all 15 rows, but the full matrix rejects it as a fallback.
2. **Raw support carries the representation.** Replacing document
   magnitudes by IDF generally lowers raw ranking quality. IDF becomes
   useful as a calibration surface for fixed lexical hybridization.
3. **The fixed hybrid is an auditable calibration ceiling.** Relative
   to raw M190 it improves
   4/4 principal
   macro metrics without a dataset-specific weight, but it is not a
   one-index product result.
4. **Raw M190 does not displace P2.1 on this surface.** It wins
   `0/4` principal macro metrics against P2.1 and has
   `6` rows with at least
   one material (`<-0.01`) principal-metric loss. The fixed hybrid
   wins `4/4` macro
   metrics but remains a separate-source complementarity ceiling.
5. **Candidate coverage is the first quality bottleneck.** Across full15,
   the M190-minus-P2.1 CUB delta correlates with Recall delta at `0.848208`
   and MAP delta at `0.686959`. All four rows with MAP loss below `-0.03`
   also have lower CUB. This is descriptive, not causal, but it rules out
   treating another post-hoc scorer tweak as the default next step.
6. **The native engine is exact but not uniformly within latency bounds.**
   Quantization and boundary parity pass on every row. FEVER and HotpotQA
   exceed the frozen p95 gate, while MSMARCO passes at `18.420 ms`. Quality
   rejection has precedence, so engine tuning alone cannot rescue M190.

## Gates

- BM25 principal-metric wins: `4/4`.
- Recall retention vs dense: `1.0163`.
- MAP retention vs dense: `0.9860`.
- Raw M190 Recall retention vs dense: `0.9788`.
- Raw M190 MAP retention vs dense: `0.9276`.
- Raw M190 principal macro wins vs P2.1: `0/4`.
- Fixed hybrid principal macro wins vs P2.1: `4/4`.
- Raw/hybrid rows with material P2.1 harm: `6` / `3`.
- broad9 quality gate: `PASS`.
- Full15 complete: `15/15`.
- Full15 dense Recall retention: `0.959912` (`PASS`).
- Full15 dense MAP retention: `0.887020` (`FAIL`).
- Full15 principal macro wins vs P2.1: `2/4`.
- Full15 all-engine gate: `FAIL` (FEVER and HotpotQA p95).
- Self-trained raw-backup gate: `REJECT`.

## Next Step

Close historical M190 optimization. Preserve the checkpoint, recovered
surface, exact-BMP index archive, matrices, and provenance as training
evidence. Do not reopen scorer calibration, DF pruning, or checkpoint tuning
on this route. A future self-trained candidate must change the representation
or training evidence and re-enter through the same full15 native gate.

## Reproducibility

- Contract: `ii42-m1960-historical-self-trained-pplx-sae-replay-contract.md`.
- Canonical checkpoint archive:
  `/Volumes/Betty/II42/m1960-historical-selftrain-replay-v1/checkpoints/bm25sae_stageb_best.pt`
  (`SHA-256 49dfd1e8de9cbbe270c1f98793157101f1892cf3dd32d10033ead66d47e2360b`).
- Exporter: `scripts/export_m1960_m190_checkpoint_surface.py`.
- Native runner: `scripts/run_m1960_native_replay_local.sh`.
- Machine-readable matrix: `/Volumes/Betty/II42/m1960-historical-selftrain-replay-v1/m1960_broad9_matrix.json`.
- Exact-BMP full15 matrix:
  `/Volumes/Betty/II42/m1963-m190-bmp-full15-v1/m1963_full15_matrix.json`.
- Full15 report: `ii42-m1963-m190-bmp-full15-report.md`.
- MSMARCO source validation:
  `/Volumes/Betty/II42/m1960-historical-selftrain-replay-v1/provenance/msmarco_historical_embedding_validation.json`.
- MSMARCO BMP archive manifest:
  `/Volumes/Betty/II42/m1963-m190-bmp-full15-v1/index-archive/msmarco-b16-u32.bmp.zst.manifest.json`.
