# M1917-M1918 Parent Selection And PPLX Evidence Report

Date: 2026-07-13

Decision at experiment start: **retain all three frozen routes as controls,
test OpenSearch and M1914 trainability, and use PPLX only inside a heterogeneous
teacher.** The completed closure selects frozen OpenSearch as the product
parent, retains M1914 and P1 as controls, and rejects both the M1918 checkpoint
and PPLX tied-vocabulary LoRA. See
`docs/research-sae/reports/m1900-m1999/ii42-m1917-m1920-parent-and-pplx-closure-report.md`.

## Exact Native Result

M1917 evaluated P1, calibrated Granite M1914, and OpenSearch sparse-v2 on the
same full official FiQA, ArguAna, NFCorpus, and SciFact corpora. All routes use
one exact PostgreSQL normalized-postings backend, candidate depth 1,000, the
same query IDs, and the same frozen PPLX dense reference.

| Route | NDCG@10 | MAP@100 | R@100 | MRR@20 | O@100 | CUB | Storage MiB | p95 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| P1 | 0.444319 | 0.352600 | **0.731116** | 0.478968 | **0.493404** | **0.866933** | **703.59** | 108.038 |
| M1914 | 0.452702 | 0.353850 | 0.725207 | 0.490606 | 0.401635 | 0.860291 | 1026.16 | **90.848** |
| OpenSearch | **0.455414** | **0.355934** | 0.716066 | **0.495727** | 0.404142 | 0.854005 | 1276.27 | 176.284 |

The PostgreSQL storage and latency numbers describe this common exact backend,
not each model's separately optimized Lucene/BMP serving engine. They are valid
for equal-backend comparison but must not be interpreted as OpenSearch's own
production latency.

## Why There Is No Single Parent

- P1 is the strongest dense-faithfulness and Recall route. It dominates FiQA
  and has the highest macro CUB, but is severely worse on NFCorpus and SciFact.
- OpenSearch has the highest macro head quality. It is already a mature broad
  learned-sparse product baseline, but loses 0.1144 FiQA Recall against P1 and
  loses 0.0200 SciFact Recall against M1914.
- M1914 is the fastest route on the common backend and has better macro Recall
  than OpenSearch. It leads OpenSearch on NFCorpus Recall/MAP and SciFact
  Recall, but does not repair the FiQA gap.

OpenSearch and M1914 are also not interchangeable. At top100, M1914 contains
616 relevant pairs missed by OpenSearch, while OpenSearch contains 671 missed
by M1914. P1 contains 917 relevant pairs missed by OpenSearch, while OpenSearch
contains 1,179 missed by P1. Selecting one frozen route and discarding the
other teacher signals would remove measurable retrieval information.

## PPLX Observability Result

| Sparse route | PPLX-only positive rescue @100 | Sparse-only rescue @100 |
| --- | ---: | ---: |
| P1 | 6.5589% | 4.4106% |
| M1914 | 7.0532% | 6.2167% |
| OpenSearch | 6.9265% | 6.4385% |

PPLX has useful unique semantic coverage. It is therefore a valuable teacher.
It is not a sufficient teacher: every sparse route also rescues a material set
of positives which PPLX misses. A student trained only to imitate PPLX would be
explicitly supervised to erase some of the strongest sparse behavior.

The appropriate target is a heterogeneous relevance distribution over the
union of PPLX and sparse-parent candidates, with parent-faithfulness and
candidate membership constraints. This mirrors the successful OpenSearch
training result more closely than dense-only KL.

## What The Literature Changes

The mature systems support a scale-and-initialization explanation, not another
small-loss explanation:

- [OpenSearch sparse-v2](https://arxiv.org/abs/2411.04403) combines dense and
  sparse teachers and an IDF-aware sparsity objective. Its released model card
  reports training over many public query-pair sources.
- [Granite Embedding Models](https://arxiv.org/abs/2502.20204) use
  retrieval-oriented pretraining, contrastive fine-tuning, knowledge
  distillation, and model merging.
- [LACONIC](https://arxiv.org/abs/2601.01684) uses about nine million weakly
  supervised pairs and a second 690K hard-negative phase. At 1B scale, sparse
  remains below its dense counterpart after phase one and approaches it only
  after the hard-negative phase.
- [SPLARE](https://arxiv.org/abs/2603.13277) keeps a pretrained SAE fixed and
  fine-tunes a pretrained backbone with LoRA. It does not learn a random sparse
  basis and retrieval geometry together.
- [DF-FLOPS](https://arxiv.org/abs/2505.15070) shows why ordinary batch FLOPS
  permits universal high-DF terms and refreshes corpus-level DF estimates
  during training.
- [PairDistill](https://arxiv.org/abs/2410.01383) shows that nearby ordering
  benefits from pairwise supervision combined with pointwise distillation and
  candidate refresh.

These results explain the local history. M1820 improved local ordering but not
full-corpus admission because it trained a small head on 512 queries while the
PPLX root was frozen. M1903-M1905 improved candidate-set ranking with depth but
did not control corpus-wide activation. Neither experiment fairly tested a
mature-parent, broad-data continuation.

## Authorized Next Experiment

M1918 performs a paired response test, not a declaration that one parent wins:

1. Prepare a fixed public RLHN surface from NQ, HotpotQA, and FEVER only. A
   complete scan observed 170,803 eligible rows and retained 170,249 after
   strict normalized-query deduplication. FiQA, ArguAna, SciDocsRR, and MS
   MARCO are excluded.
2. Build a query-text-hash train/heldout split and retain one or more positives
   plus at most 15 relabeled hard negatives.
3. Score the same candidate sets with frozen PPLX, OpenSearch, and M1914.
4. Audit teacher disagreement and candidate coverage before optimization.
5. Fine-tune only a small final-layer/LoRA surface of OpenSearch and M1914 with
   parent pointwise, PPLX listwise, boundary pairwise, and refreshed DF-FLOPS
   objectives.
6. Select on public heldout only. BEIR is touched only after the heldout gate.

The PPLX-root student is deferred. It is authorized only if a mature sparse
parent first proves that the additional PPLX signal is learnable without
destroying sparse-only positives and cost. If authorized, it must train a
meaningful backbone LoRA over at least a 100K-row rung and later a million-row
rung; a frozen-root head is not allowed.

## Expected Value And Risk

OpenSearch is the lower-risk product parent because its frozen macro head
quality is highest and its original recipe already supports heterogeneous
distillation. M1914 is the higher-efficiency research parent because it is 30M
parameters and its 50/192 support is fixed, but its FiQA failure is large.

The main risk is not insufficient optimizer steps. It is that parent-specific
unique positives cannot be represented by one common score distribution under
the fixed support and DF budget. The paired response test detects this early:
if both parents reduce heldout loss but lose membership/CUB or increase high-DF
load, the route stops before any full native benchmark.

The credible upside is not an immediate dense-beating model. It is a verified
parent that absorbs PPLX-only rescue while retaining sparse-only rescue and a
bounded posting shape. That would be the first justified foundation for a
larger unified sparse encoder training program.

## Artifacts

- `ii42-m1917-tri-parent-native-matrix.json`
- `docs/research-sae/reports/m1900-m1999/ii42-m1917-tri-parent-native-report.md`
- `ii42-m1917-parent-complementarity.json`
- `docs/research-sae/reports/m1900-m1999/ii42-m1917-parent-complementarity-report.md`
- `docs/research-sae/reports/m1900-m1999/ii42-m1918-mature-parent-continuation-contract.md`
- `ii42-m1918-rlhn-public-pilot-manifest.json`
- `ii42-m1918-rlhn-public-pilot-audit.json`
- `docs/research-sae/reports/m1900-m1999/ii42-m1918-public-data-audit-report.md`
- `scripts/prepare_m1918_rlhn_public_pilot.py`
- `scripts/audit_m1918_public_data.py`
- `scripts/run_m1918_prepare_rlhn_public_pilot_spark.sh`

## Completed Outcome

The OpenSearch parent passed the small heldout response gate at step 1,024,
but failed exact native promotion: NDCG@10, MAP@100, Recall@100, MRR@20, and
dense overlap all regressed on the four-dataset macro. M1914 did not pass its
canary response gate.

A one-shot global impact floor restored the M1918 checkpoint's cost shape but
did not restore frozen OpenSearch quality. The expanded PPLX tied-vocabulary
audit reached 0.797279 dense-score correlation but only 0.681641 positive top1
and did not authorize LoRA. The authoritative final decision and matrices are
recorded in `docs/research-sae/reports/m1900-m1999/ii42-m1917-m1920-parent-and-pplx-closure-report.md`.
