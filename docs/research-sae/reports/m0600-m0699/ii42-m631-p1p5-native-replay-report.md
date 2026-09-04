# M631 / P1.5 Native Replay Report

Status: native replay expansion not started; smoke gate failed.

## Objective

M631 required final validation through M604/M603-compatible native replay, but
only after M631-B produced a non-zero Recall@100 improvement on smoke.  That
gate did not pass, so full shared15 and DB/native engineering replay are
intentionally skipped.

## Smoke Eval-Style Rows

M631 still emits M603-style eval rows for the best smoke probe so downstream
comparisons can inspect the failed direction:

| Source | Path |
| --- | --- |
| P1.3-a010 smoke rows | `runs/m631_p1p5_dense_tail_probe_smoke_v1/eval_style/m631_p1p5_smoke/*/P1.3-a010/eval.json` |
| M631 best probe rows | `runs/m631_p1p5_dense_tail_probe_smoke_v1/eval_style/m631_p1p5_smoke/*/M631-probe-lexical_coverage/eval.json` |

## Smoke Replay Matrix

| Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | Dense overlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `P1.3-a010` | 0.673264 | 0.410853 | 0.563729 | 0.792249 | 0.932157 |
| `M631-probe-lexical_coverage` | 0.535156 | 0.365473 | 0.561909 | 0.680098 | 0.844706 |
| `delta` | -0.138108 | -0.045380 | -0.001821 | -0.112151 | -0.087451 |

Candidate upper bound is unchanged at `0.865332`, so the failure is not caused
by candidate availability.  The best probe actively worsens top100 ordering.

## Gap Delta Context

| Model | Surface | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | Decision |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `M629-B pairwise cross` | shared15 eval split | +0.000000 | +0.000078 | +0.001923 | +0.000000 | guarded, not promoted |
| `M630-B anchor010` | smoke | -0.000922 | +0.001235 | +0.000000 | +0.004724 | guarded, not promoted |
| `M631 best probe lexical_coverage` | smoke | -0.138108 | -0.045380 | -0.001821 | -0.112151 | stopped |

M631 does not meet the minimum smoke requirement of non-zero positive
Recall@100 delta.  It is also below the already guarded M629-B context gain and
does not preserve M630's bounded top-metric behavior.

## Decision

Stop M631 at B.

Do not expand to shared15.  Do not run native DB/plugin replay.  Do not train
M631-C or generate M631-D correction config from this probe.

The actionable conclusion is that the current P1/posting feature surface is
not sufficient for a hand-designed dense-tail support channel.  The next stage
should move upstream to first-stage output-head/posting-compiler training
rather than continuing to tune feature probes.
