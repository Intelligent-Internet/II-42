# M601 Stage-2 Auditable BM25 Ranking Rebuild Plan

## Purpose

M601 rebuilds the successful second-stage route on top of the now-stable
M549U first-stage encoder/compiler.

The first-stage contract stays unchanged:

```text
text -> frozen PPLX/ST root -> M549U output compiler -> dense-equivalent posting
```

M601 starts only after this surface is stable.  It adds BM25 and ranking
optimization as a separate, auditable second stage:

```text
M549U posting score + lexical/ranking stage -> final retrieval score
```

This separation is mandatory.  BM25 must not be trained into the M549U
first-stage encoder until a later explicit unification experiment proves that
doing so preserves the BM25-free dense-equivalent surface.

## Rebuild Target

The known successful second-stage baseline is M550's fixed z-score blend:

```text
score = zscore(m549_tail768_score) + 0.10 * zscore(bm25_score)
```

M549U.21 has already shown that M549U active-locked can substitute for M549
in this blend:

```text
score = zscore(m549u_active_locked_score) + 0.10 * zscore(bm25_score)
```

Full broad10 macro:

| Source | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `exact_dense` | 0.57263 | 0.43535 | 0.74803 | 0.65179 | 1.00000 |
| `m549_tail768` | 0.57279 | 0.43545 | 0.74802 | 0.65195 | 0.99525 |
| `m549u_active_locked` | 0.57279 | 0.43545 | 0.74802 | 0.65195 | 0.99525 |
| `m549_bm25_zblend_a010` | 0.59045 | 0.44785 | 0.75494 | 0.66396 | 0.87669 |
| `m549u_active_locked_bm25_zblend` | 0.59045 | 0.44785 | 0.75494 | 0.66396 | 0.87669 |

This is the replay baseline.  Any new M601 optimizer must beat or match this
surface under stricter audit conditions before it is promoted.

## Design Rules

1. Keep stage boundaries explicit.
   - First stage: M549U dense-equivalent posting.
   - Second stage: BM25/ranking fusion over fixed first-stage scores.
   - Do not use BM25 improvements to justify a weaker first-stage encoder.

2. Make replay deterministic.
   - Record root path, task list, source checkpoint, alpha grid, candidate_k,
     qrel split, and score normalization in every JSON.
   - Store per-task source metrics and macro metrics.
   - Add merge guards for exact task coverage and no duplicate task units.

3. Treat fixed BM25 as the control.
   - Fixed `alpha=0.10` is the conservative promoted M550 surface.
   - `alpha=0.15` can be measured, but it must not replace `0.10` unless it
     wins heldout and broad replay without recall/MRR regression.

4. Restrict learned optimizers.
   - No free document-level residual without monotonic/fallback constraints.
   - Learned gates can choose from a small alpha set or interpolate within a
     bounded range.
   - Every learned gate must have a dense/posting fallback floor.

5. Avoid dataset cheating.
   - Do not use dataset id as a feature.
   - Do not tune per-dataset alpha.
   - If qrels are used for a learned second-stage selector, split query groups
     into train/heldout and promote only from heldout.

## Work Packages

### M601.0 Audit Manifest

Create a compact manifest format for second-stage runs:

- first-stage source:
  - `m549u_active_locked`;
  - compiler checkpoint path;
  - canonical root path;
  - transform id `tail768_1p025625`.
- lexical source:
  - BM25 implementation;
  - tokenization/config hash if available;
  - candidate depth.
- normalization:
  - per-query z-score definition;
  - tie handling;
  - score clipping if used.
- evaluation:
  - task roots;
  - query split;
  - qrel file hashes or size/mtime;
  - metric set.

Stop condition: do not run new learned experiments until this manifest exists.

### M601.1 Exact Replay

Replay the already validated fixed blend with the active-locked M549U source:

```text
alpha in [0.05, 0.10, 0.15]
```

Required outputs:

- three-task smoke with FiQA2018, SCIDOCS, TRECCOVID;
- full broad10 replay;
- strict equivalence check against M549/M550 where old outputs are available;
- report section comparing exact dense, M549U, BM25, and fixed blends.

Promotion gate:

- `alpha=0.10` must reproduce `NDCG@10 0.59045`, `MAP@100 0.44785`,
  `R@100 0.75494`, and `MRR@20 0.66396` within report precision on broad10.
- If replay misses this, stop and debug evaluator/config drift before any
  learned optimization.

### M601.2 Predeclared Alpha Sweep

Run a slightly wider fixed-alpha sweep:

```text
alpha in [0.00, 0.025, 0.05, 0.075, 0.10, 0.125, 0.15, 0.175, 0.20]
```

This is still a fixed control, not a learned optimizer.

Promotion gate:

- Prefer the smallest alpha within `0.001` NDCG of the heldout winner if it has
  better Recall@100 or MRR@20 stability.
- Do not promote a higher alpha if it only improves all-query macro while
  heldout or per-task stability worsens.

### M601.3 Conservative Learned Gate

Train a query-level gate that selects or interpolates alpha from the fixed
grid.  Features must be generic query/candidate-set statistics, for example:

- BM25 score entropy and top-gap;
- M549U score entropy and top-gap;
- BM25/M549U top-k overlap;
- score correlation between BM25 and M549U candidates;
- candidate-set size and score variance.

Prohibited features:

- dataset id;
- qrel-derived per-query labels at inference;
- document ids or task-specific memorization.

Training objective:

- listwise heldout-aware objective over train query groups;
- floor penalty if the selected alpha underperforms M549U-only;
- optional monotonic regularization to keep alpha near the fixed prior.

Promotion gate:

- Must beat fixed `alpha=0.10` on heldout NDCG and not lose Recall@100.
- Must not lose broad10 all-query MRR@20 versus fixed `alpha=0.10`.
- Must include fallback behavior when gate confidence is low.

### M601.4 Ranking Optimizer Canary

Only after M601.3 passes should we test ranking optimization beyond alpha
selection.

Allowed shape:

```text
final_score =
    m549u_z
    + alpha(query_features) * bm25_z
    + beta(query_features) * bounded_monotonic_features
```

The residual must be bounded and monotonic.  It cannot freely reorder documents
outside a small rerank window unless it beats the fixed blend on heldout and
passes the fallback floor.

Promotion gate:

- Heldout must beat M601.1 fixed baseline.
- Broad10 all-query must beat or match the fixed baseline on at least three of
  four primary metrics.
- Any Recall@100 regression larger than `0.001` blocks promotion.

### M601.5 Broader Validation

After broad10 passes:

- run official1024 BEIR8 with the streaming evaluator extended for BM25;
- run local BEIR15/shared root if available and cheaper than official full
  roots;
- keep exact dense, M549U, fixed blend, and learned second-stage outputs in
  the same matrix.

Promotion gate:

- The learned second stage must beat or match fixed `alpha=0.10` on broad10
  and at least one broader validation surface.
- If the learned route only wins on broad10 but loses broader BEIR, preserve
  fixed `alpha=0.10` as the deployed second-stage baseline.

## Immediate Implementation Steps

1. Preserve M549U as the first-stage milestone.
2. Keep `scripts/research_sae_m549u_stage2_fixed_bm25.py` as the replay
   baseline and add manifest fields instead of changing score semantics.
3. Add an M601 runner that only invokes replay/audit mode first.
4. Add a separate M601 learned-gate script after replay is verified.
5. Update reports in this order:
   - M601 replay report;
   - M601 alpha sweep report;
   - M601 learned gate report, only if canary passes.

## Stop Conditions

Stop the M601 branch and keep fixed M550/M549U blend if any of these happens:

- replay cannot reproduce the existing fixed baseline;
- learned gate cannot beat fixed `alpha=0.10` on heldout;
- learned gate improves NDCG but loses Recall@100 or MRR@20 beyond tolerance;
- learned residual repeats the M550 free-residual failure mode;
- broader validation says the learned route is a broad10-only artifact.

## Current Decision

The next stage should not start from a free learned ranker.  It should rebuild
the successful second stage in the following order:

```text
M549U fixed replay -> alpha sweep -> conservative query gate -> bounded ranker
```

The last known safe second-stage product remains:

```text
M549U active-locked + fixed BM25 z-score blend alpha=0.10
```
