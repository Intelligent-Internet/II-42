# M1726A Full-Scale Load-Generalization Report

## Decision

**Stop the M1723-M1726 joint-source family.** Increasing the corpus from
35,831/4,498 to 88,992/8,988 documents materially reduces the train-to-
validation posting-load shift, but no trained checkpoint improves the frozen
full-scale initialization. The failed M1725 gate was therefore not only a
small-corpus DF estimation problem.

ClearML task: `564bddf3254f46938dbaf9eafdd979b4`.

## Frozen Surface

- M1600 10,000/1,000 disjoint query pools;
- 88,992 train and 8,988 validation documents;
- M1600 full 8x512 initialization;
- exact corpus-local BM25 top256 cache;
- identical dense-control and dense-minus-BM25 residual arms;
- 1,600 updates, batch 16, 128 candidates, 96 teacher documents;
- one document key and eight query probes per group;
- full hard train-corpus DF refresh every 50 updates;
- exact train and validation posting reads at every evaluation;
- fixed `0.18x` read target, lambda 100, seed 1726;
- no qrels, dataset identity, threshold search, or metric-selected rerun.

The frozen initialization is:

| O@100 | O@256 | R@100 | R@256 | Validation reads | Train reads | Max DF |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0.865390 | 0.745742 | 0.811074 | 0.679794 | 0.146086x | 0.139055x | 0.009457 |

## Best Trained Checkpoints

| Arm | Step | O@100 | O@256 | R@100 | R@256 | Validation reads | Max DF |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Dense control | 100 | 0.860380 | 0.739422 | 0.803611 | 0.671954 | 0.141987x | 0.009346 |
| Residual, best O@100 | 400 | 0.861980 | 0.746465 | 0.805858 | 0.681131 | 0.161243x | 0.010570 |
| Residual, best O@256 | 500 | 0.860410 | 0.746777 | 0.804789 | 0.682308 | 0.159355x | 0.009902 |

Residual step 400 passes both cost limits, but versus initialization it is:

- O@100 `-0.003410`;
- O@256 `+0.000723`;
- R@100 `-0.005216`;
- R@256 `+0.001336`;
- validation reads `+0.015157x`.

Residual step 500 has the best O@256, but still loses O@100 `-0.004980` and
R@100 `-0.006285`. Every evaluated residual checkpoint through step 1,600
loses O@100 and R@100. Consequently epoch zero is the only valid selection,
and the formal decision is `stop_joint_source_no_hard_checkpoint`.

## What Scale Fixed

At M1725's quality peak, exact reads were:

- train corpus: `0.171965x`;
- validation corpus: `0.209341x`;
- gap: `+0.037376x`.

At M1726 residual step 400 they are:

- train corpus: `0.147929x`;
- validation corpus: `0.161243x`;
- gap: `+0.013314x`.

The larger corpus reduces the split gap by about 64%. Full-corpus DF
measurement and more data therefore solve a real estimator/generalization
problem. This does not recover the canary quality gain: the stronger
full-scale initialization leaves no useful headroom for the joint update.

## Causal Interpretation

The residual objective remains better than the matched dense objective. At
step 400, residual minus dense-control is:

- O@100 `+0.009630`;
- O@256 `+0.015797`;
- R@100 `+0.012617`;
- R@256 `+0.019520`;
- reads `+0.015463x`.

Thus dense-minus-BM25 supervision is not noise. It teaches a more useful
direction than dense-only supervision, but moving the shared source destroys
part of the already-good frozen geometry. On the 4k canary this trade appeared
as a net gain because initialization was weaker and load estimates were less
stable. On the full surface it is only damage reduction.

This directly rejects the undertraining explanation. More updates do not
approach a delayed pass: the best residual O@100 occurs at step 400, and steps
600-1,600 remain below initialization. Another seed, schedule, lambda, probe
count, or relaxed cost target is not authorized.

## Stop Boundary

Closed:

- replacing or jointly moving the M1600 base source with this residual loss;
- deeper training of M1723-M1726;
- query-cost coefficient, probe-count, learning-rate, or gate sweeps;
- interpreting matched-control gains as product gains;
- native/qrel evaluation of a checkpoint that cannot beat initialization.

The remaining structurally distinct question is whether a frozen base can be
protected by construction while a separate, small residual namespace uses
only the unused posting budget. That question must begin with a deterministic
full-scale capacity/observability oracle. It must not begin with another model
training run.

## Reproducibility

- host: `spark-1`;
- run:
  `/home/huoju/leask/runs/ii42-m1726-fullscale-load-v1/runs/m1726a-fullscale-load10k-seed1726-v1`;
- summary SHA-256:
  `28ee5fec0f9892b9a169f03e869bb71d86b9bf2b12908d69a20488db4a04552f`;
- local result: `ii42-m1726a-fullscale-load-generalization-result.json`.
