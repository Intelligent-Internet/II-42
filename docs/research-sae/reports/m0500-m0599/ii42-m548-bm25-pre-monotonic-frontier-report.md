# M548 BM25-Pre Monotonic Frontier Report

## Purpose

M548 answers one narrow question before BM25 or search-aware training is
introduced:

```text
Can the current M546 monotonic output compiler be pushed further while still
remaining a dense-equivalent stage-one surface?
```

This run does not train on qrels, does not use BM25, and does not introduce a
ranking loss.  It uses qrels only to verify that the dense-only retrieval
surface remains stable.

## Artifacts

Runner:

```text
scripts/run_m548_bm25_pre_monotonic_frontier_spark.sh
```

Report builder:

```text
scripts/research_sae_m548_monotonic_frontier_report.py
```

Remote run:

```text
/home/huoju/leask/runs/ii42-m548-bm25-pre-monotonic-frontier-v1/
  broad10_gamma_frontier_v1/
```

Local mirror:

```text
outputs/m548/bm25_pre_monotonic_frontier/broad10_gamma_frontier_v1/
```

Combined report:

```text
outputs/m548/bm25_pre_monotonic_frontier/broad10_gamma_frontier_v1/
  m548_broad10_gamma_frontier_v1_summary.md
```

## Teacher-Fit Frontier

The M546 three-seed broad10 traces already form a useful frontier.  All three
checkpoints preserve active support under the strict active floor.  Larger
gamma improves soft teacher fit but costs support geometry.

| Checkpoint | Gamma Mean | Support Drop | Active Drop | KL Relative Improvement | Passes |
| --- | ---: | ---: | ---: | ---: | --- |
| `step100` | 1.00954815 | 0.00009159 | 0.00000000 | 5.46% | yes |
| `step200` | 1.01827836 | 0.00019442 | 0.00000000 | 9.90% | yes |
| `step300` | 1.02562527 | 0.00029496 | 0.00000000 | 13.23% | yes |

The selected BM25-pre gamma remains:

```text
gamma = 1.02562527
```

It is selected by the teacher-fit constraints, not by qrels.

## Dense-Only Retrieval Frontier

Full broad10 dense-only retrieval was rerun on spark-1 with a gamma grid from
`1.005` to `1.050`.  The key baseline is exact dense:

| Source | Gamma | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | dNDCG | dR@100 | Gate |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `exact_dense` |  | 0.57263 | 0.43535 | 0.74803 | 0.65179 | 1.00000 | 0.00000 | 0.00000 | pass |
| `row_int8` |  | 0.57269 | 0.43543 | 0.74799 | 0.65187 | 0.99660 | +0.00006 | -0.00004 | pass |
| `gamma_1p005` | 1.00500000 | 0.57260 | 0.43543 | 0.74808 | 0.65184 | 0.99872 | -0.00003 | +0.00005 | pass |
| `gamma_1p010` | 1.01000000 | 0.57269 | 0.43541 | 0.74790 | 0.65187 | 0.99750 | +0.00006 | -0.00013 | pass |
| `gamma_1p015` | 1.01500000 | 0.57276 | 0.43534 | 0.74787 | 0.65180 | 0.99648 | +0.00013 | -0.00016 | pass |
| `gamma_1p020` | 1.02000000 | 0.57271 | 0.43530 | 0.74789 | 0.65183 | 0.99539 | +0.00008 | -0.00014 | pass |
| `gamma_1p025` | 1.02500000 | 0.57286 | 0.43531 | 0.74784 | 0.65188 | 0.99427 | +0.00023 | -0.00019 | pass |
| `gamma_1p025625` | 1.02562527 | 0.57282 | 0.43531 | 0.74779 | 0.65188 | 0.99414 | +0.00019 | -0.00024 | pass |
| `gamma_1p030` | 1.03000000 | 0.57284 | 0.43530 | 0.74773 | 0.65190 | 0.99334 | +0.00021 | -0.00030 | fail |
| `gamma_1p040` | 1.04000000 | 0.57296 | 0.43525 | 0.74771 | 0.65181 | 0.99151 | +0.00033 | -0.00032 | fail |
| `gamma_1p050` | 1.05000000 | 0.57305 | 0.43538 | 0.74779 | 0.65203 | 0.98944 | +0.00042 | -0.00024 | fail |

Gate:

- Recall@100 drop must be no worse than `0.0003`.
- Dense overlap@100 must stay at or above `0.994`.

## Interpretation

The current line is solid but close to its BM25-pre ceiling.

The positive result is that `gamma=1.02562527` gives a reproducible
stage-one improvement in teacher distribution fit while preserving active
support and keeping broad10 dense-only retrieval essentially unchanged.  It
also slightly improves macro NDCG@10 versus exact dense by `+0.00019`, though
that is not large enough to call a meaningful dense win.

The negative result is that pushing gamma higher mostly buys cosmetic NDCG
movement by giving up dense geometry.  `gamma=1.050` reaches the largest NDCG
delta in this grid, but overlap@100 falls to `0.98944`, so it is no longer a
clean dense-equivalent surface.  That is not the right object to carry into
BM25-aware training.

## Decision

M548 keeps the BM25-pre stage-one baseline as:

```text
exact dense
row_int8 dense
M546 monotonic gamma mean = 1.02562527
```

Further dense-only monotonic tuning is not the highest-value path.  The route
is now strong enough to reintroduce BM25/search-aware optimization as a
separate stage, with M548 as the frozen dense-equivalent floor.
