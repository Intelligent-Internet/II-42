# M553 Frozen LM Judge Posting Encoder Plan

M553 is the next DREAM-derived test after M551 and M552.

M551 result: candidate-set/listwise competition against a frozen dense teacher
is useful and promoted as the current BM25-free first-stage milestone.

M552 result: a cheap dense target-doc proxy did not beat M551, even at low
judge weights.  This says the target-passage shortcut is too weak; it does not
disprove DREAM.

## Feasibility Check

`spark-2` can run a frozen causal LM judge in Docker using the existing
`/deps` transformers install and a writable cache at:

```text
/home/huoju/leask/runs/hf-cache
```

Probe model:

```text
distilgpt2
```

Probe objective:

```text
loss(query tokens | document prefix + "Query:")
```

Probe result:

| Prefix document | Query loss |
| --- | ---: |
| Rainy-day funds should be kept liquid and safe. | 4.9009 |
| Unrelated sports/weather paragraph. | 5.6322 |

Interpretation: the frozen LM signal is directionally sane on a simple pair.
It is much closer to DREAM's next-token supervision than the M552 target-doc
dense proxy.

## Proposed M553 Smoke

Build a small qrels-free training target generator:

1. Use M551's dense teacher to build a compact candidate set.
2. For each query-candidate pair, compute frozen LM query-token loss conditioned
   on candidate document text.
3. Convert negative LM loss to a candidate-set target distribution.
4. Blend lightly with dense teacher distribution only as a stabilizer.
5. Train the same locked-support residual posting encoder.
6. Evaluate with held-out qrels only after training, same as M551/M552.

Initial constraints:

- Tasks: `FiQA2018,SCIDOCS,TRECCOVID`
- `teacher_pool_k`: 32 or 64, not 128, to keep LM scoring bounded.
- `random_negatives`: 32 or 64.
- `max_train_groups`: 64 first, then 128 if the signal is non-degenerate.
- Frozen LM: `distilgpt2` for feasibility; upgrade only if it beats M551 smoke
  or shows strong diagnostics.

Promotion rule:

- Must beat M551 promoted seed552 on the same smoke tasks, or at least improve
  a key metric without MAP/Recall collapse.
- If it loses like M552, stop this cheap-LM route and only revisit with a
  stronger cached/instruct LLM judge.

## Completed Smoke Summary

Results are recorded in:

```text
docs/research-sae/reports/m0500-m0599/ii42-m553-frozen-lm-judge-report.md
```

The tested frozen-LM likelihood routes did not beat M551:

- `distilgpt2`, `JUDGE_WEIGHT=0.25`: near-tied NDCG but weaker Recall, MRR,
  and dense-overlap.
- `distilgpt2`, `JUDGE_WEIGHT=0.05`: weaker than M551 on every tracked metric.
- `Qwen/Qwen2.5-0.5B-Instruct`, `JUDGE_WEIGHT=0.25`: worse than M551 and
  slightly below `dense_topk128_sparse` on NDCG@10.

Conclusion: keep M551 as the promoted BM25-free first-stage milestone. Keep
M553 as negative evidence and infrastructure, but do not scale the cheap
query-likelihood blend route.
