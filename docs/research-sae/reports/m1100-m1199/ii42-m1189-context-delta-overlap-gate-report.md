# M1189 Context Delta Overlap Gate

## Purpose

M1188 found the first full shared15 qrels-free positive signal for
retrieval-conditioned query-side posting deltas.

Two BM25-top variants mattered:

- `bm25_top_docs3_append8_s0.1_shared1`: larger ranking gains, but tiny CUB
  regression.
- `bm25_top_docs1_append8_s0.1_shared1`: smaller gains, but all macro metrics
  positive.

M1189 tests whether a deployable qrels-free top-k overlap gate can keep most of
the aggressive docs3 gains while falling back when the mutated query perturbs
the baseline head too much.

The gate uses no qrels and no dataset labels.  It compares baseline top95 with
aggressive-mutated top95, then either accepts aggressive or falls back to
conservative/baseline.

## Artifacts

- Script: `scripts/audit_m1189_context_delta_overlap_gate.py`
- Full run JSON:
  `runs/m1189_context_delta_overlap_gate_v1/m1189_context_delta_overlap_gate.json`
- Full run Markdown:
  `runs/m1189_context_delta_overlap_gate_v1/m1189_context_delta_overlap_gate.md`
- Smoke run:
  `runs/m1189_context_delta_overlap_gate_smoke_v1/`

## Full Shared15 Macro

| Variant | Take | Top95 | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `m674` | 0.000 | 1.000 | +0.005323 | +0.000341 | +0.000000 | +0.000000 | +0.000000 |
| `aggressive` | 1.000 | 0.970 | +0.001947 | +0.003708 | +0.003271 | +0.002148 | -0.000082 |
| `conservative` | 1.000 | 0.985 | +0.000311 | +0.002182 | +0.001611 | +0.002424 | +0.000017 |
| `gate aggressive->conservative t0.95` | 0.821 | 0.975 | +0.000525 | +0.002898 | +0.001709 | +0.001739 | +0.000166 |
| `gate aggressive->conservative t0.97` | 0.467 | 0.982 | +0.000593 | +0.002481 | +0.001557 | +0.001888 | +0.000032 |
| `gate aggressive->baseline t0.97` | 0.467 | 0.993 | +0.000373 | +0.000976 | +0.000850 | +0.000330 | +0.000019 |

## Interpretation

The simple overlap gate is useful as a safety diagnostic, but it is not the
right control policy.

Positive:

- It can remove the macro CUB regression from aggressive docs3.
- It confirms that query-time qrels-free safety controls are feasible in the
  native path.
- `gate aggressive->conservative t0.95` keeps MAP/NDCG/MRR positive while
  making CUB positive.

Negative:

- It eats most of the aggressive recall gain.
- It does not fix cqadupstack row harm; cqadupstack remains negative under both
  aggressive and gated variants.
- The top-scored variant is still ungated aggressive, so top-k overlap alone is
  not predictive enough.

## Row-Level Notes

The largest unresolved issue is not global head disruption; it is
surface-specific response to BM25-top atom injection.

- `cqadupstack`: aggressive hurts Recall/MAP/NDCG/MRR despite positive CUB.
  Overlap gate does not identify this row as risky.
- `dbpedia-entity`, `scidocs`, `nfcorpus`: some CUB/row tradeoffs remain.
- `hotpotqa`, `arguana`, `scifact`, `climate-fever`: aggressive or
  near-aggressive movement is mostly useful.

This means the next gate must use richer qrels-free risk features than
baseline-vs-mutated top95 overlap.

## Decision

Do not scale a plain overlap-gated policy.

Keep M1188/M1189 as real positive structural evidence:

1. Qrels-free retrieval-conditioned posting deltas can improve full shared15
   native ranking metrics.
2. Conservative deltas already produce all-positive macro movement.
3. Aggressive deltas produce larger ranking gains, but require a stronger
   qrels-free safety model.

## Next Step

M1190 should audit and train a qrels-free risk-feature gate.

Candidate features:

- baseline/aggressive top overlap at multiple cutoffs: 10, 20, 50, 95, 100
- aggressive changed-doc count inside protected head
- proposal BM25/P1/fused ranks and score margins
- source-count patterns from native hybrid rows
- query atom count and perturbation atom count
- overlap between proposal atoms and existing query atoms
- BM25 lexical coverage of proposal docs

Acceptance target:

- Preserve at least half of aggressive MAP/NDCG/MRR gains.
- Keep macro CUB non-negative.
- Reduce cqadupstack-style row harm without dataset labels.
- If only qrels-trained or dataset-specific gates work, stop and treat this as
  a policy/reranking problem rather than a unified posting compiler control.
