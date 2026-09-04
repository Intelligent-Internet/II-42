# M1212 Fused-Tail Proposal Frontier

M1212 stops local policy tuning and goes back to proposal generation.  The
question is whether a different native proposal source can produce a safer
delta without requiring a complex risk classifier.

The smoke grid tested:

- sources: `fused_tail`, `p1_tail`, `bm25_tail`
- doc counts: 1, 3
- scales: 0.05, 0.075, 0.10
- smoke datasets: `cqadupstack`, `scidocs`, `webis-touche2020`

The only useful smoke signal was `fused_tail_docs3`.  It was then replayed on
full shared15 at scales 0.075 and 0.10.

## Smoke Result

Best hard-row smoke variant:

| Variant | dRecall | dMAP | dNDCG | dMRR | dCUB |
| --- | ---: | ---: | ---: | ---: | ---: |
| `fused_tail_docs3_append8_s0.10_shared1` | +0.001582 | +0.000658 | +0.000774 | +0.002524 | +0.000000 |

This was the first tail-family smoke in this stage that did not show immediate
CUB/rank damage on the hard-row subset.

## Full Shared15 Result

| Variant | dRecall | dMAP | dNDCG | dMRR | dCUB | Top95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `fused_tail_docs3_append8_s0.075_shared1` | +0.000554 | +0.000511 | +0.000800 | +0.000521 | +0.000058 | 0.957377 |
| `fused_tail_docs3_append8_s0.10_shared1` | +0.000376 | +0.000765 | +0.000957 | +0.001092 | +0.000081 | 0.949620 |

## Interpretation

This is a small but real proposal-source signal.

Unlike earlier `bm25_tail` and `dual_tail` attempts, `fused_tail_docs3` is
macro-safe on full shared15: Recall, MAP, NDCG, MRR, and CUB all improve.
However, the improvement is much smaller than the current M1191/M1210 frontier.
It is not a replacement for BM25-top proposal.

The useful reading is narrower:

- `fused_tail_docs3` is a safe fallback/probing source.
- It may help hard rows where `top1` fallback is too lexical and loses CUB.
- It is not strong enough as an always-on action.

## Decision

- Do not promote fused-tail as a default action.
- Keep it as a candidate fallback source for a future action-family test.
- Do not run a broad tail grid; the smoke already rejected most tail variants.

## Next Direction

The only follow-up worth testing is a minimal action-family substitution:

- high: `bm25_top_docs3_s0.10`
- mid: `bm25_top_docs3_s0.075`
- fallback: `fused_tail_docs3_s0.10` or `fused_tail_docs3_s0.075`

This should be tested once against the M1210 ordered policy.  If it cannot
beat M1210/M1191, proposal-source fallback tuning should stop and the next
breakthrough must come from a trained proposal-confidence head or a different
generated-posting objective.

## Artifacts

- Smoke JSON: `runs/m1212_tail_source_frontier_smoke_v1/m1188_context_delta_full_shared15.json`
- Smoke Markdown: `runs/m1212_tail_source_frontier_smoke_v1/m1188_context_delta_full_shared15.md`
- Full JSON: `runs/m1212_fused_tail_docs3_frontier_v1/m1188_context_delta_full_shared15.json`
- Full Markdown: `runs/m1212_fused_tail_docs3_frontier_v1/m1188_context_delta_full_shared15.md`
