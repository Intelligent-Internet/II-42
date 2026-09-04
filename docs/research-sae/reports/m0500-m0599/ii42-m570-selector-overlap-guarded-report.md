# M570 Selector-Split Overlap Guard

M570 moves the M569 overlap gate toward a deployment-clean shape.  M569 used
the evaluated query surface for the qrels-free overlap decision.  M570 changes
the runner so source selection can be decided on an independent unlabeled
selector split before held-out qrels metrics are aggregated.

This does not change M551 training loss.  It only adds:

- `--selector-query-fraction`
- `--emit-overlap-guarded-source`
- `--guarded-source-name`
- `--overlap-gate-min-delta`

The Spark runner exposes the same controls through:

- `SELECTOR_QUERY_FRACTION`
- `EMIT_OVERLAP_GUARDED_SOURCE`
- `GUARDED_SOURCE_NAME`
- `OVERLAP_GATE_MIN_DELTA`

## Smoke

Smoke run:

- host: `spark-1`
- root:
  `/home/huoju/leask/runs/ii42-m566-m551-official1024-root-v1/_shared/tasks`
- task: `scifact`
- seed: `570`
- train/selector/eval split: `0.50 / 0.20 / 0.30`
- train groups: `48`
- epochs: `1`

Local synced output:

- `runs/m570_selector_smoke_scifact_seed570/m570_selector_smoke_scifact_seed570.json`
- `runs/m570_selector_smoke_scifact_seed570/m570_selector_smoke_scifact_seed570.md`
- `runs/m570_selector_smoke_scifact_seed570/m570_selector_smoke_scifact_seed570.log`

Smoke evidence:

- split: `train_selector_eval_canary`
- selector queries: `60`
- added source: `m570_selector_overlap_guarded_m551`
- baseline selector overlap@100: `0.531667`
- learned selector overlap@100: `0.538333`
- gate decision: accepted learned source

This proves the new runner surface can write a guarded source using an
independent qrels-free selector split.

## Formal Gate

The first formal seed used the M566 official-1024 BEIR7 surface:

- tasks: `arguana,nfcorpus,fiqa,scidocs,scifact,trec-covid,webis-touche2020`
- train/selector/eval split: `0.50 / 0.20 / 0.30`
- M551 promoted settings:
  `ACTIVE_DIMS=128`, `TEACHER_POOL_K=128`, `TEACHER_TEMPERATURE=0.025`,
  `STUDENT_TEMPERATURE=0.050`, `RANDOM_NEGATIVES=128`,
  `MAX_TRAIN_GROUPS=512`, `EPOCHS=4`, `HIDDEN_DIMS=384`,
  `RESIDUAL_SCALE=0.025`, `SUPPORT_WEIGHT=0.5`, `SCORE_WEIGHT=0.05`

Seed551 output:

- `runs/m570_selector_beir7_seed551/m570_selector_beir7_seed551.json`
- `runs/m570_selector_beir7_seed551/m570_selector_beir7_seed551.md`

Seed551 deltas versus `dense_topk128_sparse`:

| Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| learned M551 | +0.006360 | +0.003820 | -0.000630 | +0.010660 | +0.001330 |
| M570 guarded | +0.005710 | +0.003470 | -0.000630 | +0.010330 | +0.001170 |

Gate decisions:

| Task | Selected Source | Selector dOverlap@100 |
| --- | --- | ---: |
| arguana | dense_topk128_sparse | -0.001250 |
| nfcorpus | learned | +0.006308 |
| fiqa | learned | +0.001538 |
| scidocs | learned | +0.001850 |
| scifact | learned | +0.006500 |
| trec-covid | learned | +0.010000 |
| webis-touche2020 | learned | +0.000000 |

Interpretation of seed551: selector-overlap gating preserves the strong
NDCG/MAP/MRR signal, but it does not protect Recall@100.  The next revision
should add a second qrels-free selector constraint for coverage or active
candidate recall.  Pure selector overlap is not enough for promotion.

Promotion condition:

- M570 guarded source must keep positive mean NDCG@10, MAP@100, Recall@100,
  MRR@20, and dense overlap versus `dense_topk128_sparse`.
- If BEIR7 passes on at least two seeds, add `cqadupstack` for BEIR8.
- If M570 loses the M569 profile, treat M569 as a diagnostic result rather
  than a final encoder/posting route.
