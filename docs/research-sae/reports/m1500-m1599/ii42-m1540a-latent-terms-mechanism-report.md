# M1540A Latent Terms Mechanism Report

Date: 2026-07-10

Decision: **source signal retained; training scale stopped by the exact-cost
gate**. Do not increase token count, epochs, latent size, or tune K/BM25 on
this checkpoint.

## Why This Experiment Was New

M1020 had already trained a small reconstruction SAE over PPLX token states,
and M310 had already shown that atom identity plus IDF can improve candidate
generation. M1520C later tested a 32K BGE token codebook, but its construction
differed from the 2026 Latent Terms paper in four causal mechanisms:

- hierarchical clustering rather than a Top-K SAE;
- max-style bounded aggregation rather than token sum pooling;
- sparse dot product rather than latent-feature BM25;
- explicit background/common-feature removal rather than retaining the
  Zipf-like latent head and letting BM25 IDF/saturation weight it.

M1540A changed those mechanisms while keeping the BGE backbone, 262,144-token
budget, train corpus, canary corpora, and M1520 exact cost contract fixed. It
therefore does not repeat M1020/M1520 and does not claim a 30B-token paper
reproduction.

## Training Health

The 32K Top-K SAE used K=16, transposed decoder initialization, three epochs,
and no retrieval labels.

| Epoch | Normalized MSE | Reconstruction cosine | Active feature ratio |
| ---: | ---: | ---: | ---: |
| 1 | 0.538679 | 0.839156 | 1.000000 |
| 2 | 0.288262 | 0.918247 | 1.000000 |
| 3 | 0.210526 | 0.941143 | 1.000000 |

This rules out optimization collapse, dead features, and insufficient
coverage of the 32K table on the bounded surface. A lower training loss would
not address the observed retrieval/cost frontier by itself.

## Exact Canary Result

`latent_full` keeps every summed latent feature. `latent_bounded` applies the
locked URSI document K=96 and query K=24 budgets.

| Dataset | Source | NDCG@10 | Recall@100 | CUB | Mean touch | Dense-miss recovery |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | exact BGE dense | 0.459368 | 0.386610 | 0.814126 | 1.000000 | - |
| nfcorpus | latent full | 0.388306 | 0.326184 | 0.716740 | 0.882908 | 0.818910 |
| nfcorpus | latent bounded | 0.271022 | 0.259044 | 0.568043 | 0.351832 | 0.589744 |
| scifact | exact BGE dense | 0.848783 | 0.980000 | 1.000000 | 1.000000 | - |
| scifact | latent full | 0.696744 | 0.960000 | 0.980000 | 0.963495 | 1.000000 |
| scifact | latent bounded | 0.522238 | 0.884667 | 0.980000 | 0.374430 | 0.000000 |
| fiqa | exact BGE dense | 0.694897 | 0.934806 | 0.996667 | 1.000000 | - |
| fiqa | latent full | 0.530502 | 0.839813 | 0.980556 | 0.873505 | 0.882353 |
| fiqa | latent bounded | 0.384334 | 0.676321 | 0.909710 | 0.357385 | 0.764706 |

Relative to M1520C, bounded residual recovery changes from `0.137821` to
`0.589744` on NFCorpus and from `0.352941` to `0.764706` on FiQA. The source
gate therefore passes 2/3 rows. SciFact has only one dense-recoverable BM25
miss, so its `0/1` result is not a useful scaling signal.

The exact cost gate fails 0/3. Full latent BM25 touches 87-96% of each corpus.
K96/Q24 reduces mean touch to 35-37%, but still misses the 30% gate and loses
substantial NDCG/Recall. Max DF remains 0.98-1.00. This reproduces the old
M1020 fanout mechanism under a much healthier SAE and paper-aligned scorer.

## Interpretation

The experiment resolves the apparent conflict between M1020/M1520 and Latent
Terms:

1. The paper mechanism is real on our BGE surface. Top-K SAE + sum/sqrt +
   latent BM25 recovers much more dense-miss signal than the M1520 codebook.
2. The paper optimizes retrieval effectiveness, not our strict exact posting
   union budget. Its useful Zipf-like heavy head is exactly what makes an
   unpruned inverted union touch most of the corpus.
3. Static K clipping is not a free cost fix. It removes the cumulative latent
   evidence that gives the source its retrieval value.
4. More reconstruction training is not the current bottleneck. The remaining
   problem is the scorer/index interface and corpus selectivity.

M1540A is therefore not a final recall breakthrough. It is a valid high-recall
semantic source that fails the required engine cost shape.

## Route Decision

Do not scale M1540A to more tokens. Keep the checkpoint as a diagnostic source
only.

The next experiment must not be another SAE loss or K sweep. The strongest
evidence-backed branch is a deterministic pooled-dense compiler capacity test
on this same BGE backbone:

- M392 already showed that signed coordinate admission plus a compact tail
  sketch can preserve a Snowflake dense surface at 5-8% semantic touch.
- M408 showed that dense-to-posting compilation is learnable.
- M549U showed that a monotonic compiler can preserve dense retrieval across
  broad and official surfaces.
- M1540A now shows the small BGE root itself is strong, but token-latent
  expansion is the wrong cost interface.

The next stage should ask whether BGE pooled geometry can be compiled into a
single native posting/forward-sketch index at the locked URSI budget. This is a
capacity audit before training. If it fails, stop the BGE source. If it passes,
add lexical BM25 in the same index and measure a fixed global unified score.

## Artifacts

- Contract: `docs/research-sae/reports/m1500-m1599/ii42-m1540a-latent-terms-mechanism-contract.md`
- Script: `scripts/research_sae_m1540_latent_terms_reproduction.py`
- Runner: `scripts/run_m1540a_latent_terms_spark.sh`
- Result: `runs/m1540a_latent_terms_mechanism_v1/summary.json`
- ClearML task: `2304944e10a641049f7f96a21f768784`
- Remote checkpoint: 193 MB, retained as a failed-cost diagnostic only.

Focused tests, Ruff, Python compilation, shell syntax, and `git diff --check`
passed before execution. No M1540 process remains on Spark.
