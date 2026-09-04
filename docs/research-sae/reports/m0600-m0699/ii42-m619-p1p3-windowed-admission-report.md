# II-42 M619 P1.3 Windowed Admission Report

Date: 2026-07-06

## Objective

M619 tests whether the M617 learned recovery signal is useful in the safer
M610-style shape: preserve the head and admit only a small number of lower-slot
tail candidates.  It does not train a new model and does not change the P1
encoder/posting generator.

This is a full M604 replay probe, not a native DB/plugin benchmark.

## Implementation

- Probe script: `scripts/probe_m619_windowed_admission.py`
- Model: `runs/m617_p1p3_recovery_ranker_v1/m617b_alpha010_keep10/m617b_recovery_ranker_model.json`
- Input rows: all `runs/m608_p1p3_m604_scorer_gap_shared15_v1/*_m604_p1_scorer_gap.jsonl`
- Output JSON:
  `runs/m619_p1p3_windowed_admission_v1/m619_m617b_admission_probe.json`
- Output Markdown:
  `runs/m619_p1p3_windowed_admission_v1/m619_m617b_admission_probe.md`

Grid:

- Preserve top-k: `95, 98, 99`
- Max admission rank: `200, 300, 500, 700, 1000`
- Model blend alpha: `0.05, 0.10, 0.20`

## Result

The probe tested 45 configs and accepted 15 under the mechanical macro gate.

Best config:

`top99_max200_a0.2`

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Cand UB |
| --- | ---: | ---: | ---: | ---: | ---: |
| Baseline | 0.775070 | 0.711403 | 0.882313 | 0.857118 | 1.000000 |
| M619 | 0.775070 | 0.711458 | 0.882699 | 0.857118 | 1.000000 |

Delta:

| Metric | Delta |
| --- | ---: |
| NDCG@10 | +0.000000 |
| MAP@100 | +0.000055 |
| Recall@100 | +0.000387 |
| MRR@20 | +0.000000 |

Per-dataset deltas for the best config:

| Dataset | dMAP@100 | dRecall@100 |
| --- | ---: | ---: |
| cqadupstack | -0.000131 | -0.002538 |
| dbpedia-entity | +0.000313 | +0.000597 |
| msmarco | +0.000328 | +0.000393 |
| nfcorpus | +0.000117 | +0.002181 |
| scidocs | +0.000100 | +0.004000 |
| trec-covid | +0.000196 | +0.000382 |
| webis-touche2020 | +0.000204 | +0.001200 |

Rows not listed are unchanged or effectively unchanged.

## Interpretation

M619 confirms the same broad pattern as M610:

- Conservative bottom-slot admission is the only safe shape found so far.
- It can recover a few relevant documents without hurting NDCG@10 or MRR@20.
- The effect size is tiny.
- The best config still harms `cqadupstack` Recall@100.

This is better than M617 free/windowed reranking, which either collapsed on the
full candidate pool or gave near-zero gains.  But it is not large or clean
enough to justify native DB/plugin benchmark execution.

## Decision

Do not promote M619.

Do not run a native DB/plugin benchmark for this policy.

Stop the current learned bottom-slot admission family.  Further alpha/window
search is unlikely to solve the bottleneck because the best valid shape admits
only one bottom slot and produces sub-0.001 macro Recall gain.

## Recommended Next Step

The next scorer attempt needs new information, not more rank-policy tuning:

1. Add richer query-local features that can distinguish dense-plausible
   blockers from dense-miss positives.
2. Candidate features should come from atom/posting interaction structure, not
   only P1/BM25/fused ranks and scalar scores.
3. The next family must first beat M619 on full M604 replay, not only selected
   split surfaces.

If no stronger query-local feature exists, the scorer route should stop and the
project should return to changing the retrieval objective or training a new
teacher signal beyond dense-equivalent preservation.

## Verification

Local verification passed:

- `python3 -m py_compile scripts/probe_m619_windowed_admission.py`
- `pytest -q tests/test_probe_m619_windowed_admission.py`
- `git diff --check`
