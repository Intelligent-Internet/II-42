# M692 Recall-Bearing Atom Proposal Report

## Objective

M692 tests whether the M691 retrieval-teacher oracle can be distilled into an
inference-compatible atom proposal model. This directly evaluates the suggested
route shift:

- keep the direct/unified posting engineering form;
- do not return to traditional SAE reconstruction loss;
- freeze the dense-root/native index geometry as the floor;
- train a retrieval-constrained generated-posting compiler only if it can cross
  top100 boundaries without breaking the dense/P1 head.

## Inputs

- Teacher/oracle source:
  `runs/m691_retrieval_teacher_proposal_expansion_all_v1/m691_summary.json`
- Native evaluation surface: local PostgreSQL shared15 native P1/BM25 path
- Target rows:
  - `rank_safe_accepted`: all M691 rank-safe accepted rows
  - `rank_safe_recall`: only M691 rank-safe accepted rows with Recall@100 lift

## Target Density

| Target | Rows | Target rows | Train target rows | Holdout target rows | Atom rows | Positive atom rows | Atom AUC | Visible target atoms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `rank_safe_accepted` | 1342 | 108 | 84 | 24 | 70669 | 397 | 0.776302 | 397 / 1010 |
| `rank_safe_recall` | 1342 | 29 | 24 | 5 | 70420 | 148 | 0.807309 | 148 / 381 |

The recall-bearing target is extremely sparse: only 24 train queries and 148
visible positive atom rows. The model can classify these atoms above chance, but
the proposal source exposes less than half of the reconstructed target atoms.

## Full Shared15 Results

Baseline is the current P1 native shared15 surface.

| Run | Source | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB | Top95 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| main accepted | atom all | -0.000039 | +0.000076 | -0.000032 | +0.000650 | -0.000283 | 0.985928 |
| main accepted | atom gate | +0.000032 | +0.000006 | -0.000042 | +0.000036 | -0.000027 | 0.999278 |
| main recall | atom all | +0.000158 | +0.000022 | +0.000152 | +0.000673 | -0.000067 | 0.985920 |
| main recall | atom gate | +0.000000 | -0.000001 | +0.000000 | +0.000000 | +0.000000 | 1.000000 |
| conservative recall | atom all | +0.000035 | -0.000005 | -0.000151 | -0.000052 | -0.000148 | 0.995184 |
| ultra recall | atom all | -0.000187 | -0.000022 | +0.000118 | -0.000060 | -0.000181 | 0.998133 |
| M674 reference | native BM25 rescue | +0.005323 | +0.000341 | +0.000000 | +0.000000 | +0.000000 | 1.000000 |

## Holdout Results

| Run | Source | dRecall@100 | dMAP@100 | dNDCG@10 | dMRR@20 | dCUB | Top95 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| main accepted | atom all | +0.001013 | -0.000031 | +0.000223 | +0.000506 | -0.000131 | 0.986626 |
| main accepted | atom gate | +0.000000 | +0.000024 | -0.000112 | +0.000000 | +0.000007 | 0.999094 |
| main recall | atom all | +0.001064 | +0.000078 | +0.000261 | +0.000260 | -0.000098 | 0.986152 |
| main recall | atom gate | +0.000000 | +0.000000 | +0.000000 | +0.000000 | +0.000000 | 1.000000 |
| conservative recall | atom all | +0.000137 | +0.000243 | +0.000098 | -0.000137 | -0.000281 | 0.994780 |
| ultra recall | atom all | -0.000820 | +0.000051 | +0.000009 | -0.000137 | +0.000000 | 0.998059 |
| M674 reference | native BM25 rescue | +0.003747 | +0.000190 | +0.000000 | +0.000000 | +0.000000 | 1.000000 |

## Findings

1. The M691 retrieval-teacher target is learnable but not safely deployable with
   the current atom proposal source. `rank_safe_recall` has atom AUC `0.807309`
   and positive holdout Recall/MAP/NDCG/MRR in the main setting, but it breaks
   top95 head preservation and slightly lowers candidate upper bound.

2. Lowering update strength improves top95 but removes the useful retrieval
   effect. The conservative run improves top95 from roughly `0.986` to `0.995`
   while keeping only a small holdout Recall/MAP lift; the ultra-conservative
   run reaches top95 `0.998059` on holdout but loses Recall.

3. The learned gate preserves the head but mostly nulls the update. For
   `rank_safe_recall`, the gate applies to `0%` of holdout queries and therefore
   cannot recover recall-bearing rows. This matches the M687/M688 conclusion:
   more gate tuning over the same proposal surface is not the bottleneck.

4. The root blocker is proposal-source/teacher coverage, not SAE loss. Only
   `148 / 381` recall-target atoms are visible in the current inference proposal
   pool. The model cannot reliably reproduce the M691 oracle when most target
   atoms are absent before classification.

## Decision

Reject M692 as a deployable compiler stage. It does not produce a strict
support-safe Recall/MAP lift through the native path.

Keep M692 as a diagnostic milestone. It confirms the current route diagnosis:
direct/unified posting remains the right engineering form, but the next
breakthrough needs a larger retrieval-constrained teacher and a richer
support-aware atom source. Returning to traditional SAE reconstruction is not
supported by this result.

## Next Step

Start M693 as corpus-derived retrieval teacher expansion:

1. Generate many more pseudo-positive query/doc events from corpus, BM25/entity,
   and dense-miss/qrels-positive surfaces.
2. Keep the dense/P1 head as a hard floor: top95, CUB, NDCG/MRR guard.
3. Build atom proposal rows from the positive teacher documents directly, not
   only from the current native tail pool.
4. Train the generated-posting compiler on the expanded teacher, then validate
   through the same native shared15 path.
5. Stop if expanded teacher still cannot produce visible target atoms or if
   safe updates collapse to zero after the head guard.
