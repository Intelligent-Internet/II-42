# ii42 M332 Full-Corpus Natural Admission/Ranking Report

## Summary

M332 tested a clean version of the idea:

> full-corpus natural candidate union + final admission/ranking objective

The experiment kept the M320 posting encoder fixed and changed only the scorer
candidate surface and final admission/ranking objective. The goal was to test
whether a more natural union of BM25, atom/SAE, and BM25+atom candidates can
turn the high candidate upper bound into better final top-k quality.

Result: M332 improves admission/recall slightly, but it does not produce a
clear ranking-quality breakthrough over M331. The best run is M332 v4:

| Model | Macro R@100 | Macro MRR@20 | Macro NDCG@10 | Macro MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M331 adaptive | 0.7268 | 0.3649 | 0.3352 | 0.2428 |
| M332 v4 clean natural strong-rank | 0.7299 | 0.3656 | 0.3347 | 0.2426 |
| Delta | +0.0031 | +0.0006 | -0.0004 | -0.0003 |

M332 should not replace M331 as a universal new mainline. It is useful evidence
that recall/admission can be improved by the natural candidate surface, but the
remaining blocker is still top-rank score calibration.

## Implementation

Files:

- `scripts/research_sae_m322_candidate_pool_scorer.py`
- `scripts/run_m332_full_corpus_natural_scorer_spark.sh`
- `docs/research-sae/reports/m0300-m0399/ii42-m332-full-corpus-natural-admission-ranking-plan.md`

New candidate-source contract:

- `1`: BM25 candidate evidence.
- `2`: atom/SAE candidate evidence.
- `4`: dense-teacher evidence, used only when explicitly injected.
- `0`: forced training-only candidate.

The candidate cache key now includes:

- unified candidate scales;
- teacher candidate split;
- teacher candidate K.

This prevents M332 from silently reusing M326/M330/M331 candidate caches with
different semantics.

The runner now installs missing `transformers` and `safetensors` in the
container before launching, matching the existing M322/M320 runner pattern.

## Runs

Common surface:

- Datasets: `nfcorpus`, `scifact`, `fiqa`, `arguana`, `scidocs`.
- Checkpoint:
  `/home/huoju/leask/runs/ii42-m320-dense-assisted-posting-v1-seed1050-admission-prior005/m320_nfcorpus_scifact_seed1050_admission_prior005.pt`
- Teacher rankings:
  `/home/huoju/leask/runs/ii42-m326-baselines/dense_teacher_seed1050_expansion5_top300.json`
- Candidate K: `160`.
- Max atom DF ratio: `0.25`.
- Doc/query active K: `96` / `80`.

### v1

Path:

`/home/huoju/leask/runs/ii42-m332-natural-union-final-objective-v1/m332_natural_union_seed1050.json`

Configuration:

- Unified candidate scales: `0.5`, `1.0`.
- Train-only teacher candidates: `120`.
- Hidden dim: `64`.
- Teacher weight: `0.15`.
- Teacher false-positive weight: `0.30`.
- Boundary weight: `0.35`.

Outcome:

- Candidate upper bound improved, but final scorer ranking regressed.
- The train/heldout distribution mismatch from train-only teacher candidates
  was not helpful.

### v2

Path:

`/home/huoju/leask/runs/ii42-m332-natural-union-final-objective-v2/m332_natural_union_v2_seed1050.json`

Configuration:

- Unified candidate scales: `0.25`, `0.5`.
- Teacher candidate injection disabled.
- Hidden dim: `64`.
- Teacher weight: `0.15`.
- Teacher false-positive weight: `0.30`.
- Boundary weight: `0.35`.

Outcome:

- Cleaner and healthier than v1.
- Still below M331 on ranking quality.

### v3

Path:

`/home/huoju/leask/runs/ii42-m332-natural-union-final-objective-v3/m332_natural_union_v3_seed1050.json`

Configuration:

- Same clean candidate surface as v2.
- Hidden dim: `128`.
- Teacher weight: `0.15`.
- Teacher false-positive weight: `0.50`.
- Boundary weight: `0.50`.

Outcome:

- Best recall in the v1-v3 series.
- Ranking still slightly below M331.

### v4

Path:

`/home/huoju/leask/runs/ii42-m332-natural-union-final-objective-v4/m332_natural_union_v4_seed1050.json`

Configuration:

- Same clean candidate surface as v2/v3.
- Hidden dim: `128`.
- Teacher weight: `0.10`.
- Teacher false-positive weight: `0.50`.
- Boundary weight: `0.50`.

Outcome:

- Best balanced M332 result.
- Improves macro recall and MRR over M331.
- Does not beat M331 on NDCG/MAP.

## Result Matrix

| Model | Macro R@100 | Macro MRR@20 | Macro NDCG@10 | Macro MAP@100 | Train R | Train MRR | Train NDCG | Train MAP | Upper-bound R |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| M326B expansion | 0.7242 | 0.3658 | 0.3339 | 0.2420 | 0.7449 | 0.3568 | 0.3369 | 0.2431 | 0.7685 |
| M330 exact | 0.7276 | 0.3660 | 0.3347 | 0.2425 | 0.7478 | 0.3569 | 0.3372 | 0.2435 | 0.7685 |
| M331 adaptive | 0.7268 | 0.3649 | 0.3352 | 0.2428 | 0.7472 | 0.3563 | 0.3380 | 0.2440 | 0.7685 |
| M332 v1 teacher-train + 0.5/1.0 | 0.7266 | 0.3607 | 0.3308 | 0.2385 | 0.7456 | 0.3530 | 0.3339 | 0.2402 | 0.7798 |
| M332 v2 clean 0.25/0.5 | 0.7278 | 0.3624 | 0.3344 | 0.2403 | 0.7468 | 0.3540 | 0.3370 | 0.2414 | 0.7758 |
| M332 v3 clean strong-rank | 0.7304 | 0.3642 | 0.3339 | 0.2427 | 0.7495 | 0.3559 | 0.3364 | 0.2438 | 0.7758 |
| M332 v4 clean tw0.10 strong-rank | 0.7299 | 0.3656 | 0.3347 | 0.2426 | 0.7492 | 0.3573 | 0.3375 | 0.2437 | 0.7758 |

## Per-Dataset M332 v4

| Dataset | R@100 | MRR@20 | NDCG@10 | MAP@100 | Candidate upper-bound R |
| --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | 0.9903 | 0.2252 | 0.3456 | 0.2272 | 1.0000 |
| `fiqa` | 0.7385 | 0.4871 | 0.3917 | 0.3331 | 0.7819 |
| `nfcorpus` | 0.3049 | 0.5810 | 0.3516 | 0.1634 | 0.3786 |
| `scidocs` | 0.4288 | 0.3386 | 0.1818 | 0.1228 | 0.5225 |
| `scifact` | 0.9569 | 0.6169 | 0.6525 | 0.6035 | 0.9878 |

## Interpretation

M332 proves that natural union can expose more relevant candidates:

- M331 upper-bound recall: `0.7685`.
- M332 v4 upper-bound recall: `0.7758`.

But the scorer does not reliably convert this extra headroom into NDCG/MAP.
The new candidates are useful for recall, but they also add false positives
that compete in the top ranks. Stronger false-positive and boundary losses help
recover MRR/MAP, but not enough to create a decisive new SOTA.

The key lesson is that the blocker is not raw candidate availability. It is the
ranking objective's ability to decide which added natural-union candidates
should be admitted near the top.

## Decision

M332 v4 is the best M332 result, but it is not a clean promotion:

- Promote only if the immediate goal is recall/admission.
- Do not promote if the gate is ranking quality across R/MRR/NDCG/MAP.

The next useful direction is not more candidate-union expansion. The next
direction should train a scorer on explicit winner/loser admission decisions:

- positive candidate admitted by dense/qrel but missing from current top-k;
- BM25 high-score false positive that should be demoted;
- atom-only false positive that should be demoted;
- BM25-supported relevant document that should be preserved;
- natural-union candidate that improves recall without top-rank harm.

That is a different objective from M332. M332 should be treated as the
controlled evidence that motivates that next step.

## Verification

Local checks:

```bash
PYTHONPATH=scripts python3 -m py_compile \
    scripts/research_sae_m322_candidate_pool_scorer.py
bash -n scripts/run_m332_full_corpus_natural_scorer_spark.sh
git diff --check
```

