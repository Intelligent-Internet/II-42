# M740 Spark Remote Route Synthesis

This report summarizes remote-only reports found on `spark-1` / `spark-2`
and maps them back to the current local M738/M739 state.

## Remote Inventory

`spark-1` has recent reports not present in the local checkout:

- M636 / M637: first-stage query compiler smokes.
- M638 / M639 / M640: boundary-positive and rank-safe boundary query attempts.
- M652 / M653: multi-teacher and support-safe deterministic query deltas.
- M654 / M655 / M656 / M657: coordinate feasibility and masked coordinate/head
  experiments.
- M666 / M667 / M668: per-positive rank audit and boundary linear reranker.

`spark-2` has only older DREAM / M549U reports in the checked path and no
newer useful reports in the recent run scan.

## Useful Signals

### First-stage blind query compiler remains negative

M636, M637, M638, M639, M640, M652, and M653 all failed their smoke or support
safe gates.  The repeated pattern is:

- trained checkpoints either are not selected or fail recall-positive gates;
- boundary positives exist, but learned query-side movement does not cross
  top100 safely;
- deterministic support-safe deltas accept zero boundary updates in M653.

This supports the current M738/M739 conclusion: the local query-side atom
selection surface is not merely undertrained.  The available blind compiler
interface is missing a deployable signal.

### Coordinate-level oracle is real but not transferred

M654 is the most important first-stage positive signal:

- status: `coordinate_feasible`
- test boundary `Recall@100`: `+0.014430`
- test boundary `MAP@100`: `+0.002902`
- test boundary `NDCG@10`: `+0.004444`
- test all `Recall@100`: `+0.001860`
- dense overlap and CUB stayed safe.

This proves safe movement exists at coordinate level.  However M655/M656/M657
show that freezing a global mask or training a masked head does not transfer:

- M655 global mask lost boundary overlap and did not gain recall.
- M656/M657 trained masked heads improved some all-row MAP/NDCG but lost
  dense overlap and did not recover boundary recall.

So the route is not impossible; the current mechanism for discovering and
deploying the coordinate mask is too weak.

### The strongest practical signal is second-stage boundary scoring

M666 exported per-positive ranks and separated the failure modes:

- positives audited: `12455`
- `present_below_top100`: `4272`
- `stable_top100`: `2484`
- `candidate_miss_both`: `5694`
- top100 delta from M661 was only `+2`, CUB delta `-1`.

This means there is substantial top100 ranking headroom inside an existing
candidate set, but M661-style first-stage movement barely captures it.

M667/M668 then trained a fixed-candidate global linear reranker:

- M667 test `Recall@100`: `+0.001990`
- M668 stricter variant test `Recall@100`: `+0.003259`
- CUB unchanged.
- MAP slightly positive.
- NDCG/MRR unchanged.

The cost is large dense-overlap loss (`O@100` around `-0.045` to `-0.062`),
so this is not a final dense-equivalence-preserving model.  But it is a strong
diagnostic: the bottleneck is rank boundary scoring, and a very simple scorer
can recover some of it.

## Relation To Local M738/M739

Local M738/M739 found:

- M738 canary query-internal ranker had a small holdout-positive signal.
- M738 failed to expand because M735B productive selector failed on shared8.
- M739 bypassed M735B and learned utility ordering, but native policies still
  failed strict gate.

Remote M654-M668 explains why:

- safe coordinate movement exists but is sparse and query-local;
- global masks and shallow heads do not transfer;
- ranking recovery is easier to learn than first-stage generated posting
  movement;
- dense-equivalence and recall recovery are currently in tension.

## Route Decision

Do not keep looping on blind first-stage atom/ranker variants.

The best next route is structural:

1. Keep P1/M549U native unified posting as the frozen substrate.
2. Treat first-stage movement as a sparse coordinate-oracle discovery problem,
   not as a generic query compiler problem.
3. Build a retrieval-conditioned boundary scorer that predicts when a
   coordinate/posting movement is safe enough to cross top100.
4. Use M667/M668-style rank-boundary features as training signal, but add hard
   dense-overlap/CUB constraints so it cannot become a loose reranker.
5. Only after that, convert accepted boundary movement back into native
   unified-posting query deltas.

In short: M654 says the coordinate movement exists; M667 says the boundary
ranking signal is learnable; M738/M739 says the current atom selector cannot
find it reliably.  The next design should combine coordinate-level movement
with boundary-ranker supervision, not train another blind query compiler.

## Proposed Next Milestone

M741: boundary-supervised coordinate compiler.

Scope:

- Input: frozen P1/M549U native candidate surface, M654 coordinate candidates,
  M666 per-positive rank rows, M667/M668 scorer features.
- Output: query-local coordinate delta candidates, not a final rerank score.
- Training: query-internal pairwise/listwise boundary objective.
- Constraints: no CUB regression, no O@100/O@256 regression beyond floor,
  no MAP/NDCG/MRR regression.
- Gate: shared8 first, then shared15.

Stop if M741 cannot reproduce M654 oracle movement on held-out boundary rows
without M667-style dense-overlap loss.
