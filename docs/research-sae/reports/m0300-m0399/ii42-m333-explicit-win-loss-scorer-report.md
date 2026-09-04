# ii42 M333 Explicit Winner/Loser Admission Scorer Report

## Summary

M333 is a low-cost follow-up to M332. It keeps the same frozen posting
encoder and the same clean natural candidate surface, then changes only the
training objective.

M332 showed that the natural candidate union can expose enough relevant
documents, but the scorer does not reliably convert that candidate headroom
into top-rank quality. M333 therefore adds an explicit winner/loser admission
loss:

- pull currently missed qrel positives upward;
- demote BM25-supported false positives;
- demote atom-only false positives;
- demote dense-teacher false positives;
- keep dense teacher as evidence, but do not treat non-qrel dense neighbors as
  positives.

This is a scorer-only canary. It does not retrain the encoder, rebuild atoms,
or claim a new mainline unless the heldout result improves ranking quality.

## Files

- `scripts/research_sae_m322_candidate_pool_scorer.py`
- `scripts/run_m333_explicit_win_loss_scorer_spark.sh`
- `docs/research-sae/reports/m0300-m0399/ii42-m333-explicit-win-loss-scorer-report.md`

## Canary Contract

Common surface:

- Datasets: `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`.
- Checkpoint:
  `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-admission-prior005/m320_nfcorpus_scifact_seed1050_admission_prior005.pt`
- Teacher rankings:
  `/home/huoju/leask/runs/ii42-m326-baselines/dense_teacher_seed1050_expansion5_top300.json`
- Candidate K: `160`.
- Max atom DF ratio: `0.25`.
- Unified candidate scales: `0.25`, `0.5`.
- Teacher candidate injection: disabled.
- Doc/query active K: `96` / `80`.

Primary run:

`/home/huoju/leask/runs/ii42-m333-explicit-win-loss-scorer-v1/m333_explicit_win_loss_seed1050.json`

Initial explicit-admission knobs:

- explicit admission weight: `0.25`;
- explicit admission margin: `0.04`;
- positive cutoff: `20`;
- hard negatives per false-positive family: `12`.

## Stop Rule

Do not expand this line unless the 5-dataset canary shows a stable ranking
gain over M332 v4:

- NDCG@10 or MAP@100 improves by at least `0.005`, or
- MRR@20 improves without hurting NDCG/MAP, and
- Recall@100 does not regress materially.

If the result is flat, M333 closes the small scorer-tweak path and the next
work should move back to representation or richer interaction features.

## Status

Closed as a non-promotion canary.

## Results

Reference rows from M331/M332:

| Model | Macro R@100 | Macro MRR@20 | Macro NDCG@10 | Macro MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M331 adaptive | 0.7268 | 0.3649 | 0.3352 | 0.2428 |
| M332 v4 clean natural strong-rank | 0.7299 | 0.3656 | 0.3347 | 0.2426 |

M333 runs:

| Run | Explicit weight | Macro R@100 | Macro MRR@20 | Macro NDCG@10 | Macro MAP@100 | Upper-bound R |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| v1 | 0.25 | 0.7274 | 0.3642 | 0.3346 | 0.2418 | 0.7758 |
| v2 | 0.10 | 0.7301 | 0.3649 | 0.3350 | 0.2425 | 0.7758 |

v2 is the better M333 run, but the gain is too small to promote:

- vs M332 v4: R@100 `+0.0002`, MRR@20 `-0.0007`,
  NDCG@10 `+0.0003`, MAP@100 `-0.0001`;
- vs M331: R@100 `+0.0033`, MRR@20 `+0.0000`,
  NDCG@10 `-0.0002`, MAP@100 `-0.0003`.

Per-dataset v2:

| Dataset | R@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| `arguana` | 0.9903 | 0.2255 | 0.3457 | 0.2273 |
| `fiqa` | 0.7385 | 0.4856 | 0.3939 | 0.3324 |
| `nfcorpus` | 0.3043 | 0.5810 | 0.3518 | 0.1632 |
| `scidocs` | 0.4296 | 0.3366 | 0.1810 | 0.1224 |
| `scifact` | 0.9569 | 0.6175 | 0.6532 | 0.6045 |

Training curves also show the same shape in v1 and v2: epoch 10 is best, and
later epochs regress. This means the explicit winner/loser objective is not
unlocking new ranking capacity; it is mostly another weak regularizer over the
same shallow scorer feature surface.

## Decision

Do not expand M333 to full15 or larger sweeps.

The experiment answered the intended question: explicit winner/loser admission
pairs are not enough, by themselves, to close the dense top-rank gap. The
candidate upper bound remains high, but the scorer still lacks enough semantic
interaction signal to decide top-rank order reliably.

Next work should avoid more small scorer-loss tweaks. The more promising
directions are:

- richer query-document interaction features over the same natural candidate
  union;
- representation/posting training that makes the atom surface itself encode
  the final ranking distinction;
- a compact second-stage scorer trained on stronger features, if product
  latency allows it.
