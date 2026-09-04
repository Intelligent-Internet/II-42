# M1401 Equal-Budget Signed MaxSim Capacity Report

Date: 2026-07-10
Status: Gate A failed

## Question

M1401 tests whether retaining PPLX token groups and scoring them with sparse
MaxSim or magnitude-routed Signed MaxSim has a better quality/cost frontier
than a P1-style single sparse vector. No text encoder, retrieval projector,
selector, or qrels-derived inference feature is trained.

The comparison uses one frozen PPLX revision for every capacity method:

```text
perplexity-ai/pplx-embed-v1-0.6B
revision 2c4d510dd4a732063c31a0f70193e35067b51fd8
```

The local shared roots provide only text and qrels. Their stored 768-dimension
Snowflake embeddings are not used for candidate generation or scoring. PPLX
1024-dimension sentence roots and token states are recomputed through the same
SentenceTransformer forward interface.

Historical P1.3 native shared15 metrics remain a separate engineering anchor.
`p1_style_single_sparse_dot` below means its scoring shape on the common PPLX
root, not a reproduction of the historical Snowflake P1.3 row.

## Protocol

- Surfaces: sampled `nfcorpus` and `scifact` corpora.
- Queries: 25 independently sampled evaluable queries per dataset.
- Documents: all 2,063 `nfcorpus` rows and all 2,000 `scifact` rows in the
  local sampled roots.
- Candidate set: PPLX dense top 128.
- Diagnostic surface: dense candidates plus missing qrel positives. This is
  explicitly oracle-augmented and is not a deployable result.
- PPLX sequence cap: 128 tokens for this bounded capacity test.
- Retained token groups: at most 24 query and 48 document tokens.
- Single-vector budgets: 32, 64, and 128 active dimensions.
- Token budgets: 1, 2, 4, and 8 active dimensions per token.
- Cost is measured as active occurrences, candidate-local posting touches,
  candidate-local atom document frequency, and touched-document fraction.

## Parity Controls

| Dataset | Root/pooling mean cosine | Minimum cosine | Signed parity max error |
| --- | ---: | ---: | ---: |
| `nfcorpus` | 0.99969783 | 0.99404240 | 1.595e-08 |
| `scifact` | 0.99971188 | 0.99922979 | 2.912e-08 |

The earlier smoke mismatch was a lineage error: it compared current PPLX token
states with the historical Snowflake root. The corrected same-forward parity
is clean.

## Dense-Candidate Results

### nfcorpus

| Method | R@100 | MAP@100 | NDCG@10 | MRR@20 | O@100 | Doc occurrences | Touches |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| exact PPLX dense | 0.28512 | 0.16296 | 0.34748 | 0.56000 | 1.0000 | 997.3 | 125,926 |
| dense token MaxSim | 0.28174 | 0.16933 | 0.33875 | 0.54667 | 0.8312 | 49,087.6 | 6,283,469 |
| P1-style dot k128 | 0.27423 | 0.15461 | 0.31336 | 0.53558 | 0.8252 | 128.0 | 2,580 |
| Signed MaxSim k2 | 0.26078 | 0.05760 | 0.16264 | 0.35747 | 0.7876 | 95.9 | 639 |
| Signed MaxSim k8 | 0.26356 | 0.08364 | 0.21816 | 0.39400 | 0.7932 | 383.5 | 4,444 |
| sparse MaxSim k8 | 0.27522 | 0.08535 | 0.22149 | 0.42422 | 0.8032 | 383.5 | 4,259 |

### scifact

| Method | R@100 | MAP@100 | NDCG@10 | MRR@20 | O@100 | Doc occurrences | Touches |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| exact PPLX dense | 1.00000 | 0.79629 | 0.81407 | 0.80063 | 1.0000 | 996.5 | 125,110 |
| dense token MaxSim | 0.96000 | 0.66567 | 0.69053 | 0.66307 | 0.8360 | 49,152.0 | 6,291,456 |
| P1-style dot k128 | 1.00000 | 0.71525 | 0.72996 | 0.71230 | 0.8244 | 128.0 | 2,985 |
| Signed MaxSim k2 | 0.96000 | 0.24738 | 0.28154 | 0.23880 | 0.7856 | 96.0 | 1,491 |
| Signed MaxSim k8 | 0.96000 | 0.38418 | 0.43206 | 0.38771 | 0.7916 | 384.0 | 9,561 |
| sparse MaxSim k8 | 0.96000 | 0.47953 | 0.54202 | 0.47857 | 0.7988 | 384.0 | 9,912 |

No sparse late-interaction point dominates the P1-style single-vector
frontier. Increasing token k raises cost and fanout without recovering the
single-vector head metrics. Signed routing is also not better than the
non-negative sparse MaxSim control on these frozen raw-token coordinates.

## Oracle-Augmented Diagnostic

On `nfcorpus`, inserting all missing qrel positives exposes a real but unsafe
tail signal:

| Method | R@100 | MAP@100 | NDCG@10 | MRR@20 | Touches |
| --- | ---: | ---: | ---: | ---: | ---: |
| exact PPLX dense | 0.28512 | 0.16296 | 0.34748 | 0.56000 | 146,743 |
| dense token MaxSim | 0.38386 | 0.17813 | 0.33875 | 0.54667 | 7,320,904 |
| P1-style dot k128 | 0.36815 | 0.16615 | 0.31649 | 0.53558 | 2,973 |
| Signed MaxSim k2 | 0.61788 | 0.13213 | 0.23456 | 0.43197 | 721 |
| Signed MaxSim k4 | 0.66549 | 0.13296 | 0.21411 | 0.39148 | 1,898 |
| sparse MaxSim k2 | 0.67954 | 0.13500 | 0.19481 | 0.43816 | 824 |

Late interaction can promote injected tail positives, but it exchanges head
ranking quality for Recall. The full dense-token ceiling has the same shape:
Recall and MAP rise, NDCG and MRR fall, and touches grow by about 50 times.
This is not the required Pareto improvement and relies on an oracle candidate
surface.

`scifact` already has candidate upper bound 1.0 on its dense surface, so its
oracle-augmented matrix is identical and gives no hidden rescue case.

## Failure Mechanism

1. PPLX pooled sentence geometry is strong, but raw token geometry was not
   trained as a late-interaction retriever.
2. MaxSim exposes tail movement, but does not preserve the head ordering that
   supplies MAP, NDCG, MRR, and dense overlap.
3. Top-k token sparsification does not separate useful movement from harmful
   movement. At k4/k8 it touches nearly every candidate document.
4. Magnitude-routed signs solve signed score transport, but they do not create
   retrieval semantics absent from the token coordinates.
5. This repeats the broader project lesson: score expressiveness is not the
   same as qrels-free safe action observability.

## Gate Decision

Gate A fails on two independent 25-query surfaces with the same mechanism.
This triggers M1400 stop rules 1, 5, and 7:

- no quality/cost Pareto improvement over single sparse dot;
- useful-looking token interaction approaches full candidate fanout;
- the same failure repeats on `nfcorpus` and `scifact`.

M1402 frozen-root training is therefore not started. Starting it would violate
the predeclared capacity-before-training rule and would repeat the project's
loss-first experimentation pattern.

This result falsifies the current raw-token Signed MaxSim route. It does not
claim that every separately pretrained late-interaction model is impossible;
such a model would be a different hypothesis and would need a new goal with a
new anchor and cost contract.

## Artifacts

```text
runs/m1401_signed_maxsim_capacity_v1/
  nfcorpus_m1401_capacity.json
  nfcorpus_m1401_capacity.md
  scifact_m1401_capacity.json
  scifact_m1401_capacity.md
```
