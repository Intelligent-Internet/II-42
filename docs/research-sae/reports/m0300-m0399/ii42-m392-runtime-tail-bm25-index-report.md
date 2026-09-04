# II-42 M392 Runtime Tail-Sketch + BM25 Unified Posting Report

## Scope

M392 converts the dense-only joint-PCA tail-sketch result into an
index-shaped runtime prototype, then tests whether adding BM25 lexical postings
can approach a BM25+dense result on the local BEIR15 artifact face.

The evaluation is still qrels-free during scoring:

- no dataset id policy;
- no per-dataset hyperparameter selection;
- no qrels in projection, candidate generation, or scoring;
- qrels are used only for final BEIR15 metric reporting.

The local BEIR15 face is the existing sampled artifact root:
`/Volumes/Betty/Tmp/psql_bm25s_sae_beir15_shared`.

## Artifacts

| Run | Output | Purpose |
| --- | --- | --- |
| M392 smoke | `/tmp/ii42-m392-smoke-fiqa-v2/m392_smoke_fiqa_v2.json` | Single-dataset runtime/BM25 smoke |
| M392 parity | `/tmp/ii42-m392-runtime-parity-seed379-beir15/m392_runtime_parity_seed379.json` | Runtime parity against M390 int8 |
| M392 BM25 sweep | `/tmp/ii42-m392-runtime-tail-bm25-zblend-seed379-beir15/m392_runtime_tail_bm25_zblend_seed379.json` | Full BEIR15 BM25 fusion sweep |
| M392 5% budget | `/tmp/ii42-m392-runtime-tail-bm25-budget005-seed379-beir15/m392_runtime_tail_bm25_budget005_seed379.json` | Low-touch dense+BM25 budget |
| M392 8% budget | `/tmp/ii42-m392-runtime-tail-bm25-budget008-seed379-beir15/m392_runtime_tail_bm25_budget008_seed379.json` | Main low-touch target |
| M392 seed 1379 | `/tmp/ii42-m392-runtime-tail-bm25-budget008-seed1379-beir15/m392_runtime_tail_bm25_budget008_seed1379.json` | Query-heldout robustness |
| M392 seed 2379 | `/tmp/ii42-m392-runtime-tail-bm25-budget008-seed2379-beir15/m392_runtime_tail_bm25_budget008_seed2379.json` | Query-heldout robustness |

## Runtime Parity

M392 reproduces M390 int8 exactly on the same seed 379 query-heldout split.
This validates that the result is not an artifact of the matrix-only harness.

| Source | M390 NDCG@10 | M392 NDCG@10 | M392 R@100 | Touch |
| --- | ---: | ---: | ---: | ---: |
| dense exact | 0.7748 | 0.7748 | 0.8545 | 1.0000 |
| sparse runtime | 0.7690 | 0.7690 | 0.8484 | 0.1000 |
| fullq tail upper | 0.7752 | 0.7752 | 0.8544 | 0.1000 |
| int8 tail sketch 256 | 0.7751 | 0.7751 | 0.8551 | 0.1000 |

## BM25 Fusion Result

The first BM25 attempt used Reciprocal Rank Fusion. That failed as a global
policy: BM25 is weaker than dense on most local BEIR15 faces, so RRF gives too
much rank authority to a noisy lexical signal.

The successful policy is fixed low-weight z-score blending:

`score = 0.90 * z(dense_tail_score) + 0.10 * z(bm25_score)`

For the unified-posting variant, the candidate set is the union of dense
coordinate posting candidates and BM25 posting candidates. The final score uses
the dense-tail runtime score plus the BM25 score with the same fixed blend.

### Seed 379, 10% Dense Budget + 10% BM25 Budget

| Source | R@100 | MRR@20 | NDCG@10 | MAP@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: |
| dense exact | 0.8545 | 0.8677 | 0.7748 | 0.6931 | 1.0000 |
| runtime tail 256 int8 | 0.8551 | 0.8680 | 0.7751 | 0.6934 | 0.1000 |
| BM25 only | 0.7802 | 0.7677 | 0.6570 | 0.5810 | 0.1000 |
| BM25+dense exact RRF | 0.8500 | 0.8272 | 0.7416 | 0.6625 | 1.0000 |
| BM25+tail union RRF | 0.8468 | 0.8287 | 0.7421 | 0.6628 | 0.1667 |
| BM25+dense exact zblend alpha=0.10 | 0.8572 | 0.8620 | 0.7773 | 0.7004 | 1.0000 |
| BM25+tail union zblend alpha=0.10 | 0.8562 | 0.8648 | 0.7774 | 0.6984 | 0.1667 |

Interpretation: low-weight BM25 zblend improves over dense and tail-only; RRF
is not viable for this artifact face.

## Low-Touch Budget Sweep

The strongest setting is not necessarily the 10% dense budget. With BM25
candidate union, 5-8% dense posting touch already preserves the gain.

| Dense budget | BM25 budget | Tail256 NDCG@10 | BM25+tail zblend alpha=0.10 | R@100 | Total touch |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 0.05 | 0.05 | 0.7755 | 0.7771 | 0.8544 | 0.0838 |
| 0.08 | 0.08 | 0.7755 | 0.7773 | 0.8557 | 0.1338 |
| 0.10 | 0.10 | 0.7751 | 0.7774 | 0.8562 | 0.1667 |

The 5%+5% variant is the best efficiency point. The 8%+8% variant is the safer
quality point. The 10%+10% variant gives little additional NDCG and increases
touch.

## Query-Heldout Robustness

The 8%+8% setting was repeated across the same heldout seeds used in earlier
dense-tail robustness checks.

| Seed | Dense exact | Tail256 | BM25+tail zblend alpha=0.10 | BM25+dense exact zblend alpha=0.10 | Unified gap to full dense+BM25 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 379 | 0.7748 | 0.7755 | 0.7773 | 0.7773 | +0.0000 |
| 1379 | 0.7812 | 0.7815 | 0.7858 | 0.7875 | -0.0016 |
| 2379 | 0.7613 | 0.7608 | 0.7658 | 0.7662 | -0.0004 |
| mean | 0.7724 | 0.7726 | 0.7763 | 0.7770 | -0.0007 |

The unified-posting variant captures nearly all of the fixed full
dense+BM25 zblend gain while touching about 13-14% of documents instead of
the full corpus.

## Per-Dataset Shape

BM25 is helpful on lexical-friendly datasets such as `webis-touche2020` and
`scifact`, but it is weak on several dense-friendly datasets. This is why RRF
fails and low-weight zblend works: BM25 should be a small correction and
candidate supplement, not an equal ranking authority.

## Current Best Configuration

Promote this as the current candidate:

- rotation: `pca_doc`;
- active dims: `128`;
- dense-tail projection: query-heldout `joint_pca`;
- sketch dims: `256`;
- doc sketch quantization: `int8`;
- dense coordinate candidate budget: `0.08`;
- BM25 candidate budget: `0.08`;
- final score: `0.90 * z(dense_tail_score) + 0.10 * z(bm25_score)`;
- candidate set: dense-coordinate posting union BM25 lexical posting;
- expected local BEIR15 macro: `NDCG@10 = 0.7773`, `R@100 = 0.8557`,
  `MAP@100 = 0.6983`, touch about `0.1338`.

## Verdict

The dense-tail route is not at a dead end. It is now an index-shaped
near-dense posting system. Adding BM25 does help, but only under a conservative
global blend. A naive equal-rank BM25 fusion is worse than dense-only.

The next product-facing step is to lock alpha=0.10 as a global default and
validate it on an external or later-heldout corpus before treating it as a
universal setting.
