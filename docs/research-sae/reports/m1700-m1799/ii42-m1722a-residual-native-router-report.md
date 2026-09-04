# M1722A Residual-Native Router Report

## Decision

**Do not scale the frozen-source residual router.** M1722A ran the full
predeclared 1,500-update canary and selected a trained step-100 checkpoint for
both arms. The residual teacher has a stronger source oracle than the original
dense teacher, but its useful keys are less observable from the frozen query
representation and training captures less of the available gap.

This is not evidence that lexical residual training is impossible. It is a
specific causal result:

> Reweighting the teacher without changing the M1600 document source does not
> repair query/source compatibility.

The next justified repair must jointly move semantic document assignments and
query compatibility. More steps, a larger frozen-source router, or a full
10,000/1,000 rerun are not authorized by this result.

## Controlled Surface

- frozen dense root and embeddings: M1600 BGE caches;
- train/validation: 4,000/500 disjoint MS MARCO text rows and
  35,831/4,498 unique documents;
- frozen source: 8 groups x 512 keys, one key per document/group;
- semantic read budget: `0.15x`;
- BM25: exact corpus-local index, `k1=0.9`, `b=0.4`, top256;
- router: rank 128;
- training: 1,500 updates, batch 32, validation every 100 updates;
- qrels, explicit pair labels, cross-encoder scores, dataset identity, and
  threshold search: unused.

The dense control and residual arm used identical model capacity,
initialization, optimizer, source, data, read budget, and checkpoint rule.
Only the key-utility teacher differed.

ClearML task: `4c37265eade541c2a035e80fae20306c`.

## Capacity Versus Observability

| Variant | Teacher O@100 | Teacher O@256 | Teacher R@100 | Teacher R@256 | Base key AUC | Base key recall |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Dense control | 1.000000 | 0.971445 | 1.000000 | 0.965342 | 0.730598 | 0.139077 |
| Residual | 1.000000 | 0.989352 | 1.000000 | 0.987843 | 0.721456 | 0.111170 |

Removing BM25-covered dense documents makes the oracle more efficient at the
same read budget: residual O@256 rises by `0.017906`. However, the residual
selected-key AUC falls by `0.009142`, and selected-key recall at the matched
count falls by `0.027907`. The source contains the required documents, but its
existing key layout was learned for all dense neighbors, not the lexical
complement.

## Trained Result

| Variant | Surface | O@100 | O@256 | R@100 | R@256 | Reads |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Dense control | base | 0.807300 | 0.662578 | 0.720020 | 0.560618 | 0.149844x |
| Dense control | selected step 100 | 0.808100 | 0.663461 | 0.720805 | 0.561391 | 0.149844x |
| Residual | base | 0.807300 | 0.662578 | 0.720020 | 0.560618 | 0.149844x |
| Residual | selected step 100 | 0.807940 | 0.663070 | 0.720731 | 0.560930 | 0.149844x |

The dense control captures only `0.42%` of its O@100 teacher gap and `0.29%`
of its O@256 gap. The residual arm is weaker at `0.33%` and `0.15%`.
Residual-recovery gains are also below `0.001` in both arms.

All scale gates fail:

- required residual O@100 gain `>=0.005`; observed `+0.000640`;
- required residual O@256 gain `>=0.002`; observed `+0.000492`;
- required residual R@100 gain `>=0.01`; observed `+0.000711`;
- required residual R@256 gain `>=0.005`; observed `+0.000312`;
- required teacher-gap capture `>=5%`; observed below `0.34%`;
- residual must beat the dense control; it does not.

## Training-Depth Evidence

Both variants peak at update 100 and then regress while the optimization
continues. Representative O@100/O@256 values are:

| Step | Dense control | Residual |
| ---: | ---: | ---: |
| 100 | 0.808100 / 0.663461 | 0.807940 / 0.663070 |
| 300 | 0.779320 / 0.637352 | 0.779760 / 0.637930 |
| 800 | 0.737860 / 0.604961 | 0.744000 / 0.611461 |
| 1,500 | 0.739440 / 0.607445 | 0.745800 / 0.613086 |

The residual objective is mildly less destructive late in training, but this
does not constitute a useful checkpoint. The deeper schedule directly answers
the undertraining concern: the route is not being abandoned before
convergence; hard retrieval quality deteriorates long before the schedule
ends.

## Interpretation

M1722A refines the M1600 diagnosis. The problem is not that residual targets
lack sparse source capacity. The residual teacher reaches nearly exact
candidate access with max DF `0.012228`. The problem is that the frozen source
partitions documents according to the original dense geometry. Once lexical
coverage removes part of each neighborhood, the remaining useful key order is
even less aligned with local query-to-centroid logits.

Therefore the following are closed:

- more frozen-router steps;
- scaling this router unchanged to 10,000/1,000;
- rank, learning-rate, loss-weight, BM25-depth, or threshold sweeps;
- claiming that the stronger residual oracle is deployable.

The one structurally distinct continuation is a runtime-matched asymmetric
source model:

- initialize from the dense-root source;
- train document key assignment and query multi-probe compatibility jointly;
- use one document key/group but the actual eight query probes/group in the
  forward pass;
- optimize dense-minus-BM25 candidate admission directly;
- retain balance, max-DF, read, and dense-control arms;
- select only on hard heldout source overlap and residual recovery.

That experiment changes the source geometry rather than asking a static
router to predict corpus-context actions hidden by the source.

## Reproducibility

- host: `spark-1`;
- run:
  `/home/huoju/leask/runs/ii42-m1722-residual-native-v1/runs/m1722a-residual-native-router4k-seed1722-v1`;
- summary SHA-256:
  `52ab02943ff903ff802d183ea407010d5bb8f2765314f019bd42fee8485de9f0`;
- local immutable result:
  `ii42-m1722a-residual-native-router-result.json`;
- focused local verification: 22 tests passed; Python compile, Ruff, shell
  syntax, and `git diff --check` passed before launch.
