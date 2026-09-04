# ii42 M338 Learned Admission Report

## Summary

M338 starts from the M337 diagnosis: the deep BM25/atom score maps often contain
many qrel-positive documents, but the fixed top160 admission policy discards too
many of them. M338 tests whether a compact learned admission policy can select
a better top100 candidate pool from those deep score maps.

This experiment does not rebuild encoder embeddings or posting caches. It uses
the existing M334/M335 candidate caches and trains only a small score-map
admission scorer.

## Contract

- Script: `scripts/research_sae_m338_learned_admission.py`
- Candidate cache:
  `/home/huoju/leask/runs/ii42-m334-interaction-candidate-cache-v1`
- Variant: `df_le_0p25`
- Source pool: top `1000` from BM25, atom, unified scale `0.25`, and unified
  scale `0.5`
- Admission budget: top `100`
- Seed / split seed: `1050` / `1050`

## Smoke Result

Initial smoke on `nfcorpus + scifact` completed successfully:

- JSON:
  `/home/huoju/leask/runs/ii42-m338-learned-admission-smoke-v1/m338_smoke_nfcorpus_scifact_k1000.json`
- Local copy:
  `/tmp/m338_smoke_nfcorpus_scifact_k1000.json`

| Method | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M338 anchor | 0.5626 | 0.5225 | 0.4484 | 0.3463 |
| M338 learned admission | 0.5624 | 0.5230 | 0.4450 | 0.3431 |
| M338 candidate upper bound | 0.7370 | 0.9360 | 0.8658 | 0.7370 |

The smoke proves that the deep score-map admission surface is useful, but the
first learned residual does not beat the simple anchor. That is a useful
negative signal: the immediate win is deeper admission, while the learned
policy needs a sharper loss or feature set before it can add value over the
anchor.

## Broad-8 Result

The broad-8 K1000 run completed on spark-1:

- Log:
  `/home/huoju/leask/runs/ii42-m338-learned-admission-broad8-v1/m338_broad8_k1000.log`
- JSON:
  `/home/huoju/leask/runs/ii42-m338-learned-admission-broad8-v1/m338_broad8_k1000_seed1050.json`
- Local copy:
  `/tmp/m338_broad8_k1000_seed1050.json`

Heldout aggregate:

| Method | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M338 anchor | 0.5478 | 0.3071 | 0.2908 | 0.2474 |
| M338 learned admission | 0.5473 | 0.3046 | 0.2903 | 0.2464 |
| M338 candidate upper bound | 0.6456 | 0.7360 | 0.6800 | 0.6456 |

Per-dataset heldout recall and top-rank signal:

| Dataset | Anchor R@100 | Learned R@100 | Upper R@100 | Anchor NDCG@10 | Learned NDCG@10 | Upper NDCG@10 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| arguana | 0.9143 | 0.9357 | 0.9976 | 0.2850 | 0.3094 | 0.9976 |
| cqadupstack | 0.5276 | 0.5247 | 0.5927 | 0.2905 | 0.2871 | 0.6088 |
| fiqa | 0.5326 | 0.5326 | 0.7880 | 0.2758 | 0.2760 | 0.8181 |
| nfcorpus | 0.2601 | 0.2496 | 0.4996 | 0.3019 | 0.2960 | 0.7615 |
| scidocs | 0.3513 | 0.3513 | 0.6174 | 0.1514 | 0.1518 | 0.7037 |
| scifact | 0.8552 | 0.8552 | 0.9667 | 0.5901 | 0.5904 | 0.9667 |
| trec-covid | 0.0798 | 0.0800 | 0.2174 | 0.5074 | 0.5004 | 1.0000 |
| webis-touche2020 | 0.5071 | 0.5071 | 0.8248 | 0.2927 | 0.2944 | 0.9850 |

## Interpretation

M338 should be judged in two layers:

- The deep score-map pool still has substantial headroom. Heldout candidate
  upper bound is `0.6456 / 0.7360 / 0.6800 / 0.6456`, and several datasets
  have very high upper-bound top-rank signal.
- The learned admission head is not promotable as a direct replacement. It is
  almost identical to the anchor on recall and slightly worse on MRR/NDCG/MAP.
- Compared with the M335 qidfix broad result, this direct-admission route is
  much weaker. M335 reached `0.8000 / 0.4370 / 0.4238 / 0.3700`, so M338 should
  not replace the interaction scorer.

## Next Step

Do not continue by simply training this M338 admission head deeper. The result
shows that the compact score-map policy is not expressive enough to be the
final scorer.

The cleaner M339 direction is a two-stage route:

- use the M338/M337 deep score-map pool as a recall-expansion admission layer;
- feed the expanded candidate set into the existing M335 interaction-feature
  scorer;
- evaluate whether the M335 scorer can recover its strong ranking quality while
  benefiting from the deeper candidate pool.

Promotion rule: if deep-pool interaction reranking cannot recover the M335
qidfix broad quality while improving candidate recall, stop this branch and
return to feature/encoder-side changes.
