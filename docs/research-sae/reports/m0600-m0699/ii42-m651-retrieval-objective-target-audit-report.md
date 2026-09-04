# M651 / Retrieval Objective Target Audit Report

## Status

M651 is a target-design audit for the next first-stage generated-posting
objective.

It consolidates the current evidence from:

- M640-B: first-stage query compiler crossing signal,
- M646: high-Recall but concentrated second-stage teacher,
- M650: broader high-MAP/high-precision second-stage teacher.

The result is not another scorer.  It defines the next objective contract and
prevents the next generated-posting run from blindly mixing incompatible
signals.

Run output:

- `runs/m651_retrieval_objective_target_audit_v1/m651_audit.json`
- `runs/m651_retrieval_objective_target_audit_v1/m651_audit.md`

## M640-B First-Stage Signal

M640-B remains the strongest first-stage query-compiler evidence:

| Metric | All-query delta | Boundary delta |
| --- | ---: | ---: |
| Dense overlap@100 | +0.001302 | +0.000833 |
| MAP@100 | +0.001009 | -0.000343 |
| NDCG@10 | +0.000098 | -0.005952 |
| Recall@100 | +0.001736 | +0.013889 |
| MRR@20 | +0.001046 | +0.000000 |

Interpretation: query-side generated postings can cross top100 and improve
global metrics, but local boundary ranking is still unsafe.

## Second-Stage Teachers

| Source | dRecall@100 | dMAP@100 | promoted | displaced | Role |
| --- | ---: | ---: | ---: | ---: | --- |
| M646 | +0.003856 | +0.000123 | 13 | 7 | recall floor |
| M650 | +0.002162 | +0.000219 | 50 | 37 | precision auxiliary |

M650 improves MAP and promotes many more positives, but it remains below M646
on macro Recall.

## Dataset Signal Conflict

| Signal | Datasets |
| --- | --- |
| Shared positive | nfcorpus |
| M646-only positive | cqadupstack, fiqa |
| M650-only positive | dbpedia-entity, scidocs, trec-covid |
| M650 negative vs M646 | cqadupstack, fiqa, msmarco, nfcorpus |

This is the critical blocker.  M650 cannot be used as the main teacher because
it loses the M646 FiQA/cqadupstack recall shape and introduces MSMARCO loss.

## What This Proves

1. The next breakthrough should stay in first-stage generated postings.
   M640-B is the only route that improves all-query metrics and boundary
   crossing while preserving dense overlap.

2. The objective cannot be single-teacher.
   M646 and M650 are complementary and partially conflicting.

3. M650 should be auxiliary only.
   It provides a high-precision replacement signal, but its dataset distribution
   is not safe as the main target.

4. M646 should define the recall floor.
   Any new generated-posting checkpoint must beat M646 macro Recall before
   promotion.

## M652 Objective Contract

M652 should be a small query-side generated-posting canary.

Scope:

- freeze document postings/index geometry,
- train query-side generated postings only,
- use FiQA/ArguAna canary plus synthetic boundary cases first,
- no BM25, no fixed alpha, no learned gate, no dataset-specific threshold.

Loss terms:

- dense/rank preservation from M637,
- top100 replacement crossing from M640-B,
- M646-style high-recall boundary teacher,
- M650-style high-precision replacement teacher as an auxiliary term only,
- dataset-balanced recall penalty to avoid FiQA/MSMARCO tradeoff.

Hard gates:

- trained checkpoint only; never accept epoch0 fallback,
- dense overlap@100 must not regress beyond tolerance,
- all-query MAP/NDCG/MRR must be non-negative,
- boundary MAP/NDCG must not reproduce the M640 failure,
- macro Recall must beat M646 before promotion,
- explicit rejection if FiQA, cqadupstack, or MSMARCO are lost.

## Next Step

Implement M652 as the smallest canary that encodes this contract.

Do not scale to shared15 or official matrix until M652 shows:

- boundary Recall crossing,
- boundary MAP/NDCG protection,
- dense-overlap safety,
- macro Recall above M646,
- no FiQA/MSMARCO regression.
