# II-42 M506c Adaptive Fanout Probe

## Purpose

M506b showed that the PPLX-root support head works, but Broad4 prefix 256 had a
visible TRECCOVID failure:

- row-int8 dense teacher: NDCG@10 0.78947;
- M506b support head prefix 256: NDCG@10 0.68205;
- Dense O@100 0.34080, Candidate R@100 0.34120, Touch 0.15779.

M506c tests whether this is a semantic failure of the PPLX-root route or a
fanout/prefix coverage failure.

No BM25 and no qrels are used for training. Qrels are evaluation-only.

## Artifacts

Remote run root:

- `/home/huoju/leask/runs/ii42-m506c-adaptive-fanout-v1`

Local artifacts:

- `outputs/m506c/treccovid_prefix512_probe/`
- `outputs/m506c/treccovid_prefix768_probe/`

Both probes use:

- task: `TRECCOVID`;
- variant: `rotation_residual`;
- support loss: dense score + TopK membership;
- fixed structural dense-tail scorer;
- heldout query canary;
- eval queries: 25;
- active dims: 128;
- sketch dims: 128;
- no BM25.

## TRECCOVID Prefix Sweep

| Source | Prefix | NDCG@10 | R@100 | MRR@20 | MAP@100 | Dense O@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `teacher_row_int8_dense` | all | 0.78947 | 0.14829 | 0.91333 | 0.11243 | 1.00000 | 1.00000 | 1.00000 |
| `exact_materialized_dense` | all | 0.78767 | 0.14818 | 0.91333 | 0.11233 | 0.99680 | 1.00000 | 1.00000 |
| `m506b_rotation_residual_support_structural_score` | 256 | 0.68205 | 0.09138 | 0.90667 | 0.05875 | 0.34080 | 0.34120 | 0.15779 |
| `m506b_rotation_residual_support_structural_score` | 512 | 0.77018 | 0.11706 | 0.86333 | 0.08573 | 0.54800 | 0.56240 | 0.27949 |
| `m506b_rotation_residual_support_structural_score` | 768 | 0.78104 | 0.12597 | 0.89000 | 0.09583 | 0.66200 | 0.69480 | 0.37707 |
| `m506b_structural_compiler` | 512 | 0.70348 | 0.08935 | 0.88667 | 0.06089 | 0.34240 | 0.34760 | 0.29093 |
| `m506b_structural_compiler` | 768 | 0.71489 | 0.10616 | 0.85000 | 0.07368 | 0.47320 | 0.48360 | 0.39711 |

Prefix 512 already recovers most of TRECCOVID's lost quality:

- NDCG@10 improves from 0.68205 to 0.77018;
- only 27.949% of documents are touched;
- the gap to row-int8 dense drops from 0.10742 to 0.01929.

Prefix 768 is stronger:

- NDCG@10 improves to 0.78104;
- touch rises to 37.707%;
- the gap to row-int8 dense drops to 0.00843.

## Broad4 Oracle-Mixed Estimate

This is not a promoted method. It is a diagnostic estimate that keeps the
existing Broad4 prefix 256 results for ArguAna, FiQA2018, and SCIDOCS, then
replaces only the known low-coverage TRECCOVID row with the M506c probe.

It answers one question: if a qrels-free dynamic fanout policy could identify
TRECCOVID-like low-coverage queries, how much headroom is available?

| Broad4 Surface | Macro NDCG@10 | Dense O@100 | Cand R@100 | Touch |
| --- | ---: | ---: | ---: | ---: |
| M506b fixed prefix 256 | 0.46254 | 0.72215 | 0.77424 | 0.54764 |
| Oracle-mixed, TRECCOVID prefix 512 | 0.48457 | 0.77395 | 0.82954 | 0.57806 |
| Oracle-mixed, TRECCOVID prefix 768 | 0.48729 | 0.80245 | 0.86264 | 0.60246 |
| Row-int8 dense teacher | 0.50301 | 1.00000 | 1.00000 | 1.00000 |

The oracle-mixed prefix 768 estimate recovers roughly two thirds of the Broad4
remaining quality gap versus M506b prefix 256 while touching about 60% of the
corpus on average.

## Interpretation

M506c does not prove that dataset-specific fanout should be used. It proves a
more important point:

```text
the PPLX-root support head is not the main TRECCOVID blocker;
candidate coverage under fixed fanout is the blocker.
```

The route is still viable. The next step should be a qrels-free dynamic fanout
controller, not another learned scorer replacement.

## Next Step

M506d should implement query-adaptive fanout using only signals available at
index/query time:

- candidate count under the current prefix;
- support-score margin between top candidates and tail candidates;
- support entropy or concentration;
- touched-ratio estimate from posting heads;
- agreement between structural support and dense-tail sketch scores;
- fallback threshold for low-confidence queries.

Promotion gate:

- recover most of the TRECCOVID prefix 512/768 gain;
- do not raise ArguAna-like already-high-touch queries unnecessarily;
- keep the method qrels-free and dataset-name-free;
- then run Broad4 and Broad10 sampled before considering M507 ranking-aware
  fine-tuning.

## Decision

M506c promotes dynamic fanout as the next mainline. It does not promote
dataset-specific tuning, BM25 rescue, learned scorer replacement, or RL.
