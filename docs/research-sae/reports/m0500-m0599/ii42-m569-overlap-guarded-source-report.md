# M569 Overlap-Guarded M551 Source Report

M569 is the production-shaped successor to the M568 conservative overlap gate.
It does not retrain M551.  It turns the M568 post-hoc analysis into explicit
guarded result artifacts by adding a new source to each M566 result JSON:

`m569_overlap_guarded_m551`

For each task and seed, this source uses the learned M551-family source only
when its `dense_overlap_at_100` is not lower than `dense_topk128_sparse`.
Otherwise, it falls back to `dense_topk128_sparse` for that task.

The decision signal is qrels-free.  Qrels metrics are only aggregated after the
source has been selected.  This still uses the evaluated query surface, so the
next deployment-clean step should move the same gate to an independent
unlabeled selector split.

## Artifacts

- Script:
  `scripts/research_sae_m569_overlap_guarded_source.py`
- Summary JSON:
  `outputs/m569_overlap_guarded_source/m569_overlap_guarded_source.json`
- Summary Markdown:
  `outputs/m569_overlap_guarded_source/m569_overlap_guarded_source.md`
- Guarded seed JSONs:
  `outputs/m569_overlap_guarded_source/guarded_results/`

## Result

M569 was run over the M566 official-1024 BEIR7 three-seed surface.

| Source | dNDCG@10 | dMAP@100 | dRecall@100 | dMRR@20 | dOverlap@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| M551 learned | +0.002207 | +0.002997 | +0.003792 | -0.001533 | +0.001149 |
| M569 guarded | +0.003528 | +0.003074 | +0.002937 | +0.000899 | +0.001982 |

Gate summary:

- accepted task-seed cells: `14`
- fallback task-seed cells: `7`
- fallback cells with learned MRR regression: `4`
- fallback cells with learned NDCG regression: `4`

## Interpretation

The DREAM-inspired M551 line is still alive, but the stable shape is not
"always use the learned residual."  The stable shape is:

1. train with frozen dense teacher and candidate-set competition;
2. keep output edits inside dense-derived support;
3. select the learned source only when a qrels-free teacher-surface signal says
   it did not damage dense overlap.

M569 is materially better than continuing pairwise micro-sweeps.  M567 pairwise
slightly moved metrics but did not repair the bad-seed MRR failure.  M569
directly fixes the mean MRR sign while preserving positive NDCG, MAP, Recall,
and overlap.

## Next Gate

The next step should be M570:

- create an independent unlabeled selector split per task;
- compute the same dense-overlap decision on that selector split before
  held-out qrels evaluation;
- evaluate the selected source on official-1024 BEIR7 first;
- add `cqadupstack` only after BEIR7 stays positive, because it dominates
  query count and runtime.

If M570 keeps the M569 profile without using the evaluation query surface for
source selection, this line becomes a real candidate for BEIR8 and then later
BM25-fusion work.  If it collapses, M551/M568 should be treated as a useful
diagnostic but not a final encoder/posting route.
