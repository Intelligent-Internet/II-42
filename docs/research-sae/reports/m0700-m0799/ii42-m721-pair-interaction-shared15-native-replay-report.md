# M721 Pair-Interaction Shared15 Native Replay

M721 broadens the M720 positive canary from four datasets to the shared15
native boundary surface. It keeps the same first-stage constraints:

- no BM25;
- no reranker;
- no qrels-driven objective;
- no learned gate;
- no dataset-specific tuning;
- frozen `P1.3 / M549U native signed-dot` document/index geometry.

## Why This Step

M719 proved that pair-interaction features can recover target-like atom sets.
M720 proved that those sets can move dense-boundary pairs in the native scorer
on the four canary datasets. M721 asks whether that signal survives broader
shared15 validation.

This is still a dense-equivalence proof surface. It is not a final retrieval
matrix and not a second-stage ranking/fusion experiment.

## Boundary Rows

The original M653 canary row file only covered four datasets, so M721 first
built a scored dense teacher and broader boundary rows:

```bash
python3 scripts/build_m608_aligned_dense_rankings.py \
    --atom-root runs/m608_p1p3_signed_dot_query_atoms_shared15_v1 \
    --output-root runs/m721_dense_teacher_top256_shared15_v1 \
    --top-k 256 \
    --include-scores

python3 scripts/build_m653_dense_boundary_training_rows.py \
    --dense-root runs/m721_dense_teacher_top256_shared15_v1 \
    --output-root runs/m721_dense_boundary_training_rows_shared15_v1 \
    --output-md docs/research-sae/reports/m0700-m0799/ii42-m721-dense-boundary-training-rows-report.md
```

Coverage:

| Scope | Pairs | Queries |
| --- | ---: | ---: |
| All | 44910 | 1335 |
| Train | 28063 | 839 |
| Dev | 8073 | 227 |
| Test | 8774 | 269 |

All 15 shared15 datasets produced boundary rows.

## Native Replay

M721 reuses the M720 native replay script with the broader row file. Only the
safe b64 family is tested:

```bash
python3 scripts/train_m720_pair_interaction_native_replay.py \
    --training-rows runs/m721_dense_boundary_training_rows_shared15_v1/m653_dense_boundary_training_rows.jsonl \
    --datasets arguana,climate-fever,cqadupstack,dbpedia-entity,fever,fiqa,hotpotqa,msmarco,nfcorpus,nq,quora,scidocs,scifact,trec-covid,webis-touche2020 \
    --budgets 64 \
    --scales 0.02,0.05,0.1 \
    --score-names pair_hgb_rp0,pair_hgb_rp05,pair_hgb_rp1 \
    --output-root runs/m721_pair_interaction_native_replay_shared15_v1
```

Runtime was `204.72s`.

## Macro Result

Held-out eval rows:

| Variant | Pair success | Baseline | Fixed | Regressed | Top95 | Target recall | Negative-only |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `pair_hgb_rp1_b64_s0.1` | 0.741511 | 0.534351 | 787 | 0 | 0.936036 | 0.861306 | 0.002079 |
| `pair_hgb_rp05_b64_s0.1` | 0.738352 | 0.534351 | 775 | 0 | 0.934847 | 0.882384 | 0.009703 |
| `pair_hgb_rp0_b64_s0.1` | 0.725191 | 0.534351 | 726 | 1 | 0.933234 | 0.916636 | 0.026147 |
| `pair_hgb_rp1_b64_s0.05` | 0.643064 | 0.534351 | 413 | 0 | 0.967381 | 0.861306 | 0.002079 |
| `pair_hgb_rp05_b64_s0.05` | 0.641221 | 0.534351 | 406 | 0 | 0.966787 | 0.882384 | 0.009703 |
| `pair_hgb_rp0_b64_s0.05` | 0.635694 | 0.534351 | 385 | 0 | 0.965471 | 0.916636 | 0.026147 |
| `pair_hgb_rp1_b64_s0.02` | 0.574625 | 0.534351 | 153 | 0 | 0.985993 | 0.861306 | 0.002079 |
| `pair_hgb_rp05_b64_s0.02` | 0.574362 | 0.534351 | 152 | 0 | 0.985569 | 0.882384 | 0.009703 |
| `pair_hgb_rp0_b64_s0.02` | 0.569887 | 0.534351 | 135 | 0 | 0.985059 | 0.916636 | 0.026147 |

The broader-safe default is `pair_hgb_rp1_b64_s0.02`:

- pair success improves from `0.534351` to `0.574625`;
- fixed/regressed is `153/0`;
- top95 overlap is `0.985993`;
- negative-only share is `0.002079`.

The canary-safe `s0.05` setting still improves strongly, but macro top95 is
`0.967381`, below the current `0.97` broader floor. It should remain an
aggressive candidate, not the default.

## Dataset Safe Rows

Best per-dataset row with `top95 >= 0.97`:

| Dataset | Variant | Pair success | Baseline | Fixed | Regressed | Top95 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `arguana` | `pair_hgb_rp1_b64_s0.05` | 0.630155 | 0.519330 | 86 | 0 | 0.972947 |
| `climate-fever` | `pair_hgb_rp05_b64_s0.05` | 0.636616 | 0.536755 | 72 | 0 | 0.976737 |
| `cqadupstack` | `pair_hgb_rp1_b64_s0.02` | 0.640327 | 0.603542 | 27 | 0 | 0.987135 |
| `dbpedia-entity` | `pair_hgb_rp1_b64_s0.02` | 0.513250 | 0.456067 | 41 | 0 | 0.987111 |
| `fever` | `pair_hgb_rp05_b64_s0.02` | 0.538363 | 0.484655 | 42 | 0 | 0.985684 |
| `fiqa` | `pair_hgb_rp05_b64_s0.02` | 0.606299 | 0.573491 | 25 | 0 | 0.985789 |
| `hotpotqa` | `pair_hgb_rp05_b64_s0.02` | 0.561558 | 0.511307 | 40 | 0 | 0.986211 |
| `msmarco` | `pair_hgb_rp05_b64_s0.02` | 0.512397 | 0.433884 | 19 | 0 | 0.986505 |
| `nfcorpus` | `pair_hgb_rp1_b64_s0.02` | 0.579545 | 0.534091 | 36 | 0 | 0.982632 |
| `nq` | `pair_hgb_rp05_b64_s0.02` | 0.533920 | 0.494975 | 31 | 0 | 0.987053 |
| `quora` | `pair_hgb_rp05_b64_s0.02` | 0.543906 | 0.507208 | 28 | 0 | 0.984421 |
| `scidocs` | `pair_hgb_rp0_b64_s0.02` | 0.522843 | 0.473350 | 39 | 0 | 0.983579 |
| `scifact` | `pair_hgb_rp05_b64_s0.02` | 0.583333 | 0.541667 | 33 | 0 | 0.986421 |
| `trec-covid` | `pair_hgb_rp1_b64_s0.02` | 0.574742 | 0.487113 | 34 | 0 | 0.978737 |
| `webis-touche2020` | `pair_hgb_rp05_b64_s0.02` | 0.629944 | 0.570621 | 21 | 0 | 0.984962 |

All 15 datasets have a safe positive row. This rules out a canary-only effect.

## Interpretation

M721 is a meaningful broader positive signal for the first-stage route.

The result supports the current hypothesis:

1. M716-M718 were not mainly under-trained. They lacked pair-interaction
   information.
2. M719 recovered the missing information.
3. M720 showed native movement on canary.
4. M721 shows the movement generalizes across shared15 when the scale is
   conservative enough.

The important correction is scale. M720 canary allowed `s0.05` as the safe
default. M721 broader validation says the default should be `s0.02`; `s0.05`
is useful but too close to the dense-overlap floor for promotion.

## Decision

Promote the pair-interaction route to the next first-stage probe, with
`pair_hgb_rp1_b64_s0.02` as the broader-safe default candidate.

Do not promote `s0.05` yet. Do not add BM25/reranker yet.

Next step M722:

1. Evaluate whether the safe boundary-pair movement translates into dense
   overlap@100/@256 and standard retrieval metrics on the native shared15
   matrix.
2. Keep `pair_hgb_rp1_b64_s0.02` frozen as the default.
3. Include `pair_hgb_rp05_b64_s0.02` as a near-tie alternative.
4. Treat `s0.05` as aggressive diagnostic only.
5. Reject the route if broader retrieval metrics regress despite the boundary
   pair-success improvement.

## Artifacts

- Dense teacher: `runs/m721_dense_teacher_top256_shared15_v1`
- Boundary rows: `runs/m721_dense_boundary_training_rows_shared15_v1`
- Boundary row report: `docs/research-sae/reports/m0700-m0799/ii42-m721-dense-boundary-training-rows-report.md`
- Native replay: `runs/m721_pair_interaction_native_replay_shared15_v1`
- Replay summary: `runs/m721_pair_interaction_native_replay_shared15_v1/m720_summary.json`
