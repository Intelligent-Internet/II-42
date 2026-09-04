# ii42 M200 PPLX Stage-A Controlled Ablation Plan

## Summary

M200 is a new exploration line. It is not part of the M190 canonical replay.
M190 showed that the M150-style Stage-A proxy can reach strong train-side
neighbor overlap, but that signal does not reliably transfer to full-corpus
SAE-only or BM25+SAE retrieval.

The M200 goal is to identify which single factor improves PPLX-to-SAE
representation quality before running any expensive full training:

- query participation;
- retrieval-aware Stage-A supervision;
- sparse capacity and active budget.

Each arm changes one variable at a time against the audited M190 PPLX1024
BEIR15 root. Broad external corpus training is a later scale-up step only if
one controlled arm shows a real Stage-A gate improvement.

## Fixed Baseline

Use the audited M190 replay root:

```text
/home/huoju/leask/runs/ii42-m190-replay-actual-beir15-pplx1024
```

Fixed values unless an arm explicitly changes them:

| Item | Value |
| --- | --- |
| embedding model | `perplexity-ai/pplx-embed-v1-0.6B` |
| embedding dimension | `1024` |
| query/document prefix | none |
| similarity | cosine |
| Stage-A trainer | `research_sae_m130_bm25sae_stage_a_pretrain.py` |
| stream mode | `legacy_doc_query` |
| batch size | `384` |
| epochs for full run | `3` |
| learning rate | `2e-4` |
| top-k implementation | `sigmoid` |
| top-k iterations | `24` |
| baseline features | `16384` |
| baseline active k | `96` |

PPLX differs from Snowflake here: PPLX does not require Snowflake-style
`query:` or document prefixes. Prefixes must not be added unless a separate
prefix ablation is explicitly named.

## Smoke Mechanics

Short smoke runs must not compress the annealing schedule. Use
`--schedule-total-steps` for the full estimated run length and `--max-steps`
only as the hard stop.

This is required because early M190 query-repeat smoke with `MAX_STEPS=10000`
changed the tau schedule and produced non-comparable results.

## M200 Arms

### M200-Q: Query Participation

Purpose: test whether M190 under-trained query-side atoms.

| Arm | Change |
| --- | --- |
| `qrep8` | `query_repeat=8`, no coverage loss |
| `qrep16` | `query_repeat=16`, no coverage loss |

Initial acceptance signal:

- better Stage-A candidate gate than M190 baseline;
- no obvious train instability;
- improved SAE-only recall on a small full-corpus canary.

### M200-R: Retrieval-Aware Stage-A Loss

Purpose: test whether Stage-A needs direct retrieval target supervision rather
than only local dense-neighborhood imitation.

Use M190 PPLX1024 Stage-B train rows as coverage supervision:

```text
/home/huoju/leask/runs/ii42-m190-m150-fusionnorm-strictscale-v1-candidate-rows
```

Loss weights start from the M170 M150-style coverage setup:

| Loss | Weight |
| --- | ---: |
| coverage recall | `0.05` |
| BM25-complement CE | `0.10` |
| dense KL | `0.02` |

Initial acceptance signal:

- candidate-eval MRR/hit improves against the same validation rows;
- SAE-only recall improves or does not regress;
- fanout does not rise materially.

### M200-C: Capacity / Active Budget

Purpose: test whether PPLX 1024 needs more sparse capacity.

| Arm | Change |
| --- | --- |
| `32k-k96` | `n_features=32768`, `feature_k=96` |
| `32k-k128` | `n_features=32768`, `feature_k=128` |

Initial acceptance signal:

- stronger SAE-only full-corpus canary;
- no disproportionate posting/fanout increase;
- improvement is visible without retrieval coverage loss.

## Promotion Order

1. Run 10k-12k smoke for `qrep8`, `qrep16`, `coverage`, and `32k-k96`.
2. Compare train overlap, candidate-eval metrics, and a small full-corpus
   canary.
3. Only then run a deeper continuation for the strongest arm.
4. Only after that consider broad corpus scale-up with Wikipedia/arXiv/PubMed.

## Stop Rules

- If an arm improves only train neighbor overlap but not retrieval canary, do
  not promote it.
- If an arm needs benchmark-specific scoring to look good, do not promote it.
- If coverage rows are missing or not PPLX1024, fail fast instead of silently
  disabling the retrieval loss.

## Current Smoke Results

All rows below use the same audited M190 PPLX1024 BEIR15 replay root and the
same Stage-B validation candidate-eval surface unless noted. The validation
surface has 886 rows. Dense on this surface is `hit@20=0.8939` and
`mrr@20=0.6784`; BM25 is `hit@20=0.7246` and `mrr@20=0.4844`.

| Run | Changed variable | Step | hit@20 | mrr@20 | Decision |
| --- | --- | ---: | ---: | ---: | --- |
| `qrep8` | `query_repeat=8` | 12000 | 0.8488 | 0.5918 | Weak; higher query ratio does not solve the gap. |
| `qrep2-control` | `query_repeat=2` | 12000 | 0.8465 | 0.6162 | Baseline control; better ranking than `qrep8`. |
| `coverage-start0-v3` | retrieval coverage loss | 12000 | 0.8747 | 0.6003 | Promising for admission/hit, not sufficient for top-rank ranking. |
| `32k-k96` | `n_features=32768` | 3000 | 0.8172 | 0.5360 | Stopped early; pure capacity is weak and slower. |

Invalid runs:

- `coverage-stagea-smoke-v1` did not exercise coverage loss because
  `coverage_start_frac=0.05` starts after the 12k smoke stop when the full
  schedule length is used.
- `coverage-start0-v1` and `coverage-start0-v2` were launcher failures while
  plumbing `COVERAGE_START_FRAC` through the remote shell and Docker layers.
  They are not model results.

Interim conclusions:

- M200-Q should be parked for now. Query participation changes the data mix,
  but it does not produce a clear retrieval-quality improvement.
- M200-R is the first useful direction. It improves candidate admission
  (`hit@20`) materially, but it still needs a ranking/listwise or calibration
  term because `mrr@20` does not improve over the qrep2 control.
- M200-C should be parked for now. Larger sparse capacity alone worsens early
  candidate-eval quality and increases training/runtime cost.

Recommended next M200 step:

- Keep `coverage-start0-v3` as the admission baseline.
- Add a small rank-aware objective on top of coverage, focused on dense/BM25
  near-ties and high-BM25 false positives.
- Do not increase coverage weights blindly; the current signal is recall-like,
  not rank-like.

## M201 Follow-Up Result

The proposed small rank-aware Stage-A objective was tested in
`docs/research-sae/reports/m0200-m0299/ii42-m201-stagea-coverage-rank-smoke-report.md`.

Result: parked.

The pairwise qrel-positive versus high-BM25 / low-dense negative loss regressed
candidate-eval quality quickly:

| Run | Step | hit@20 | MRR@20 | Decision |
| --- | ---: | ---: | ---: | --- |
| `coverage-rank-smoke-v1`, weight `0.05` | 1250 | 0.8623 | 0.5820 | stop |
| `coverage-rank-w001-smoke-v1`, weight `0.01` | 500 | 0.8634 | 0.5789 | stop |

This suggests the simple pairwise rank loss is too direct for Stage A. The next
attempt should keep Stage A representation-first, or move ranking repair to
Stage B/C on a full-corpus hard-negative surface.
