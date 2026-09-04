# ii42 M1000 Runtime Milestone And Recall Mainline Status

Date: 2026-06-16

## Current Checkpoint

Worktree:

```text
/Volumes/Betty/Tmp/ii42-m1000/psql_bm25s_sae_m1000
```

Branch:

```text
codex/m1000-sae-bm25-atoms
```

Current runtime head:

```text
2779c321 Explore M1015 bitset membership bounds
```

This document records the current M1000 runtime milestone and then switches
back to the recall-quality mainline. The two surfaces should stay separate:

- M1015 answers whether exact unified-posting traversal can be made cheaper.
- The recall mainline answers whether BM25+SAE can beat dense on full-corpus
  retrieval quality.

## M1015 Runtime Milestone

M1015 is an efficiency milestone, not a quality milestone.

What is now proven:

- The exact final scorer can be preserved while using bitset membership bounds.
- On the `fiqa` q120 exact check, the route kept `120/120` strict top-k parity.
- The current proxy skipped about `72.5%` of block checks.
- Decoded postings dropped to about `0.320` of full traversal.
- Bitset-word reads dropped to about `0.176` of full traversal.

What is not proven:

- M1015 does not improve Recall, MRR, NDCG, or MAP by itself.
- M1015 should not be used to choose the model/scoring route.
- Further block skipping is lower priority until the quality surface is settled.

The role of M1015 is therefore clear: keep it as the runtime backend candidate
for an exact scorer after the scorer/admission route is selected.

## Dense Comparison: Mainline Quality Surface

The strongest comparable full-corpus mixed evaluation rows in this checkout are
from the M160/B12 and M160A C6 diagnostic reports. They are not M1015 runtime
rows.

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 | Delta Versus Dense |
| --- | ---: | ---: | ---: | ---: | --- |
| Dense | 0.3132 | 0.2992 | 0.2251 | 0.1504 | baseline |
| BM25+dense score fusion | 0.3185 | 0.2970 | 0.2256 | 0.1499 | +R, -MRR, +NDCG, -MAP |
| SAE-only | 0.3279 | 0.2922 | 0.2217 | 0.1541 | +0.0147 R, -0.0070 MRR, -0.0034 NDCG, +0.0037 MAP |
| Learned BM25+SAE score fusion | 0.3263 | 0.2966 | 0.2162 | 0.1439 | +0.0131 R, -0.0026 MRR, -0.0089 NDCG, -0.0065 MAP |
| C6 best recall fixed profile: `excess_w0.3_bm25top100` | 0.3370 | 0.2897 | 0.2242 | 0.1576 | +0.0238 R, -0.0095 MRR, -0.0009 NDCG, +0.0072 MAP |
| B12 checkpoint BM25+SAE | 0.3344 | 0.3017 | 0.2225 | 0.1526 | +0.0212 R, +0.0025 MRR, -0.0026 NDCG, +0.0022 MAP |
| B12 NDCG/MAP residual | 0.3322 | 0.2945 | 0.2278 | 0.1586 | +0.0190 R, -0.0047 MRR, +0.0027 NDCG, +0.0082 MAP |

Interpretation:

- BM25+SAE is already competitive with dense on aggregate recall.
- The best robust rows beat dense on Recall@100 and MAP@100.
- B12 checkpoint is the cleanest balanced row because it also beats dense on
  MRR@20.
- The remaining weakness is top-rank stability: NDCG@10 and MRR@20 trade off
  depending on the scoring profile.

## Historical Frontier Row

The strongest historical row remains:

| Row | Recall@100 | MRR@20 | NDCG@10 | MAP@100 |
| --- | ---: | ---: | ---: | ---: |
| M150 A1 C6 official dense-miss BM25+SAE | 0.3841 | 0.4271 | 0.3010 | 0.2008 |

This row is far above dense:

- Recall@100: `+0.0709`
- MRR@20: `+0.1279`
- NDCG@10: `+0.0759`
- MAP@100: `+0.0504`

However, this row should remain a historical frontier until its data lineage is
fully replayed and verified under the current ii42 data/embedding contract. It
is useful as a target shape, not yet as a deployment proof.

## Admission Headroom

The C6 diagnostic shows that the problem is not only representation coverage.
The candidate pool has more relevant documents than the ranked output admits:

| Surface | Recall@100 |
| --- | ---: |
| SAE-only ranked | 0.3279 |
| SAE top100 union BM25 top100 pool | 0.3588 |
| Best post-hoc ranked policy | 0.3370 |

This leaves about `0.0218` Recall@100 headroom between the current best ranked
policy and the simple BM25+SAE candidate pool. That is the most important
quality gap for the next round.

## What Looks Promising

1. **B12-style named scoring profiles.**
   The B12 checkpoint profile is currently the safest balanced route. The
   residual profile beats dense on NDCG@10 and MAP@100 but gives back MRR. These
   should become explicit evaluator/runtime profiles before more model training.

2. **Admission training against full-corpus pools.**
   The next training target should focus on BM25-only relevant hits,
   SAE-positive hits lost by fusion, and high-BM25 false positives. The loss
   should optimize final top-100 admission and top-20 ordering, not generic SAE
   atom imitation.

3. **Dataset-agnostic adaptive scoring.**
   Adaptive policy is still plausible, but only with runtime-safe query/source
   features. It must not use dataset ids or benchmark-specific profiles.

4. **M1000/M1015 runtime work as backend, not scorer.**
   Once the quality scorer is chosen, M1015 can make exact traversal cheaper.
   It should not drive recall decisions.

## What To Avoid Next

- Do not continue block-WAND/MaxScore work expecting quality improvement.
- Do not run more generic fixed-weight fusion sweeps unless they are attached to
  official full-corpus profile validation.
- Do not use M1000 patch/segment pooling as the next quality bet; earlier
  direction-setting evidence showed it below the global/unified path.
- Do not promote the historical M150 frontier row without replaying its data
  lineage under the current ii42 contract.

## Recommended Next Step

Return to the recall mainline with a narrow quality objective:

1. Freeze M1015 as the runtime milestone for exact traversal.
2. Run official/full-corpus per-dataset matrix for these named profiles:
   `SAE-only`, `B12 checkpoint BM25+SAE`, `B12 residual`, and
   `C6 excess_w0.3_bm25top100`.
3. If the same top-rank gap remains, train only the admission/scoring head on
   full-corpus pools with hard negatives and BM25-only positives.
4. Reuse M1015 traversal only after the scoring profile is chosen.

