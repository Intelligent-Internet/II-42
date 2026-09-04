# SAE M150 C7 Fusion Policy Diagnostic Report

Date: 2026-05-31

## Summary

C7 tested whether the C6 result can be improved without retraining by changing
only the runtime BM25+SAE fusion policy.

The result is mixed but useful:

- A small fixed BM25 boost is better than the C6 learned global fusion scale.
- No fixed BM25 profile dominates standalone SAE on every metric.
- The simple runtime-safe gate did not beat the best fixed profiles under
  hash-fold validation.
- The per-query oracle is materially higher, so there is still policy upside,
  but the current runtime-safe features are not enough to select it reliably.

## Inputs

```text
rankings:
/home/huoju/leask/runs/bm25sae-m150-a1-c6-official-dense-miss-v1-full-corpus-eval/m110_full_corpus_rankings.jsonl

output:
/home/huoju/leask/runs/bm25sae-m150-a1-c7-fusion-policy-v1
```

Local copy:

```text
results/m150-c7-fusion-policy-v1/
```

## Fixed Profiles

| Source | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.1771 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.2408 | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| SAE-only | 0.3149 | 0.3899 | 0.4363 | 0.3120 | 0.2107 |
| C6 learned BM25+SAE | 0.3069 | 0.3841 | 0.4271 | 0.3010 | 0.2008 |
| `rank_boost` (`w0.05_bm25top100`) | 0.3148 | 0.3921 | 0.4258 | 0.3116 | 0.2128 |
| `ndcg_boost` (`w0.1_bm25top100`) | 0.3162 | 0.3915 | 0.4253 | 0.3122 | 0.2125 |
| `recall_boost` (`w0.2_bm25top20`) | 0.3170 | 0.3927 | 0.4210 | 0.3095 | 0.2095 |
| `recall20_boost` (`w0.35_bm25top10`) | 0.3177 | 0.3922 | 0.4197 | 0.3078 | 0.2083 |

Interpretation:

- `rank_boost` is the best fixed MAP profile and improves Recall@100 over
  standalone SAE.
- `recall_boost` is the best fixed Recall@100/Recall@20 profile.
- Standalone SAE remains the best fixed MRR@20 profile.
- C6 learned BM25+SAE is worse than all useful C7 fixed profiles.

## Cross-Validated Runtime Gate

The gate used only runtime-safe features from rankings:

- query length
- BM25/SAE top-k overlap
- BM25 top-score gap
- SAE top-score gap

The selected fold rules were simple threshold gates between SAE and a low BM25
boost profile.

| Source | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 5-fold runtime gate | 0.3140 | 0.3893 | 0.4316 | 0.3113 | 0.2101 |

This is not good enough to promote. It is below `rank_boost` on Recall@100,
NDCG@10, and MAP@100, and below standalone SAE on MRR@20.

## Oracle Upper Bounds

The per-query oracle chooses the best fixed C7 profile with access to qrels.
This is not deployable, but it estimates how much policy upside remains.

| Oracle target | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Recall@20 oracle | 0.3215 | 0.3932 | 0.4366 | 0.3121 | 0.2114 |
| Recall@100 oracle | 0.3150 | 0.3981 | 0.4324 | 0.3116 | 0.2116 |
| MRR@20 oracle | 0.3199 | 0.3924 | 0.4626 | 0.3247 | 0.2231 |
| NDCG@10 oracle | 0.3178 | 0.3911 | 0.4613 | 0.3272 | 0.2237 |
| MAP@100 oracle | 0.3206 | 0.3974 | 0.4604 | 0.3259 | 0.2247 |

The oracle gap is meaningful. It says profile selection can matter, but the
current runtime-safe features are not sufficient.

## Decision

Promote C7 fixed-profile evidence, not the adaptive gate.

Recommended next fixed controls:

| Control | Use |
| --- | --- |
| `rank_boost` | Balanced ranking/MAP profile |
| `recall_boost` | Recall-oriented profile |
| `sae` | MRR-oriented semantic-only baseline |

Do not launch another Stage-C training run from C6 yet. The next useful step is
to run these fixed profiles on the official representative datasets and compare
where each profile wins or loses. If the same pattern holds, the product scorer
should expose a small number of stable BM25 admission policies before learning
a gate.

## Next Step

Run C7 fixed-profile official gates over the same representative datasets used
for C4/C5:

- `fiqa`
- `scidocs`
- `cqadupstack`
- `trec-covid`
- `msmarco`

The key question is whether `rank_boost`/`recall_boost` reduce the official
dense gap without reintroducing the C6 learned-fusion regression.

Implementation note: the official gate is tracked in
`docs/research-sae/reports/m0100-m0199/sae-m150-c7-official-representative-gate-plan.md`. Existing local official
artifacts from `official-beir-mainline-recall-current` are from an older
checkpoint and are only valid as collector smoke tests, not as C6/C7 promotion
evidence.
