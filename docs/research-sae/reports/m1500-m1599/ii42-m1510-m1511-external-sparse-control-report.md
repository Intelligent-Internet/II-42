# M1510-M1511 External Sparse Control Report

Date: 2026-07-10

Decision: stop M1512 pooled SPLARE-like training; admit one bounded M1513 SSR
grouped-posting canary.

## Truth Boundary

SPLARE does not provide an official public checkpoint. M1510 evaluates the
independent `aswath86/splare-finetune-opensearch-demo` reproduction at commit
`fe4c95c9`, not an official SPLARE score. Its reported training sources are
approximately 89k MIRACL and Mr.TyDi rows. The three evaluated BEIR datasets
are not listed training sources, but the adapter model card is incomplete, so
these rows are OOD controls rather than formally proven clean-heldout results.

SSR provides MIT-licensed code at commit `c64bc0599`, but no public checkpoint
was discoverable at the 2026-07-09 cutoff. That is an artifact-availability
gap, not a negative model result.

No M1510 query or document encoding, pruning, candidate generation, or scoring
uses BEIR qrels. The P1 rows below are seen-regression references.

## Full-Corpus Quality

| Dataset | Source | NDCG@10 | MAP@100 | Recall@100 | MRR@20 | CUB@1000 |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | SPLARE reproduction | 0.3695 | 0.1881 | 0.3356 | 0.5885 | 0.6533 |
| nfcorpus | dense | 0.3228 | 0.1398 | 0.2691 | 0.5488 | 0.5420 |
| nfcorpus | P1-a0125 | 0.3040 | 0.1393 | 0.2864 | 0.5142 | 0.5035 |
| scifact | SPLARE reproduction | 0.6956 | 0.6545 | 0.9533 | 0.6634 | 0.9900 |
| scifact | dense | 0.7141 | 0.6851 | 0.8982 | 0.6945 | 0.9182 |
| scifact | P1-a0125 | 0.6790 | 0.6489 | 0.9043 | 0.6549 | 0.9677 |
| fiqa | SPLARE reproduction | 0.3338 | 0.2760 | 0.6363 | 0.4053 | 0.8369 |
| fiqa | dense | 0.5106 | 0.4517 | 0.8041 | 0.5997 | 0.9125 |
| fiqa | P1-a0125 | 0.4490 | 0.3855 | 0.7029 | 0.5432 | 0.8630 |

The result is not uniformly negative. On `nfcorpus`, every reported quality
metric beats dense and P1. On `scifact`, every metric beats P1, but the model
trades dense NDCG/MAP/MRR for Recall. On `fiqa`, it beats BM25 but loses to P1
and dense on all four quality metrics. Retrieval training therefore creates a
real sparse semantic signal, but this reproduction does not generalize as a
quality Pareto improvement.

## Full-Corpus Cost

| Dataset | Total postings | Max DF ratio | Head 1% share | Mean touched docs | Mean posting touches |
| --- | ---: | ---: | ---: | ---: | ---: |
| nfcorpus | 1,441,757 | 1.0000 | 0.2287 | 3,633 / 3,633 | 32,811 |
| scifact | 2,049,631 | 1.0000 | 0.2347 | 5,183 / 5,183 | 51,753 |
| fiqa | 18,779,119 | 1.0000 | 0.2906 | 57,638 / 57,638 | 485,778 |

All three rows hit the predefined pooled-route stop condition. A sparse
inverted index that reaches every document per query is not an acceptable
quality/cost frontier, even when its NumPy canary latency looks small.

## M1511 Failure Localization

M1511 performs no qrels-based filtering and no threshold search. For each
target dataset it blocks only dimensions with document DF at least 0.99 in
every other dataset.

| Dataset | LODO blocked atoms | Touch ratio before | Touch ratio after | Query active after |
| --- | ---: | ---: | ---: | ---: |
| nfcorpus | 3 | 1.0000 | 0.9836 | 35.85 |
| scifact | 3 | 1.0000 | 0.9964 | 37.86 |
| fiqa | 5 | 1.0000 | 0.9873 | 37.64 |

Removing corpus-invariant universal atoms does not restore selectivity. The
failure is distributed through the pooled vocabulary, not explained by one or
two trivial background atoms. Post-hoc DF thresholds are therefore not the
next research route.

## What Changed

M1020 showed that reconstruction-only token SAE plus max pooling produced
nearly universal atoms and weak quality. M1401 showed that untrained raw token
coordinates plus sparse MaxSim traded head quality for tail Recall. M1502
showed that qrels-derived oracle factors were not predictable from frozen PPLX
sentence roots.

M1510 adds a new fact: a retrieval-trained token-to-sparse model can materially
improve quality from text without distilling the M1501 oracle field. The text
function is learnable when supervision is placed inside representation
construction. The remaining failure is the single pooled posting shape and
its corpus fanout, not absence of retrieval signal.

## Next Gate

M1512 is not started. M1513 may run one bounded canary using the released SSR
implementation, MS MARCO-only training, and independent full `nfcorpus` and
`scifact` evaluation. It must preserve token groups, train the SAE with
reconstruction/AuxK plus retrieval contrastive supervision, and report K=16
and K=32 quality/cost frontiers.

Stop M1513 if either condition holds:

- both K=16 and K=32 fail to improve the M1401 quality/cost mechanism on two
  independent rows;
- grouped retrieval still touches near-full corpora, selects epoch zero, or
  gains only by exchanging NDCG/MRR for Recall.

Do not expand to shared15 or the native index until this canary passes.

## Artifacts

- `docs/research-sae/reports/m1500-m1599/ii42-m1510-external-sparse-control-contract.md`
- `ii42-m1510-external-artifact-manifest.json`
- `scripts/evaluate_m1510_external_splare.py`
- `scripts/audit_m1511_splare_head_atoms.py`
- `runs/m1510_external_sparse_control_v1/`
