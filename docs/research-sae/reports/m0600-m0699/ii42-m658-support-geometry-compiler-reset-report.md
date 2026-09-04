# M658 / Support-Geometry Compiler Reset Report

Status: `geometry_gate_passed_retrieval_guard_near_miss`

M658 resets the exploration after M654-M657.  The failed coordinate-mask branch
showed that local query-side edits can move rankings, but not without damaging
dense-equivalent support.  M658 therefore returns to the first-stage
output-head route:

```text
frozen 1024-d dense root -> active-locked compiler -> dense-derived posting
```

This phase intentionally excludes BM25, alpha search, dataset-specific tuning,
and qrels as a training signal.  Qrels are used only for a post-training smoke
diagnostic.

## Why This Differs From M636

M636 added retrieval/listwise pressure directly to the generated-posting
compiler and selected epoch 0 because trained checkpoints damaged dense
overlap or failed Recall.  M658 changes the gate:

1. support geometry is a hard floor, not a soft regularizer;
2. trained checkpoint selection is required;
3. retrieval metrics are diagnostic, not the primary selection objective;
4. no local coordinate patch or support-rank barrier is used.

## Setup Boundary

The first local attempt used
`data/ii42-m599-m551-beir15-shared15-root-v1/_shared/tasks`, whose rows are
768-dimensional.  With M549 `tail_identity_dims=768`, that root degenerates to
identity and has no useful M549 tail objective.  That run was aborted as a
setup mismatch, not a model result.

The valid M658 smoke used the canonical 1024-dimensional broad root on
`spark-1`:

```text
/home/huoju/leask/runs/mteb-eng-v2-retrieval-history-p0-v1/_shared/tasks
```

## Training Run

| Field | Value |
| --- | --- |
| Host | `spark-1` |
| Runner | `scripts/run_m658_support_geometry_compiler_reset_spark.sh` |
| Source trainer | `scripts/research_sae_m549u_official_root_compiler.py` |
| Gate checker | `scripts/check_m658_support_geometry_reset.py` |
| Tasks | `FiQA2018`, `SCIDOCS`, `TRECCOVID`, `ArguAna` |
| Compiler | `active_locked_bucket_delta` |
| Device | `cpu` in Docker |
| Train rows | `12000` |
| Trainable parameters | `32777` |
| Best epoch / step | `1 / 94` |
| Learned gamma | `1.0254510641` |

Artifact root:

```text
runs/ii42-m658-support-geometry-compiler-reset-v1/
  m658_support_geometry_smoke_seed6581/
```

## Geometry Gate

M658 passes the support-geometry gate.  A trained checkpoint was selected and
all hard floors held.

| Metric | Delta |
| --- | ---: |
| doc active recall | +0.00000000 |
| query active recall | +0.00000000 |
| doc support cosine | +0.00005688 |
| query support cosine | +0.00005872 |
| doc support KL | -0.00022254 |
| query support KL | -0.00033080 |
| score Pearson vs M549 | +0.00001756 |
| top100 overlap vs dense | +0.00000000 |
| top100 overlap vs M549 | +0.00322812 |

Per-task final geometry:

| Task | Doc Cos | Query Cos | Doc Active | Query Active | Top100 vs M549 | Top100 vs Dense |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| FiQA2018 | 0.99999809 | 0.99999803 | 0.99986267 | 1.00000000 | 0.99906250 | 0.99625000 |
| SCIDOCS | 0.99999809 | 0.99999797 | 0.99993896 | 1.00000000 | 0.99906250 | 0.99484375 |
| TRECCOVID | 0.99999809 | 0.99999803 | 0.99995422 | 1.00000000 | 0.99940000 | 0.99680000 |
| ArguAna | 0.99999809 | 0.99999809 | 0.99990845 | 1.00000000 | 0.99890625 | 0.99562500 |

## Retrieval Smoke

The retrieval smoke used the generated M658 checkpoint through
`evaluate_m635_output_compiler_smoke.py`.  The report still labels the compiled
source as `m635_compiler`; in this run that row is the M658 checkpoint.

Smoke query cap: `128` queries per task.

| Source | CUB | O@100 | NDCG@10 | MAP@100 | R@100 | MRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| dense root | 0.815023 | 1.000000 | 0.495491 | 0.269021 | 0.605242 | 0.562409 |
| M549 target | 0.814579 | 0.995499 | 0.495009 | 0.268518 | 0.605185 | 0.562013 |
| M658 compiler | 0.814584 | 0.994909 | 0.495444 | 0.269333 | 0.605230 | 0.562744 |
| M658 + dense anchor 0.025 | 0.814570 | 0.995067 | 0.495623 | 0.269337 | 0.605230 | 0.562762 |

Deltas versus dense root:

| Source | dCUB | dO@100 | dNDCG@10 | dMAP@100 | dR@100 | dMRR@20 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| M549 target | -0.000444 | -0.004501 | -0.000482 | -0.000503 | -0.000057 | -0.000396 |
| M658 compiler | -0.000439 | -0.005091 | -0.000047 | +0.000312 | -0.000012 | +0.000335 |
| M658 + dense anchor 0.025 | -0.000453 | -0.004933 | +0.000132 | +0.000316 | -0.000012 | +0.000353 |

## Interpretation

M658 is a positive first-stage signal, but not a completed retrieval
breakthrough.

What improved:

1. The trained output head improves support geometry without dropping active
   recall or dense top100 overlap.
2. It materially improves top100 overlap versus the M549 target on the smoke
   surface.
3. Retrieval smoke improves over M549 target on NDCG, MAP, Recall, and MRR.
4. Versus dense root, MAP and MRR are positive, NDCG is almost flat, and Recall
   is only `-0.000012`.

What still blocks promotion:

1. Recall@100 is still not positive versus dense root.
2. Dense overlap is slightly lower than M549 target despite better teacher
   overlap, so geometry fit and dense ranking preservation are not identical.
3. Candidate upper bound remains slightly below dense root.
4. This is a 4-task smoke, not shared15/native DB validation.

## Decision

Keep M658 as the current first-stage compiler-reset branch.  It is the first
post-M657 result that satisfies strict geometry gates with a trained checkpoint
and gets retrieval metrics close to dense without qrels training.

Do not expand directly to full matrix yet.  The next step should be M659:
increase retrieval relevance without weakening the M658 hard geometry floor.

## Next Step: M659

M659 should be a bounded retrieval-constrained generated-posting objective with
M658 as the baseline and gate:

1. initialize from the M658 checkpoint;
2. freeze or heavily regularize the support-geometry compiler components;
3. add retrieval pressure only as a tie-break inside the M658 floor;
4. reject any trained checkpoint that drops active recall, top100-vs-dense, or
   support cosine;
5. require Recall@100 to become non-negative versus dense on the same 4-task
   smoke before expanding.

If M659 cannot cross the remaining `-0.000012` Recall gap without support
damage, then the next viable branch is not more local retrieval loss.  It
should inspect candidate upper-bound loss and decide whether the teacher target
itself needs a different dense-derived support shape.

## Verification

- `python3 -m py_compile scripts/check_m658_support_geometry_reset.py`
- `python3 -m pytest tests/test_check_m658_support_geometry_reset.py -q`
- `bash -n scripts/run_m658_support_geometry_compiler_reset_spark.sh`

