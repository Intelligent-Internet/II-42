# SAE M140 Hybrid Complement Probe Report

Date: 2026-05-25

## Status

M140 is a small quick probe after the M130 milestone, not a promoted training
line. M130 remains preserved at commit `6e7a795` with tag
`m130-official-beir-milestone-20260525`.

The goal was to test whether a more integrated BM25+SAE scoring route has
signal before starting another expensive training branch. This probe does not
train atoms. It uses existing official full-corpus ranking JSONL artifacts and
learns only a runtime-safe linear scorer over BM25 and SAE evidence features.

Artifacts:

```text
scripts/research_sae_m140_hybrid_complement_probe.py
scripts/research_sae_m140_listwise_hybrid_ranker.py
results/sae/m140-hybrid-complement-probe/strict-heldout-v1
results/sae/m140-hybrid-complement-probe/all-test-v1
results/sae/m140-listwise-hybrid-ranker/strict-heldout-v3-scale003
results/sae/m140-listwise-hybrid-ranker/all-test-v3-scale003
```

The `results/` tree is intentionally not tracked by git; the summarized
numbers below are the tracked milestone record.

## Method

The probe builds one candidate pool per query from existing full-corpus
rankings:

- BM25 top results;
- SAE top results;
- fixed BM25+SAE score-fusion results;
- dense and BM25+dense rows as controls only.

It then trains a leave-one-dataset-out linear model with features that are safe
at runtime:

- normalized BM25 score;
- normalized SAE score;
- normalized fixed BM25+SAE fusion score;
- reciprocal ranks for BM25, SAE, and fixed fusion;
- BM25/SAE interaction and disagreement features;
- source indicators for BM25-only, SAE-only, and shared candidates.

The model is intentionally weak. The useful output is diagnostic:

- if the BM25+SAE candidate-pool oracle approaches dense, coverage is not the
  immediate blocker;
- if the learned scorer still cannot beat fixed fusion, the next work should be
  listwise/differentiable ranking and calibration, not another scalar fusion
  sweep.

## Strict-Heldout Result

Completed datasets at this probe point:

```text
arguana, nfcorpus, scidocs, scifact
```

| Source | R@20 | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.5017 | 0.5980 | 0.4068 | 0.3569 | 0.2683 |
| SAE | 0.5129 | 0.6304 | 0.3851 | 0.3461 | 0.2606 |
| BM25+SAE fixed fusion | 0.5602 | 0.6615 | 0.4578 | 0.4051 | 0.3075 |
| M140 learned scorer | 0.5629 | 0.6670 | 0.4523 | 0.4022 | 0.3043 |
| Dense | 0.5936 | 0.6931 | 0.4778 | 0.4266 | 0.3226 |
| BM25+dense fusion | 0.5924 | 0.6856 | 0.4790 | 0.4269 | 0.3221 |
| BM25+SAE oracle pool | 0.6915 | 0.6948 | 0.9337 | 0.8002 | 0.6948 |

Interpretation:

- The learned scorer improves Recall@100 over fixed BM25+SAE by `+0.0055`.
- It regresses top-rank quality versus fixed BM25+SAE:
  NDCG@10 `-0.0029`, MAP@100 `-0.0032`.
- The BM25+SAE candidate-pool oracle reaches Recall@100 `0.6948`, slightly
  above dense `0.6931` and BM25+dense fusion `0.6856` on these completed
  datasets.

This means the completed strict-heldout surface does not primarily lack
candidate coverage. The blocker is ranking/calibration inside the union.

## All-Test Result

Completed datasets at this probe point:

```text
arguana, nfcorpus, scidocs, scifact
```

| Source | R@20 | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.5100 | 0.6042 | 0.4143 | 0.3652 | 0.2749 |
| SAE | 0.5427 | 0.6490 | 0.4455 | 0.3920 | 0.3044 |
| BM25+SAE fixed fusion | 0.5821 | 0.6762 | 0.4982 | 0.4364 | 0.3348 |
| M140 learned scorer | 0.5838 | 0.6805 | 0.4938 | 0.4326 | 0.3303 |
| Dense | 0.5941 | 0.6955 | 0.4867 | 0.4327 | 0.3276 |
| BM25+dense fusion | 0.5970 | 0.6902 | 0.4895 | 0.4353 | 0.3295 |
| BM25+SAE oracle pool | 0.7042 | 0.7070 | 0.9443 | 0.8151 | 0.7070 |

Interpretation:

- Fixed BM25+SAE already beats BM25+dense on MRR@20, NDCG@10, and MAP@100 in
  this partial all-test surface, but remains behind on Recall@100.
- M140 learned scoring improves Recall@100 by `+0.0042` over fixed BM25+SAE,
  but again loses NDCG/MAP.
- The oracle pool is far above dense/fusion ranking metrics, so the candidate
  surface contains useful positives that the scorer cannot order correctly.

## Decision

M140 has enough signal to continue, but not as a linear scorer. The useful next
route is a differentiable hybrid training objective that learns BM25 and SAE
complementarity together:

1. keep the M130 official full-corpus queue running to finish the complete BEIR
   table before making a broad claim;
2. use the BM25+SAE oracle gap as the training target: reduce the gap between
   actual fixed/learned ranking and oracle pool ranking;
3. train a listwise or pairwise scorer against qrels and teacher neighborhoods,
   not pointwise relevance over a small union;
4. make BM25 suppression and SAE over-promotion explicit loss terms;
5. only after the scorer improves NDCG/MAP should atom allocation be retrained
   so semantic atoms are optimized for BM25 complementarity rather than dense
   imitation alone.

The negative conclusion is also clear: another scalar fusion or global
weight-sweep is unlikely to solve the current gap. The candidate union already
has enough headroom; the training objective needs to learn ordering inside that
union.

## M140.1 Listwise Residual Probe

After the linear scorer, M140 tested a small query-level listwise/pairwise
ranker. Two implementation checks were necessary before trusting the numbers:

- ClearML logging was fixed in `scripts/research_sae_aim.py` so SDK host and
  credential environment variables are explicitly set before `Task.init`.
- The residual scorer was forced to reproduce fixed BM25+SAE when
  `residual_scale=0`. Candidates absent from fixed fusion now receive anchor
  score `-1.0`; otherwise absent candidates tied with the lowest fixed-fusion
  score and corrupted the sanity check.

The promoted probe setting is intentionally conservative:

```text
score_mode=residual
residual_scale=0.03
epochs=80
hidden_dim=16
loss=listwise + pairwise
training=leave-one-dataset-out
```

ClearML tasks:

```text
strict-heldout: bm25sae-m140-listwise-strict-heldout-v3-scale003
all-test: bm25sae-m140-listwise-all-test-v3-scale003
```

Strict-heldout result:

| Source | R@20 | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25+SAE fixed fusion | 0.5602 | 0.6615 | 0.4578 | 0.4051 | 0.3075 |
| M140 listwise residual | 0.5613 | 0.6615 | 0.4582 | 0.4056 | 0.3080 |
| BM25+dense fusion | 0.5924 | 0.6856 | 0.4790 | 0.4269 | 0.3221 |
| Oracle pool | 0.6915 | 0.6948 | 0.9337 | 0.8002 | 0.6948 |

All-test result:

| Source | R@20 | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25+SAE fixed fusion | 0.5821 | 0.6762 | 0.4982 | 0.4364 | 0.3348 |
| M140 listwise residual | 0.5823 | 0.6762 | 0.4984 | 0.4366 | 0.3359 |
| BM25+dense fusion | 0.5970 | 0.6902 | 0.4895 | 0.4353 | 0.3295 |
| Oracle pool | 0.7042 | 0.7070 | 0.9443 | 0.8151 | 0.7070 |

Interpretation:

- The listwise residual scorer is a small but consistent positive signal over
  fixed BM25+SAE on both available splits.
- It does not improve Recall@100 because `residual_scale=0.03` keeps non-fixed
  candidates below the fixed-fusion top-100. This was deliberate: the first
  reliable step should not trade away the strong head ranking.
- The large oracle gap remains. The next useful M140 step is a two-band scorer:
  preserve a conservative residual for the fixed-fusion head while separately
  learning a controlled admission gate for SAE/BM25 candidates outside the
  fixed top-100.

## M140.2 Stage-D Two-Band Admission Probe

Stage-D keeps the M130 BM25+SAE model unchanged and adds only a quick
query-level admission scorer. This is intentionally a downstream calibration
test: it asks whether bad rows in the completed official BEIR matrix can be
improved by letting selected non-fixed BM25/SAE candidates enter the final
top-100.

Implementation:

- `score_mode=two-band` in
  `scripts/research_sae_m140_listwise_hybrid_ranker.py`;
- fixed-fusion candidates use the conservative residual head
  `anchor + 0.03 * tanh(raw_score)`;
- non-fixed candidates use an admission lane
  `admission_base + admission_scale * tanh(raw_score)`;
- training remains leave-one-dataset-out and ClearML-tracked.

Two settings were tested:

```text
conservative: admission_base=-0.03, admission_scale=0.12, epochs=100
aggressive:   admission_base=0.00,  admission_scale=0.30, epochs=80
```

ClearML tasks:

```text
strict conservative: bm25sae-m140-stage-d-strict-twoband-v0
strict aggressive:   bm25sae-m140-stage-d-strict-twoband-aggressive-v0
all-test aggressive: bm25sae-m140-stage-d-all-test-twoband-aggressive-v0
```

Strict-heldout aggressive result:

| Source | R@20 | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25+SAE fixed fusion | 0.5602 | 0.6615 | 0.4578 | 0.4051 | 0.3075 |
| M140 Stage-D two-band | 0.5608 | 0.6645 | 0.4604 | 0.4076 | 0.3105 |
| BM25+dense fusion | 0.5924 | 0.6856 | 0.4790 | 0.4269 | 0.3221 |

All-test aggressive result:

| Source | R@20 | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25+SAE fixed fusion | 0.5821 | 0.6762 | 0.4982 | 0.4364 | 0.3348 |
| M140 Stage-D two-band | 0.5833 | 0.6787 | 0.4997 | 0.4383 | 0.3370 |
| BM25+dense fusion | 0.5970 | 0.6902 | 0.4895 | 0.4353 | 0.3295 |

Weak-row deltas:

| Split | Dataset | R@100 fixed | R@100 Stage-D | R@100 dense | NDCG fixed | NDCG Stage-D | NDCG dense |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| strict | `nfcorpus` | 0.2843 | 0.2871 | 0.3190 | 0.3138 | 0.3149 | 0.3562 |
| strict | `scidocs` | 0.4220 | 0.4220 | 0.4705 | 0.1790 | 0.1789 | 0.2071 |
| strict | `arguana` | 0.9955 | 0.9955 | 0.9992 | 0.4023 | 0.4051 | 0.4300 |
| all-test | `nfcorpus` | 0.3273 | 0.3273 | 0.3284 | 0.3723 | 0.3718 | 0.3683 |
| all-test | `scidocs` | 0.4220 | 0.4220 | 0.4705 | 0.1790 | 0.1798 | 0.2071 |
| all-test | `arguana` | 0.9957 | 0.9957 | 0.9993 | 0.4098 | 0.4127 | 0.4281 |

Interpretation:

- Stage-D is positive overall and improves the fixed BM25+SAE matrix without
  changing the underlying SAE model.
- It helps `nfcorpus` slightly on strict-heldout and improves top-rank quality
  for `arguana` and `scifact`.
- It does not fix `scidocs` Recall@100. Even aggressive admission cannot
  promote the missing `scidocs` positives into top-100, so that row is not a
  downstream rerank-only problem. The likely blocker is candidate generation or
  atom allocation for scientific-document semantic neighborhoods.
- Therefore M140 Stage-D is useful as a final calibration layer, but it is not
  sufficient to close the remaining BM25+dense gap. The next substantive work
  should return to the M130 candidate/atom surface for `scidocs`-like misses,
  not keep widening the admission gate.
