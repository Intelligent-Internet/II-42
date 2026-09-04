# M632 / P1.6 Smoke Gate Report

Status: smoke gate failed; shared15 and native replay skipped.

## Best Candidate

The best candidate by Recall-first selection was the linear head with swap
budget 5.  It improved Recall but failed the top-ranking guard.

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense overlap@100 | Dense KL |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `P1.3-a010` | 0.673264 | 0.410853 | 0.563729 | 0.792249 | 0.932157 | 1.138930 |
| `M632-linear-swap5` | 0.595267 | 0.322866 | 0.566961 | 0.699658 | 0.904706 | 0.686651 |
| `delta` | -0.077996 | -0.087987 | +0.003231 | -0.092591 | -0.027451 | -0.452278 |

The guarded config also failed:

| Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dDense overlap@100 | dKL |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `M632-guarded-linear-swap5` | -0.146484 | -0.130506 | +0.003462 | -0.164816 | -0.004314 | -0.453376 |
| `M632-guarded-residual-swap5` | -0.004617 | -0.001045 | -0.000485 | -0.000901 | -0.001176 | -0.040577 |

## Gap Delta Context

| Model | Surface | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | Decision |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `M629-B pairwise cross` | shared15 eval split | +0.000000 | +0.000078 | +0.001923 | +0.000000 | guarded, not promoted |
| `M630-B anchor010` | smoke | -0.000922 | +0.001235 | +0.000000 | +0.004724 | guarded, not promoted |
| `M631 best probe lexical_coverage` | smoke | -0.138108 | -0.045380 | -0.001821 | -0.112151 | stopped |
| `M632 best linear` | smoke | -0.077996 | -0.087987 | +0.003231 | -0.092591 | stopped |

## Decision

Stop M632 at smoke.

Do not run shared15.  Do not run native DB/plugin replay.  The acceptance gate
requires positive Recall with `NDCG@10` and `MRR@20` regression no worse than
`0.001`; M632 does not meet that.

## Next Direction

The useful signal is that dense-tail promotion is learnable enough to move
Recall.  The failure is that current P1/posting features cannot make safe
promotion decisions without damaging the head.

The next serious route should not be a deeper reranker over the same feature
surface.  It should revisit the encoder/posting representation or train a
structured compiler target that explicitly preserves head ordering while
recovering dense-tail support.
