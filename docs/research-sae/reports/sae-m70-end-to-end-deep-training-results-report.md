# SAE M70 End-To-End Deep Training Results Report

Date: 2026-05-21

Status: M70 implementation started. The first commit is a reproducibility
smoke, not a product-quality claim.

## Summary

M70 now has a runnable end-to-end training path that starts from random model
weights and directly trains the deployed retrieval shape:

```text
query text + document text
  -> asymmetric query/doc atom encoder
  -> hard sparse atom path
  -> dynamic BM25/SAE scales
  -> final BM25 + SAE ranking loss
```

This is materially different from the M60 line. It does not initialize from a
previous text-to-atoms checkpoint, and it does not train only a scalar
calibration head. The smoke result proves the new objective is executable and
reproducible before scaling to Spark.

## Implemented Pieces

| Piece | File | Status |
| --- | --- | --- |
| M70 source registry v2 | `scripts/research_sae_m70_source_registry.py` | implemented |
| End-to-end deep trainer | `scripts/research_sae_m70_e2e_deep_train.py` | implemented |
| Local reproducibility smoke | `results/sae/m70/e2e-deep-smoke` | completed |
| Source/leakage registry output | `results/sae/m70/source-registry` | completed |

## Source Registry Snapshot

The first registry pass found the currently available local training and
evaluation surface:

| Source | Role | Datasets | Docs | Queries | Qrel pairs | Notes |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `beir15_current_eval` | eval | 15 | 49059 | 1342 | 39742 | full15 regression |
| `m32_official_train_dev` | train | 8 | 128816 | 2167 | 18957 | official train/dev |
| `m39_broad_plus_nfcorpus` | train | 9 | 146353 | 4981 | 146212 | broad generated source |
| `m60_official_msmarco_train` | train | 1 | 100000 | 2000 | 2128 | official MS MARCO slim source |
| `m20_real_corpus_efficiency` | representation | 4 | 30000 | 1200 | 0 | quality claims disabled |

Leakage gate:

```text
train_query_keys = 6681
eval_query_keys = 1342
overlap_count = 0
status = passed
```

Planned M70 expansion remains: Wikipedia passages, larger arXiv/PubMed/Commons
representation text, TREC DL, NQ retrieval, and LoTTE/domain holdouts.

## Reproducibility Smoke

Command:

```bash
PYTHONPATH=scripts python3 scripts/research_sae_m70_e2e_deep_train.py \
  --datasets scifact nfcorpus \
  --output-dir results/sae/m70/e2e-deep-smoke \
  --max-docs-per-dataset 1200 \
  --max-train-queries-per-dataset 60 \
  --max-eval-queries-per-dataset 40 \
  --latent-dims 256 \
  --query-active-dims 12 \
  --doc-active-dims 24 \
  --token-dim 96 \
  --hidden-dim 192 \
  --epochs 2 \
  --batch-size 8 \
  --candidate-k 60 \
  --max-candidates 72 \
  --device auto
```

Scope:

| Item | Value |
| --- | ---: |
| Datasets | 2 |
| Documents | 2400 |
| Train queries | 112 |
| Eval queries | 43 |
| Vocab size | 24526 |
| Device | `mps` |
| Elapsed | 9.44 seconds |

Metrics:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.5258 | 0.6051 | 0.4705 | 0.3531 |
| `student` | 0.5457 | 0.6012 | 0.4688 | 0.3535 |

Training loss:

| Epoch | Loss | Listwise | Pairwise | BM25 complement | Cost | BM25 scale | SAE scale |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 4.1820 | 3.7725 | 0.7939 | 0.0414 | 0.0315 | 1.0834 | 0.2374 |
| 2 | 3.8299 | 3.4439 | 0.7579 | 0.0228 | 0.0305 | 1.0214 | 0.1360 |

Interpretation:

- The result is not strong enough to promote anything.
- It is enough to prove the new M70 operator is trainable from scratch and
  reproducible.
- Recall improved over BM25 on the smoke split, while MRR/NDCG stayed flat.
  That is acceptable for a first smoke because the objective is now wired end
  to end and can be scaled.

## Spark M39 Broad From-Zero Run

After the local smoke, the same trainer was synced to Spark and run inside the
NVIDIA PyTorch container:

```text
host = huoju@100.123.2.95
tmux = m70_e2e_m39
image = leask/ii42-sae:pytorch26.03-aim
output = /home/huoju/leask/runs/m70-e2e-m39-broad-fromzero-e8
log = /home/huoju/leask/logs/m70-e2e-m39-broad-fromzero-e8.log
```

Configuration:

| Item | Value |
| --- | ---: |
| Datasets | 9 |
| Documents | 64816 |
| Train queries | 1453 |
| Eval queries | 594 |
| Latent dims | 1024 |
| Query active dims | 24 |
| Doc active dims | 48 |
| Epochs | 8 |
| Device | `cuda` |
| Elapsed | 52.39 seconds |

Metrics:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.6484 | 0.6600 | 0.5654 | 0.4751 |
| `student` | 0.6477 | 0.6607 | 0.5653 | 0.4751 |

Training loss:

| Epoch | Loss | Listwise | Pairwise | BM25 complement | BM25 scale | SAE scale |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 4.9239 | 4.4123 | 0.8415 | 0.0329 | 0.8920 | 0.1970 |
| 2 | 4.7083 | 4.2116 | 0.8204 | 0.0216 | 0.9749 | 0.1248 |
| 3 | 4.5355 | 4.0523 | 0.7983 | 0.0208 | 1.0307 | 0.1225 |
| 4 | 4.3931 | 3.9216 | 0.7794 | 0.0188 | 1.0629 | 0.1112 |
| 5 | 4.3468 | 3.8806 | 0.7713 | 0.0169 | 1.1008 | 0.1014 |
| 6 | 4.3198 | 3.8563 | 0.7670 | 0.0164 | 1.1281 | 0.0986 |
| 7 | 4.1654 | 3.7137 | 0.7475 | 0.0157 | 1.0961 | 0.0942 |
| 8 | 4.1347 | 3.6874 | 0.7407 | 0.0141 | 1.1326 | 0.0837 |

Interpretation:

- The scaled run is stable and reproducible, but not yet useful as a promoted
  model.
- Loss decreases cleanly, but the learned model converges to a conservative
  BM25-dominant regime. SAE scale falls from `0.1970` to `0.0837`.
- This confirms the trainer can scale, but the first objective is too cautious:
  complementarity/cost pressure prevents SAE from learning enough semantic
  residual signal.
- The next M70 iteration should strengthen positive semantic coverage before
  adding more epochs: add teacher/dense near-miss candidates, qrel-positive
  coverage loss, and separate representation pretraining before final ranking.

## Coverage Candidate Smoke V2

The next local smoke kept the M70 end-to-end design but changed three training
details:

- candidate pool includes qrel positives, BM25, dense-teacher candidates, and
  SAE-teacher candidates;
- qrel positives are never truncated by `max_candidates`; the batch collator now
  pads variable-size candidate lists;
- semantic coverage loss acts directly on SAE scores, so the model cannot pass
  the loss only by increasing BM25 scale.

Scope:

| Item | Value |
| --- | ---: |
| Datasets | 2 |
| Documents | 2400 |
| Train queries | 112 |
| Eval queries | 43 |
| Candidate mean | 97.45 |
| Qrel positives in pool | 1458 / 1458 |

Metrics:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.5258 | 0.6051 | 0.4705 | 0.3531 |
| `student` | 0.5404 | 0.6040 | 0.4733 | 0.3542 |

Training signal:

| Epoch | Loss | Coverage | BM25 scale | SAE scale |
| ---: | ---: | ---: | ---: | ---: |
| 1 | 5.7610 | 1.1648 | 1.0766 | 0.3139 |
| 2 | 5.3967 | 1.1573 | 1.0445 | 0.2311 |
| 3 | 5.2241 | 1.1561 | 0.9232 | 0.2077 |
| 4 | 4.9810 | 1.1555 | 1.1089 | 0.2809 |

Interpretation:

- This is the first M70 smoke where semantic coverage improves Recall@100,
  NDCG@10, and MAP@100 over BM25 while keeping SAE scale active.
- MRR@20 is slightly lower, so this is not a promotion result.
- The result is good enough to scale the same objective to Spark and check
  whether the signal survives broader data.

## Current Engineering Meaning

## Soft Teacher Semantic Target Follow-Up

The coverage path was extended again so dense and SAE teacher near-miss
candidates are not only inserted into the candidate pool. Their scores are now
preserved as a soft semantic target for the SAE branch, while qrel positives
remain the hard ranking target. The useful local setting was intentionally
small:

```text
semantic_teacher_weight = 0.10
dense_soft_label_weight = 0.35
sae_soft_label_weight = 0.45
```

Local smoke result:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.5258 | 0.6051 | 0.4705 | 0.3531 |
| `student` | 0.5442 | 0.6040 | 0.4717 | 0.3542 |

Higher teacher weights (`0.20`, `0.35`, `0.60`) were worse on the same smoke.
This means teacher-neighborhood information is useful as light semantic
regularization, but it cannot dominate qrel-positive coverage.

## Spark Soft Teacher Run

The best local soft-teacher setting was scaled on Spark:

```text
host = huoju@100.123.2.95
tmux = m70_soft_w010_e18
image = leask/ii42-sae:pytorch26.03-aim
output = /home/huoju/leask/runs/m70-e2e-m39-soft-teacher-w010-e18
log = /home/huoju/leask/logs/m70-e2e-m39-soft-teacher-w010-e18.log
```

Configuration:

| Item | Value |
| --- | ---: |
| Datasets | 9 |
| Documents | 120816 |
| Train queries | 2007 |
| Eval queries | 802 |
| Latent dims | 2048 |
| Query active dims | 40 |
| Doc active dims | 80 |
| Epochs | 18 |
| Candidate mean | 257.70 |
| Qrel positives in pool | 43596 / 43596 |
| Device | `cuda` |
| Elapsed | 331.90 seconds |

Metrics:

| Source | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `bm25` | 0.6602 | 0.6734 | 0.5858 | 0.4912 |
| `student` | 0.6596 | 0.6740 | 0.5860 | 0.4915 |

Training signal:

| Epoch | Loss | Coverage | Teacher | BM25 scale | SAE scale |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 7.1974 | 1.1655 | 5.5494 | 1.0260 | 0.2403 |
| 6 | 6.5400 | 1.1646 | 5.5491 | 1.1599 | 0.1505 |
| 12 | 6.3091 | 1.1638 | 5.5493 | 1.1847 | 0.1145 |
| 18 | 6.2606 | 1.1639 | 5.5489 | 1.1886 | 0.1042 |

Interpretation:

- The run is stable and keeps all qrel positives in the pool.
- MRR/NDCG/MAP improve slightly over BM25, but Recall@100 is slightly lower.
- SAE scale still decays during ranking training. The objective is not broken,
  but it remains too conservative to become the strong semantic residual branch
  needed for dense-removal readiness.
- A staged semantic-pretrain smoke was also tested (`3` semantic epochs + `4`
  ranking epochs). It reached Recall@100 `0.5421`, below the single-stage
  soft-teacher smoke `0.5442`, and semantic loss barely moved. Simple staged
  pretraining is therefore not promoted.
- Stronger qrel coverage sweeps (`1.75`, `2.25`) increased SAE scale but
  reduced smoke quality. This suggests the blocker is not just loss weight; it
  is the semantic target/candidate-label structure.

## Current Engineering Meaning

The M70 base is executable and reproducible, but not ready for promotion. The
immediate blocker is no longer "can we express the objective?" but "how do we
build a semantic target that makes SAE learn a useful residual instead of
becoming a low-scale auxiliary signal?"

Next run should not be another single-stage weight sweep. It should use the
source registry to build a larger Spark train/eval artifact and then run:

```text
M70B: broad representation pretraining
M70C: supervised final BM25+SAE ranking training
M70D: hard-negative refresh
```

Promotion remains blocked until M70 beats the M60/BM25 baselines under the
full quality and physical-cost gates.
