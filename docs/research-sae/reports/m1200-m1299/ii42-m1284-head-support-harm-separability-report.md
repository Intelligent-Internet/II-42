# M1284 Head-Support Harm Separability Audit

## Question

M1283 rejected ordinary selected-source gate/filter features for M721b harm.
M1284 tests a more specific hypothesis:

> Is M721b rank harm caused by selected atoms perturbing the baseline top
> head documents?

If true, baseline-head document support should make rank-harm or
tradeoff-among-gain queries separable before a native replay.

This is an observability audit only.

## Surface

Hard-row smoke:

- `cqadupstack`
- `scidocs`
- `webis-touche2020`

Variant:

- `pair_hgb_rp1_b64_s0.02`

New features:

- selected atom support in baseline top10 and top50 documents;
- signed contribution to baseline head docs;
- absolute contribution and max contribution;
- positive/negative support counts.

No qrels features are used in the observable group.

## Result

LODO separability:

| Feature group | Label | QueryCount | PositiveShare | AUC |
| --- | --- | ---: | ---: | ---: |
| observable | rank_harm | 248 | 0.2097 | 0.3678 |
| observable | safe_gain | 248 | 0.3831 | 0.5780 |
| observable | tradeoff_among_gain | 117 | 0.1880 | 0.5335 |
| diagnostic | rank_harm | 248 | 0.2097 | 0.3568 |
| diagnostic | safe_gain | 248 | 0.3831 | 0.5644 |
| diagnostic | tradeoff_among_gain | 117 | 0.1880 | 0.5349 |

Best observable feature AUCs:

| Label | Best feature | Direction | AUC |
| --- | --- | ---: | ---: |
| rank_harm | `rank_min` | - | 0.5723 |
| safe_gain | `pair_pair_candidate_align_max` | + | 0.6020 |
| tradeoff_among_gain | `head10_max_abs_std` | - | 0.5995 |

## Interpretation

M1284 rejects a simple head-support veto route.

Adding top10/top50 baseline document support features does not make M721b rank
harm observable.  The LODO AUC for `rank_harm` is worse than random, and the
best individual head feature only reaches about `0.60` AUC for
`tradeoff_among_gain`.

This means the M722 top-rank degradation is not explained by a simple
"selected atoms touch the head too much" pattern.  A head-neutral filter would
likely become another weak micro-tuning loop.

## Decision

Do not run native replay for head-support veto variants.

Do not continue with:

- top10/top50 head-support thresholds;
- selected-atom head-risk vetoes;
- rank-harm classifiers based on this feature family;
- deeper training of the same M721b candidate with a head-support penalty.

The next route must change the source or compiler architecture more
structurally.  The most likely next useful question is no longer "can we filter
M721b selected atoms?", but "can we produce a rank-preserving atom source whose
boundary movements are safe before selection?"

## Artifacts

- Script:
  `scripts/audit_m1284_head_support_harm_separability.py`
- JSON:
  `runs/m1284_head_support_harm_separability_smoke_v1/m1284_head_support_harm_separability.json`
- Markdown:
  `runs/m1284_head_support_harm_separability_smoke_v1/m1284_head_support_harm_separability.md`
