# SAE M150 C6 Post-Hoc Fusion Diagnostic Report

Date: 2026-05-31

## Summary

C6 completed successfully. The low-cost post-hoc diagnostic re-scored the
existing C6 full-corpus rankings without retraining, rerunning dense inference,
or rebuilding indexes.

The result is clear: the C6 representation is not the main bottleneck on the
M130 continuity surface. Standalone SAE already beats dense by a large margin.
The problem is the final BM25+SAE fusion shape. The learned C6 BM25 scale is
too strong, and BM25 can push lexical high-score negatives above semantic
positives that SAE already retrieves.

## Inputs

```text
rankings:
/home/huoju/leask/runs/bm25sae-m150-a1-c6-official-dense-miss-v1-full-corpus-eval/m110_full_corpus_rankings.jsonl

diagnostic output:
/home/huoju/leask/runs/bm25sae-m150-a1-c6-official-dense-miss-v1-posthoc-fusion-diagnostic
```

Local copy:

```text
results/m150-c6-posthoc-fusion-diagnostic/
```

The diagnostic sweeps:

- BM25 score weight: `0.0, 0.05, 0.1, 0.2, 0.35, 0.5, 0.75, 1.0`
- BM25 candidate admission: top `0, 5, 10, 20, 50, 100`
- `bm25top0` means BM25 can only re-rank SAE candidates; it cannot introduce
  BM25-only documents into the final candidate set.

## Full-Corpus Baseline

| Source | Recall@20 | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: | ---: |
| BM25 | 0.1771 | 0.2439 | 0.2211 | 0.1542 | 0.0965 |
| Dense | 0.2408 | 0.3132 | 0.2992 | 0.2251 | 0.1504 |
| BM25+dense score fusion | 0.2397 | 0.3161 | 0.2897 | 0.2153 | 0.1421 |
| SAE-only | 0.3149 | 0.3899 | 0.4363 | 0.3120 | 0.2107 |
| C6 learned BM25+SAE score fusion | 0.3069 | 0.3841 | 0.4271 | 0.3010 | 0.2008 |

The important observation is that `SAE-only` beats the C6 learned
`BM25+SAE score fusion` on every tracked metric. This should not happen if the
main issue were atom compression. It points to fusion/ranking calibration.

## Best Post-Hoc Sweep Points

| Metric | Best source | Value |
| --- | --- | ---: |
| Recall@20 | `w0.35_bm25top10` | 0.3180 |
| Recall@100 | `w0.2_bm25top20` | 0.3928 |
| MRR@20 | `sae` | 0.4363 |
| NDCG@10 | `w0.1_bm25top100` | 0.3122 |
| MAP@100 | `w0.05_bm25top100` | 0.2128 |

The best fusion points use much smaller BM25 weights than the learned C6 final
scale. MRR@20 remains strongest with standalone SAE, which means BM25
admission is useful for recall/MAP but still risky for top-rank order. The C6
checkpoint learned approximately:

```text
scale_sae  = 1.3203
scale_bm25 = 0.7635
```

In post-hoc terms, the best quality region is closer to a BM25 weight of
`0.05-0.20`, not `0.58` relative to SAE.

## Dataset Signals

| Dataset | Dense R@100 | SAE R@100 | C6 learned BM25+SAE R@100 | Best diagnostic signal |
| --- | ---: | ---: | ---: | --- |
| arguana | 1.0000 | 1.0000 | 1.0000 | fusion choice is mostly ranking-only |
| fiqa | 0.7792 | 0.6161 | 0.5821 | representation gap remains; BM25 hurts |
| msmarco | 0.7349 | 0.6862 | 0.7297 | BM25 helps recall but can hurt ranking |
| nfcorpus | 0.1925 | 0.2035 | 0.2069 | BM25+SAE is useful |
| scifact | 0.8000 | 0.9333 | 1.0000 | BM25+SAE can help strongly |
| trec-covid | 0.2599 | 0.2124 | 0.2579 | BM25 recovers recall but ranking is noisy |

The problem is not uniform. Some datasets want BM25 admission, while others
want SAE-dominant ranking. A single learned global BM25 scale is too blunt.

## Interpretation

C6 answered the first low-cost question:

```text
Are atoms too compressed?
```

On this continuity surface, probably not. SAE-only is stronger than dense and
stronger than learned BM25+SAE fusion.

C6 instead exposes a fusion policy problem:

1. BM25 is useful as selective candidate expansion on datasets like `scifact`,
   `nfcorpus`, and `msmarco`.
2. BM25 is harmful when admitted too broadly or weighted too strongly, because
   lexical hard negatives can displace semantic positives already retrieved by
   SAE.
3. The current Stage-C loss allows the model to optimize top-rank confidence
   and MRR while giving up some top-k semantic coverage.
4. The final scorer should treat BM25 as a controlled precision/expansion
   signal, not as a peer signal that always competes with SAE.

## Decision

Do not continue blind C6-style Stage-C training.

The next step should be a fusion-policy redesign:

1. Promote SAE as the primary semantic candidate/ranking signal.
2. Add BM25 as a small calibrated boost or limited candidate admission path.
3. Use BM25 admission caps or query-adaptive gates rather than one global
   learned BM25 scale.
4. Make Stage-C selection optimize Recall@100/Recall@20 first, then use
   MRR/NDCG/MAP as tie-breakers.
5. Only after this post-hoc policy is stable should another Stage-C training
   run be launched.

## Immediate Next Experiment

Run a C7 post-hoc scorer before training:

```text
score = SAE_norm + alpha(query) * BM25_norm
```

Candidate policy:

- Always keep SAE top-100.
- Admit only BM25 top `10-20` by default.
- Let BM25 top `50-100` in only when query-level diagnostics predict lexical
  intent.

Initial fixed controls:

| Profile | Purpose |
| --- | --- |
| `w0.05_bm25top100` | best ranking/MAP diagnostic point |
| `w0.1_bm25top100` | best NDCG diagnostic point |
| `w0.2_bm25top20` | best Recall@100 diagnostic point |
| `w0.35_bm25top10` | best Recall@20 diagnostic point |

If a simple fixed profile already beats SAE-only and C6 learned fusion, update
deployment scoring first. If not, build a small query-adaptive gate using
runtime-safe features.
